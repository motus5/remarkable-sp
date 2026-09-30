#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

// Codec for Super Productivity's remote sync files (sync-data.json & co.):
//   "pf_" [C] [E] <modelVersion> "__" <body>
// body = JSON, optionally base64(gzip(JSON)) (C), optionally
// base64(salt16 | iv12 | AES-256-GCM(ciphertext | tag16)) with an Argon2id key (E).
namespace syncfile {

struct Decoded {
    QJsonObject json;
    bool compressed = false;
    bool encrypted = false;
    QString modelVersion;
    QString error;
    bool needsPassword = false;
};

Decoded decode(const QByteArray &file, const QString &password = {});
QByteArray encode(const QJsonObject &json, int modelVersion, bool compress, const QString &password);

bool encryptionSupported();

// Building blocks, exposed for tests.
QByteArray gzip(const QByteArray &data);
QByteArray gunzip(const QByteArray &data, bool *ok);
QByteArray encrypt(const QByteArray &plain, const QString &password);
QByteArray decrypt(const QByteArray &blob, const QString &password, bool *ok);

}
