#include "syncengine.h"

#include "spstore.h"
#include "syncbackend.h"
#include "syncfile.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>

SyncEngine::SyncEngine(SpStore *store, const QString &settingsPath, QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_settingsPath(settingsPath)
{
    loadSettings();
    m_timer.setInterval(m_interval * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, [this] { syncNow(); });
    if (configured() && m_interval > 0)
        m_timer.start();
    // Upload local changes shortly after they happen (batched).
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(20 * 1000);
    connect(&m_debounce, &QTimer::timeout, this, [this] { syncNow(); });
    connect(m_store, &SpStore::pendingChanged, this, &SyncEngine::scheduleSoon);
    if (configured())
        QTimer::singleShot(2000, this, [this] { syncNow(); });
}

SyncEngine::~SyncEngine() = default;

void SyncEngine::scheduleSoon()
{
    if (configured() && m_interval > 0 && m_store->pendingCount() > 0 && !m_debounce.isActive())
        m_debounce.start();
}

bool SyncEngine::configured() const
{
    if (m_provider == QLatin1String("webdav"))
        return !m_url.isEmpty();
    if (m_provider == QLatin1String("folder"))
        return !m_localPath.isEmpty();
    return false;
}

bool SyncEngine::encryptionSupported() const
{
    return syncfile::encryptionSupported();
}

QString SyncEngine::lastSyncText() const
{
    return m_lastSync.isValid() ? m_lastSync.toString(QStringLiteral("dd.MM. HH:mm")) : QStringLiteral("noch nie");
}

void SyncEngine::setStatus(const QString &s, bool ok)
{
    m_status = s;
    m_lastOk = ok;
    emit statusChanged();
}

std::unique_ptr<SyncBackend> SyncEngine::makeBackend() const
{
    if (m_provider == QLatin1String("webdav"))
        return std::make_unique<WebDavBackend>(QUrl::fromUserInput(m_url), m_folder, m_user, m_password);
    if (m_provider == QLatin1String("folder"))
        return std::make_unique<FolderBackend>(m_localPath);
    return nullptr;
}

bool SyncEngine::syncNow()
{
    if (m_busy)
        return false;
    std::unique_ptr<SyncBackend> owned;
    SyncBackend *backend = m_override;
    if (!backend) {
        if (!configured()) {
            setStatus(QStringLiteral("Sync ist nicht eingerichtet."), false);
            return false;
        }
        owned = makeBackend();
        backend = owned.get();
    }
    m_busy = true;
    setStatus(QStringLiteral("Synchronisiere …"), true);
    Step step = Step::Retry;
    for (int attempt = 0; attempt < 4 && step == Step::Retry; ++attempt)
        step = syncOnce(*backend);
    if (step == Step::Retry)
        setStatus(QStringLiteral("Sync-Datei wird gerade von einem anderen Gerät geändert – später erneut."), false);
    m_busy = false;
    emit statusChanged();
    return step == Step::Done;
}

SyncEngine::Step SyncEngine::syncOnce(SyncBackend &backend)
{
    const RemoteFile remote = backend.get(kSyncFile);
    if (remote.status == RemoteFile::Error) {
        setStatus(remote.error, false);
        return Step::Failed;
    }
    if (remote.status == RemoteFile::NotFound) {
        if (backend.get(QStringLiteral("sync-ops.json")).status == RemoteFile::Ok)
            setStatus(QStringLiteral("Der Ordner nutzt SPs geteilte Sync-Dateien („Surgical sync“). Bitte in SP ausschalten."), false);
        else if (backend.get(QStringLiteral("__meta_")).status == RemoteFile::Ok)
            setStatus(QStringLiteral("Alte Sync-Daten (SP ≤ 16) gefunden. Bitte Super Productivity aktualisieren und einmal synchronisieren."), false);
        else
            setStatus(QStringLiteral("Keine SP-Daten gefunden (%1). Bitte zuerst in Super Productivity den Sync einrichten und einmal synchronisieren.")
                          .arg(backend.describe()), false);
        return Step::Failed;
    }

    const syncfile::Decoded dec = syncfile::decode(remote.body, m_encryptKey);
    if (!dec.error.isEmpty()) {
        setStatus(dec.error, false);
        return Step::Failed;
    }
    QJsonObject file = dec.json;
    const int version = file.value(QStringLiteral("version")).toInt();
    if (version == 3 || file.value(QStringLiteral("format")).toString() == QLatin1String("split")) {
        setStatus(QStringLiteral("Der Ordner nutzt SPs geteilte Sync-Dateien („Surgical sync“). Bitte in SP ausschalten."), false);
        return Step::Failed;
    }
    if (version != 2) {
        setStatus(QStringLiteral("Unbekanntes Sync-Format (Version %1) – bitte App aktualisieren.").arg(version), false);
        emit remoteNewer();
        return Step::Failed;
    }
    if (file.value(QStringLiteral("schemaVersion")).toInt(1) > SpStore::kSchemaVersion) {
        setStatus(QStringLiteral("Super Productivity ist neuer als diese App – bitte App aktualisieren."), false);
        emit remoteNewer();
        return Step::Failed;
    }

    const QJsonArray recentOps = file.value(QStringLiteral("recentOps")).toArray();
    QSet<QString> remoteIds;
    for (const QJsonValue &v : recentOps)
        remoteIds.insert(v.toObject().value(QStringLiteral("id")).toString());
    // Ops of ours that made it in on an earlier, unconfirmed attempt.
    m_store->dropPendingIds(remoteIds);

    const QJsonObject remoteState = file.value(QStringLiteral("state")).toObject();
    QJsonObject clock = file.value(QStringLiteral("vectorClock")).toObject();
    m_store->raiseClock(clock.value(m_store->clientId()).toInt());

    const QJsonArray pending = m_store->pendingOps();
    if (pending.isEmpty()) {
        m_store->rebase(remoteState);
        m_lastSync = QDateTime::currentDateTime();
        setStatus(QStringLiteral("Synchronisiert."), true);
        return Step::Done;
    }

    // Build the new file.
    const int syncVersion = file.value(QStringLiteral("syncVersion")).toInt() + 1;
    QJsonObject newState = remoteState;
    QJsonArray ops = recentOps;
    const QString me = m_store->clientId();
    for (const QJsonValue &v : pending) {
        QJsonObject op = v.toObject();
        clock.insert(me, m_store->nextClock());
        op.insert(QStringLiteral("v"), clock);
        op.insert(QStringLiteral("sv"), syncVersion);
        SpStore::applyOp(newState, op);
        ops.append(op);
    }
    while (ops.size() > kMaxRecentOps)
        ops.removeFirst();
    file.insert(QStringLiteral("syncVersion"), syncVersion);
    file.insert(QStringLiteral("vectorClock"), clock);
    file.insert(QStringLiteral("lastModified"), QDateTime::currentMSecsSinceEpoch());
    file.insert(QStringLiteral("clientId"), me);
    file.insert(QStringLiteral("state"), newState);
    file.insert(QStringLiteral("recentOps"), ops);
    const QJsonObject first = ops.first().toObject();
    file.insert(QStringLiteral("oldestOpSyncVersion"), first.value(QStringLiteral("sv")).toInt(syncVersion));

    const QString password = (dec.encrypted || !m_encryptKey.isEmpty()) ? m_encryptKey : QString();
    const QByteArray body = syncfile::encode(file, 2, dec.compressed, password);
    if (body.isEmpty()) {
        setStatus(QStringLiteral("Verschlüsseln der Sync-Daten fehlgeschlagen."), false);
        return Step::Failed;
    }

    backend.put(kSyncFile + QStringLiteral(".bak"), remote.body, RemoteFile(), false);
    const PutResult put = backend.put(kSyncFile, body, remote, false);
    if (put.status == PutResult::Conflict)
        return Step::Retry;
    if (put.status == PutResult::Error) {
        setStatus(put.error, false);
        return Step::Failed;
    }
    m_store->dropPending(pending.size());
    m_store->rebase(newState);
    m_lastSync = QDateTime::currentDateTime();
    setStatus(QStringLiteral("Synchronisiert (%1 Änderung(en) hochgeladen).").arg(pending.size()), true);
    return Step::Done;
}

// --- settings ---------------------------------------------------------------

void SyncEngine::loadSettings()
{
    QSettings s(m_settingsPath, QSettings::IniFormat);
    m_provider = s.value(QStringLiteral("sync/provider")).toString();
    m_url = s.value(QStringLiteral("sync/webdavUrl")).toString();
    m_user = s.value(QStringLiteral("sync/webdavUser")).toString();
    m_password = s.value(QStringLiteral("sync/webdavPassword")).toString();
    m_folder = s.value(QStringLiteral("sync/syncFolder"), m_folder).toString();
    m_localPath = s.value(QStringLiteral("sync/localPath")).toString();
    m_encryptKey = s.value(QStringLiteral("sync/encryptKey")).toString();
    m_interval = s.value(QStringLiteral("sync/intervalMinutes"), m_interval).toInt();
}

void SyncEngine::saveSettings()
{
    QSettings s(m_settingsPath, QSettings::IniFormat);
    s.setValue(QStringLiteral("sync/provider"), m_provider);
    s.setValue(QStringLiteral("sync/webdavUrl"), m_url);
    s.setValue(QStringLiteral("sync/webdavUser"), m_user);
    s.setValue(QStringLiteral("sync/webdavPassword"), m_password);
    s.setValue(QStringLiteral("sync/syncFolder"), m_folder);
    s.setValue(QStringLiteral("sync/localPath"), m_localPath);
    s.setValue(QStringLiteral("sync/encryptKey"), m_encryptKey);
    s.setValue(QStringLiteral("sync/intervalMinutes"), m_interval);
    s.sync();
    if (configured() && m_interval > 0)
        m_timer.start(m_interval * 60 * 1000);
    else
        m_timer.stop();
    emit settingsChanged();
}

#define RMSP_SETTER(Name, member)                   \
    void SyncEngine::Name(const QString &v)         \
    {                                               \
        if (v == member)                            \
            return;                                 \
        member = v;                                 \
        saveSettings();                             \
    }
RMSP_SETTER(setProvider, m_provider)
RMSP_SETTER(setWebdavUrl, m_url)
RMSP_SETTER(setWebdavUser, m_user)
RMSP_SETTER(setWebdavPassword, m_password)
RMSP_SETTER(setSyncFolder, m_folder)
RMSP_SETTER(setLocalPath, m_localPath)
RMSP_SETTER(setEncryptKey, m_encryptKey)
#undef RMSP_SETTER

void SyncEngine::setIntervalMinutes(int v)
{
    v = qBound(0, v, 24 * 60);
    if (v == m_interval)
        return;
    m_interval = v;
    saveSettings();
}
