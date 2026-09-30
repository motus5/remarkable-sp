#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;

struct RemoteFile {
    enum Status { Ok, NotFound, Error } status = Error;
    QByteArray body;
    QString rev;             // strong ETag, or md5 of the body
    bool strongEtag = false; // true: rev can be used for If-Match
    QString error;
};

struct PutResult {
    enum Status { Ok, Conflict, Error } status = Error;
    QString error;
};

// Minimal file store as SP's file-based providers see it.
class SyncBackend
{
public:
    virtual ~SyncBackend() = default;
    virtual RemoteFile get(const QString &name) = 0;
    // ifRev empty + createOnly: only create; ifRev set: only overwrite that
    // revision; both empty: unconditional (used for the .bak copy only).
    virtual PutResult put(const QString &name, const QByteArray &body, const RemoteFile &base, bool createOnly = false) = 0;
    virtual QString describe() const = 0;

    static QString md5(const QByteArray &data);
};

// A plain folder: SP's "LocalFile" provider, or any folder kept in sync by
// other means (rclone, Syncthing, ...).
class FolderBackend : public SyncBackend
{
public:
    explicit FolderBackend(const QString &path) : m_path(path) {}
    RemoteFile get(const QString &name) override;
    PutResult put(const QString &name, const QByteArray &body, const RemoteFile &base, bool createOnly) override;
    QString describe() const override { return m_path; }

private:
    QString m_path;
};

// WebDAV (incl. Nextcloud): <baseUrl>/<folder>/<name> with Basic auth.
class WebDavBackend : public SyncBackend
{
public:
    WebDavBackend(const QUrl &baseUrl, const QString &folder, const QString &user, const QString &password,
                  int timeoutMs = 30000);
    ~WebDavBackend() override;
    RemoteFile get(const QString &name) override;
    PutResult put(const QString &name, const QByteArray &body, const RemoteFile &base, bool createOnly) override;
    QString describe() const override { return fileUrl(QString()).toString(); }

private:
    struct Response {
        int status = 0;
        QByteArray body;
        QByteArray etag;
        QString error;
    };
    QUrl fileUrl(const QString &name) const;
    Response request(const QByteArray &verb, const QUrl &url, const QByteArray &body = {},
                     const QList<QPair<QByteArray, QByteArray>> &headers = {});
    bool mkcol();

    QNetworkAccessManager *m_net;
    QUrl m_base;
    QString m_folder;
    QByteArray m_auth;
    int m_timeout;
};
