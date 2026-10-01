#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <memory>

class SpStore;
class SyncBackend;

// Two-way sync with Super Productivity's file-based sync (WebDAV, Nextcloud,
// local folder), format v2 = one "sync-data.json" per sync folder.
//
// Download: the remote "state" becomes our base (it already contains every op
// in "recentOps"), pending local ops are replayed on top.
// Upload: our pending ops are appended to "recentOps" with fresh vector clocks,
// "state" is replaced by base+ops, the file is written with If-Match (or an
// md5 compare-and-swap) and retried after a fresh download on conflict.
class SyncEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY settingsChanged)
    Q_PROPERTY(QString webdavUrl READ webdavUrl WRITE setWebdavUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString webdavUser READ webdavUser WRITE setWebdavUser NOTIFY settingsChanged)
    Q_PROPERTY(QString webdavPassword READ webdavPassword WRITE setWebdavPassword NOTIFY settingsChanged)
    Q_PROPERTY(QString syncFolder READ syncFolder WRITE setSyncFolder NOTIFY settingsChanged)
    Q_PROPERTY(QString localPath READ localPath WRITE setLocalPath NOTIFY settingsChanged)
    Q_PROPERTY(QString encryptKey READ encryptKey WRITE setEncryptKey NOTIFY settingsChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY settingsChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY settingsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool lastOk READ lastOk NOTIFY statusChanged)
    Q_PROPERTY(QString lastSyncText READ lastSyncText NOTIFY statusChanged)
    Q_PROPERTY(bool encryptionSupported READ encryptionSupported CONSTANT)

public:
    static constexpr int kMaxRecentOps = 2000;
    static inline const QString kSyncFile = QStringLiteral("sync-data.json");

    SyncEngine(SpStore *store, const QString &settingsPath, QObject *parent = nullptr);
    ~SyncEngine() override;

    QString provider() const { return m_provider; }
    void setProvider(const QString &v);
    QString webdavUrl() const { return m_url; }
    void setWebdavUrl(const QString &v);
    QString webdavUser() const { return m_user; }
    void setWebdavUser(const QString &v);
    QString webdavPassword() const { return m_password; }
    void setWebdavPassword(const QString &v);
    QString syncFolder() const { return m_folder; }
    void setSyncFolder(const QString &v);
    QString localPath() const { return m_localPath; }
    void setLocalPath(const QString &v);
    QString encryptKey() const { return m_encryptKey; }
    void setEncryptKey(const QString &v);
    int intervalMinutes() const { return m_interval; }
    void setIntervalMinutes(int v);

    bool configured() const;
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    bool lastOk() const { return m_lastOk; }
    QString lastSyncText() const;
    bool encryptionSupported() const;

    Q_INVOKABLE bool syncNow();

    // For tests: inject a backend instead of the configured one.
    void setBackendOverride(SyncBackend *backend) { m_override = backend; }

signals:
    void settingsChanged();
    void statusChanged();
    void remoteNewer(); // remote schema is newer than this app: suggest updating

private:
    enum class Step { Done, Retry, Failed };
    Step syncOnce(SyncBackend &backend);
    std::unique_ptr<SyncBackend> makeBackend() const;
    void setStatus(const QString &s, bool ok);
    void saveSettings();
    void loadSettings();
    void scheduleSoon();

    SpStore *m_store;
    QString m_settingsPath;
    QString m_provider; // "", "webdav", "folder"
    QString m_url, m_user, m_password, m_folder = QStringLiteral("super-productivity");
    QString m_localPath, m_encryptKey;
    int m_interval = 10;
    bool m_busy = false;
    bool m_lastOk = true;
    QString m_status;
    QDateTime m_lastSync;
    QTimer m_timer;
    QTimer m_debounce;
    SyncBackend *m_override = nullptr;
};
