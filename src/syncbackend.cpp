#include "syncbackend.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>

QString SyncBackend::md5(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
}

// ---------------------------------------------------------------------------

RemoteFile FolderBackend::get(const QString &name)
{
    RemoteFile r;
    QFile f(QDir(m_path).filePath(name));
    if (!f.exists()) {
        r.status = RemoteFile::NotFound;
        return r;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = QStringLiteral("Kann %1 nicht lesen.").arg(f.fileName());
        return r;
    }
    r.body = f.readAll();
    r.rev = md5(r.body);
    r.status = RemoteFile::Ok;
    return r;
}

PutResult FolderBackend::put(const QString &name, const QByteArray &body, const RemoteFile &base, bool createOnly)
{
    PutResult r;
    const RemoteFile cur = get(name);
    if (createOnly && cur.status != RemoteFile::NotFound) {
        r.status = PutResult::Conflict;
        return r;
    }
    if (!base.rev.isEmpty() && (cur.status != RemoteFile::Ok || cur.rev != base.rev)) {
        r.status = PutResult::Conflict;
        return r;
    }
    QDir().mkpath(m_path);
    QSaveFile f(QDir(m_path).filePath(name));
    if (!f.open(QIODevice::WriteOnly) || f.write(body) != body.size() || !f.commit()) {
        r.error = QStringLiteral("Kann %1 nicht schreiben.").arg(f.fileName());
        return r;
    }
    r.status = PutResult::Ok;
    return r;
}

// ---------------------------------------------------------------------------

WebDavBackend::WebDavBackend(const QUrl &baseUrl, const QString &folder, const QString &user,
                             const QString &password, int timeoutMs)
    : m_net(new QNetworkAccessManager)
    , m_base(baseUrl)
    , m_folder(folder)
    , m_timeout(timeoutMs)
{
    if (!user.isEmpty() || !password.isEmpty())
        m_auth = "Basic " + (user.toUtf8() + ':' + password.toUtf8()).toBase64();
}

WebDavBackend::~WebDavBackend()
{
    delete m_net;
}

QUrl WebDavBackend::fileUrl(const QString &name) const
{
    // Join like SP does: collapse duplicate slashes between the segments.
    QString path = m_base.path() + QLatin1Char('/') + m_folder + QLatin1Char('/') + name;
    static const QRegularExpression slashes(QStringLiteral("/{2,}"));
    path.replace(slashes, QStringLiteral("/"));
    QUrl u = m_base;
    u.setPath(path);
    return u;
}

WebDavBackend::Response WebDavBackend::request(const QByteArray &verb, const QUrl &url, const QByteArray &body,
                                               const QList<QPair<QByteArray, QByteArray>> &headers)
{
    QNetworkRequest req(url);
    if (!m_auth.isEmpty())
        req.setRawHeader("Authorization", m_auth);
    req.setRawHeader("Cache-Control", "no-cache");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    for (const auto &h : headers)
        req.setRawHeader(h.first, h.second);

    QBuffer *buf = new QBuffer;
    buf->setData(body);
    buf->open(QIODevice::ReadOnly);
    QNetworkReply *reply = m_net->sendCustomRequest(req, verb, buf);
    buf->setParent(reply);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(m_timeout);
    loop.exec();

    Response r;
    r.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.body = reply->readAll();
    r.etag = reply->rawHeader("OC-ETag");
    if (r.etag.isEmpty())
        r.etag = reply->rawHeader("ETag");
    if (r.status == 0)
        r.error = reply->errorString();
    reply->deleteLater();
    return r;
}

static bool isStrongEtag(const QByteArray &etag)
{
    return etag.size() >= 2 && etag.startsWith('"') && etag.endsWith('"');
}

RemoteFile WebDavBackend::get(const QString &name)
{
    RemoteFile f;
    const Response r = request("GET", fileUrl(name));
    if (r.status == 404) {
        f.status = RemoteFile::NotFound;
        return f;
    }
    if (r.status < 200 || r.status >= 300) {
        f.error = r.status == 401 ? QStringLiteral("WebDAV: Anmeldung fehlgeschlagen (401).")
                                  : QStringLiteral("WebDAV-Fehler %1 %2").arg(r.status).arg(r.error);
        return f;
    }
    f.status = RemoteFile::Ok;
    f.body = r.body;
    f.strongEtag = isStrongEtag(r.etag);
    f.rev = f.strongEtag ? QString::fromLatin1(r.etag) : md5(r.body);
    return f;
}

bool WebDavBackend::mkcol()
{
    const Response r = request("MKCOL", fileUrl(QString()));
    return (r.status >= 200 && r.status < 300) || r.status == 405; // 405: exists already
}

PutResult WebDavBackend::put(const QString &name, const QByteArray &body, const RemoteFile &base, bool createOnly)
{
    PutResult res;
    QList<QPair<QByteArray, QByteArray>> headers{{"Content-Type", "application/octet-stream"}};
    if (createOnly) {
        headers.append(qMakePair(QByteArray("If-None-Match"), QByteArray("*")));
    } else if (!base.rev.isEmpty()) {
        if (base.strongEtag) {
            headers.append(qMakePair(QByteArray("If-Match"), base.rev.toLatin1()));
        } else {
            // No usable ETag: compare the content right before writing, like SP.
            const RemoteFile cur = get(name);
            if (cur.status != RemoteFile::Ok || cur.rev != base.rev) {
                res.status = PutResult::Conflict;
                return res;
            }
        }
    }
    Response r = request("PUT", fileUrl(name), body, headers);
    if (r.status == 404 || r.status == 409) {
        if (mkcol())
            r = request("PUT", fileUrl(name), body, headers);
    }
    if (r.status == 412) {
        res.status = PutResult::Conflict;
        return res;
    }
    if (r.status < 200 || r.status >= 300) {
        res.error = QStringLiteral("WebDAV-Upload fehlgeschlagen: %1 %2").arg(r.status).arg(r.error);
        return res;
    }
    // Read back and verify: guards against servers that ignore If-Match.
    const RemoteFile check = get(name);
    if (check.status != RemoteFile::Ok || md5(check.body) != md5(body)) {
        res.status = PutResult::Conflict;
        return res;
    }
    res.status = PutResult::Ok;
    return res;
}
