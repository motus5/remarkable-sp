#include "updater.h"

#include <QCoreApplication>
#include <cstdio>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QSysInfo>
#include <QVersionNumber>

#ifndef RMSP_VERSION
#define RMSP_VERSION "0.0.0"
#endif
#ifndef RMSP_UPDATE_REPO
#define RMSP_UPDATE_REPO "motus5/remarkable-sp"
#endif

Updater::Updater(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
    m_repo = qEnvironmentVariable("RMSP_UPDATE_REPO", QStringLiteral(RMSP_UPDATE_REPO));
}

QString Updater::currentVersion() const
{
    return QStringLiteral(RMSP_VERSION);
}

QString Updater::assetName()
{
    return QStringLiteral("remarkable-sp-") + QSysInfo::buildCpuArchitecture();
}

int Updater::compareVersions(const QString &a, const QString &b)
{
    auto parse = [](QString s) {
        if (s.startsWith(QLatin1Char('v')))
            s.remove(0, 1);
        return QVersionNumber::fromString(s);
    };
    return QVersionNumber::compare(parse(a), parse(b));
}

void Updater::setStatus(const QString &s, bool busy)
{
    m_status = s;
    m_busy = busy;
    emit stateChanged();
}

void Updater::check()
{
    if (m_busy)
        return;
    m_assetUrl.clear();
    m_checksumUrl.clear();
    setStatus(QStringLiteral("Suche nach Updates …"), true);
    QNetworkRequest req(QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(m_repo)));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("remarkable-sp/") + currentVersion());
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onReleaseInfo(reply); });
}

void Updater::onReleaseInfo(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        setStatus(QStringLiteral("Update-Prüfung fehlgeschlagen: %1").arg(reply->errorString()), false);
        return;
    }
    const QJsonObject rel = QJsonDocument::fromJson(reply->readAll()).object();
    m_latest = rel.value(QStringLiteral("tag_name")).toString();
    if (m_latest.isEmpty() || compareVersions(m_latest, currentVersion()) <= 0) {
        setStatus(QStringLiteral("Aktuell (Version %1).").arg(currentVersion()), false);
        return;
    }
    const QString want = assetName();
    for (const QJsonValue &v : rel.value(QStringLiteral("assets")).toArray()) {
        const QJsonObject a = v.toObject();
        const QString name = a.value(QStringLiteral("name")).toString();
        const QString url = a.value(QStringLiteral("browser_download_url")).toString();
        if (name == want)
            m_assetUrl = url;
        else if (name == want + QStringLiteral(".sha256"))
            m_checksumUrl = url;
    }
    if (m_assetUrl.isEmpty())
        setStatus(QStringLiteral("Version %1 verfügbar, aber ohne Datei für dieses Gerät (%2).").arg(m_latest, want), false);
    else
        setStatus(QStringLiteral("Version %1 verfügbar.").arg(m_latest), false);
}

void Updater::install()
{
    if (m_busy || m_assetUrl.isEmpty())
        return;
    setStatus(QStringLiteral("Lade Version %1 …").arg(m_latest), true);
    m_expectedSha.clear();
    auto fetchBinary = [this] {
        QNetworkRequest req{QUrl(m_assetUrl)};
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *r = m_net->get(req);
        connect(r, &QNetworkReply::finished, this, [this, r] { onBinary(r); });
    };
    if (m_checksumUrl.isEmpty()) {
        fetchBinary();
        return;
    }
    QNetworkRequest req{QUrl(m_checksumUrl)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *r = m_net->get(req);
    connect(r, &QNetworkReply::finished, this, [this, r, fetchBinary] {
        r->deleteLater();
        // "sha256sum" format: "<hex>  <filename>"
        m_expectedSha = QString::fromLatin1(r->readAll()).section(QLatin1Char(' '), 0, 0).trimmed().toLower();
        if (r->error() != QNetworkReply::NoError || m_expectedSha.size() != 64) {
            setStatus(QStringLiteral("Prüfsumme nicht lesbar – Update abgebrochen."), false);
            return;
        }
        fetchBinary();
    });
}

void Updater::onBinary(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        setStatus(QStringLiteral("Download fehlgeschlagen: %1").arg(reply->errorString()), false);
        return;
    }
    const QByteArray data = reply->readAll();
    if (!m_expectedSha.isEmpty()
        && QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() != m_expectedSha.toLatin1()) {
        setStatus(QStringLiteral("Prüfsumme stimmt nicht – Update abgebrochen."), false);
        return;
    }
    const QString exe = QCoreApplication::applicationFilePath();
    const QString tmp = exe + QStringLiteral(".new");
    QFile f(tmp);
    if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size()) {
        setStatus(QStringLiteral("Kann %1 nicht schreiben.").arg(tmp), false);
        return;
    }
    f.close();
    f.setPermissions(QFile::permissions(exe) | QFileDevice::ExeOwner);
    // Keep the previous binary for manual rollback, then swap in the new one.
    QFile::remove(exe + QStringLiteral(".old"));
    QFile::copy(exe, exe + QStringLiteral(".old"));
    if (::rename(QFile::encodeName(tmp).constData(), QFile::encodeName(exe).constData()) != 0) {
        setStatus(QStringLiteral("Ersetzen der Programmdatei fehlgeschlagen."), false);
        return;
    }
    m_ready = true;
    m_assetUrl.clear();
    setStatus(QStringLiteral("Version %1 installiert – Neustart nötig.").arg(m_latest), false);
}

void Updater::restart()
{
    QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments().mid(1));
    QCoreApplication::quit();
}
