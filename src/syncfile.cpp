#include "syncfile.h"

#include <QHash>
#include <QJsonDocument>
#include <QMutex>
#include <QRandomGenerator>
#include <QRegularExpression>

#include <zlib.h>

#ifdef RMSP_HAVE_CRYPTO
#include <argon2.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

namespace syncfile {

static const char kPrefix[] = "pf_";

QByteArray gzip(const QByteArray &data)
{
    z_stream zs{};
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return {};
    QByteArray out;
    out.resize(int(deflateBound(&zs, uLong(data.size()))) + 32);
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.constData()));
    zs.avail_in = uInt(data.size());
    zs.next_out = reinterpret_cast<Bytef *>(out.data());
    zs.avail_out = uInt(out.size());
    const int rc = deflate(&zs, Z_FINISH);
    out.resize(int(zs.total_out));
    deflateEnd(&zs);
    return rc == Z_STREAM_END ? out : QByteArray();
}

QByteArray gunzip(const QByteArray &data, bool *ok)
{
    *ok = false;
    z_stream zs{};
    if (inflateInit2(&zs, 15 + 32) != Z_OK) // auto-detect gzip/zlib header
        return {};
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.constData()));
    zs.avail_in = uInt(data.size());
    QByteArray out;
    char buf[65536];
    int rc;
    do {
        zs.next_out = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        rc = inflate(&zs, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END)
            break;
        out.append(buf, int(sizeof(buf) - zs.avail_out));
    } while (rc != Z_STREAM_END);
    inflateEnd(&zs);
    *ok = rc == Z_STREAM_END;
    return out;
}

// SP strips everything outside the base64 alphabet and fixes padding before
// decoding; do the same so hand-edited or wrapped files still load.
static QByteArray fromBase64Lenient(QByteArray b64)
{
    QByteArray clean;
    clean.reserve(b64.size());
    for (char c : std::as_const(b64)) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/')
            clean.append(c);
    }
    while (clean.size() % 4)
        clean.append('=');
    return QByteArray::fromBase64(clean);
}

#ifdef RMSP_HAVE_CRYPTO
static constexpr int kSaltLen = 16;
static constexpr int kIvLen = 12;
static constexpr int kTagLen = 16;
static constexpr int kKeyLen = 32;

static QByteArray argon2Key(const QString &password, const QByteArray &salt)
{
    // Argon2id with 64 MiB takes a noticeable moment on the tablet; SP reuses
    // one salt per session, so caching by (password, salt) pays off.
    static QMutex mutex;
    static QHash<QByteArray, QByteArray> cache;
    const QByteArray pw = password.toUtf8();
    const QByteArray cacheKey = pw + '\0' + salt;
    QMutexLocker lock(&mutex);
    if (cache.contains(cacheKey))
        return cache.value(cacheKey);
    QByteArray key(kKeyLen, Qt::Uninitialized);
    const int rc = argon2id_hash_raw(3, 65536, 1, pw.constData(), size_t(pw.size()), salt.constData(),
                                     size_t(salt.size()), key.data(), size_t(key.size()));
    if (rc != ARGON2_OK)
        return {};
    if (cache.size() > 8)
        cache.clear();
    cache.insert(cacheKey, key);
    return key;
}

static QByteArray legacyKey(const QString &password)
{
    const QByteArray pw = password.toUtf8();
    QByteArray key(kKeyLen, Qt::Uninitialized);
    if (PKCS5_PBKDF2_HMAC(pw.constData(), pw.size(), reinterpret_cast<const unsigned char *>(pw.constData()),
                          pw.size(), 1000, EVP_sha256(), kKeyLen, reinterpret_cast<unsigned char *>(key.data()))
        != 1)
        return {};
    return key;
}

static bool aesGcm(bool enc, const QByteArray &key, const QByteArray &iv, const QByteArray &in, QByteArray &out)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return false;
    bool ok = false;
    int len = 0;
    const auto *k = reinterpret_cast<const unsigned char *>(key.constData());
    const auto *v = reinterpret_cast<const unsigned char *>(iv.constData());
    if (enc) {
        out.resize(in.size() + kTagLen);
        auto *o = reinterpret_cast<unsigned char *>(out.data());
        int total = 0;
        ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
            && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) == 1
            && EVP_EncryptInit_ex(ctx, nullptr, nullptr, k, v) == 1
            && EVP_EncryptUpdate(ctx, o, &len, reinterpret_cast<const unsigned char *>(in.constData()), in.size()) == 1;
        total = len;
        ok = ok && EVP_EncryptFinal_ex(ctx, o + total, &len) == 1;
        total += len;
        ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagLen, o + total) == 1;
        out.resize(total + kTagLen);
    } else {
        if (in.size() < kTagLen) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        const int ctLen = in.size() - kTagLen;
        QByteArray tag = in.right(kTagLen);
        out.resize(qMax(ctLen, 1));
        auto *o = reinterpret_cast<unsigned char *>(out.data());
        int total = 0;
        ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
            && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) == 1
            && EVP_DecryptInit_ex(ctx, nullptr, nullptr, k, v) == 1
            && EVP_DecryptUpdate(ctx, o, &len, reinterpret_cast<const unsigned char *>(in.constData()), ctLen) == 1;
        total = len;
        ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagLen, tag.data()) == 1
            && EVP_DecryptFinal_ex(ctx, o + total, &len) == 1;
        total += len;
        out.resize(total);
    }
    EVP_CIPHER_CTX_free(ctx);
    return ok;
}
#endif

bool encryptionSupported()
{
#ifdef RMSP_HAVE_CRYPTO
    return true;
#else
    return false;
#endif
}

QByteArray encrypt(const QByteArray &plain, const QString &password)
{
#ifdef RMSP_HAVE_CRYPTO
    QByteArray salt(kSaltLen, Qt::Uninitialized), iv(kIvLen, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), kSaltLen) != 1
        || RAND_bytes(reinterpret_cast<unsigned char *>(iv.data()), kIvLen) != 1)
        return {};
    const QByteArray key = argon2Key(password, salt);
    QByteArray ct;
    if (key.isEmpty() || !aesGcm(true, key, iv, plain, ct))
        return {};
    return (salt + iv + ct).toBase64();
#else
    Q_UNUSED(plain);
    Q_UNUSED(password);
    return {};
#endif
}

QByteArray decrypt(const QByteArray &blob, const QString &password, bool *ok)
{
    *ok = false;
#ifdef RMSP_HAVE_CRYPTO
    const QByteArray raw = fromBase64Lenient(blob);
    QByteArray out;
    if (raw.size() >= kSaltLen + kIvLen + kTagLen) {
        const QByteArray key = argon2Key(password, raw.left(kSaltLen));
        if (!key.isEmpty() && aesGcm(false, key, raw.mid(kSaltLen, kIvLen), raw.mid(kSaltLen + kIvLen), out)) {
            *ok = true;
            return out;
        }
    }
    if (raw.size() >= kIvLen + kTagLen) { // legacy PBKDF2 format, also the fallback
        const QByteArray key = legacyKey(password);
        if (!key.isEmpty() && aesGcm(false, key, raw.left(kIvLen), raw.mid(kIvLen), out)) {
            *ok = true;
            return out;
        }
    }
#else
    Q_UNUSED(blob);
    Q_UNUSED(password);
#endif
    return {};
}

Decoded decode(const QByteArray &file, const QString &password)
{
    Decoded d;
    static const QRegularExpression re(QStringLiteral("^pf_(C)?(E)?(\\d+(?:\\.\\d+)?)__"));
    const QString head = QString::fromLatin1(file.left(32));
    const QRegularExpressionMatch m = re.match(head);
    QByteArray body;
    if (m.hasMatch()) {
        d.compressed = !m.captured(1).isEmpty();
        d.encrypted = !m.captured(2).isEmpty();
        d.modelVersion = m.captured(3);
        body = file.mid(m.capturedLength(0));
    } else if (file.startsWith(kPrefix)) {
        d.error = QStringLiteral("Unbekanntes Dateiformat.");
        return d;
    } else {
        body = file; // tolerate plain JSON without prefix
    }

    if (d.encrypted) {
        if (!encryptionSupported()) {
            d.error = QStringLiteral("Sync-Daten sind verschlüsselt, diese Version unterstützt keine Verschlüsselung.");
            return d;
        }
        if (password.isEmpty()) {
            d.needsPassword = true;
            d.error = QStringLiteral("Sync-Daten sind verschlüsselt – bitte Passwort eintragen.");
            return d;
        }
        bool ok = false;
        body = decrypt(body, password, &ok);
        if (!ok) {
            d.needsPassword = true;
            d.error = QStringLiteral("Entschlüsseln fehlgeschlagen – Passwort falsch?");
            return d;
        }
    }
    if (d.compressed) {
        bool ok = false;
        body = gunzip(fromBase64Lenient(body), &ok);
        if (!ok) {
            d.error = QStringLiteral("Entpacken der Sync-Daten fehlgeschlagen.");
            return d;
        }
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        d.error = QStringLiteral("Sync-Daten sind kein gültiges JSON: %1").arg(err.errorString());
        return d;
    }
    d.json = doc.object();
    return d;
}

QByteArray encode(const QJsonObject &json, int modelVersion, bool compress, const QString &password)
{
    QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);
    if (compress)
        body = gzip(body).toBase64();
    const bool enc = !password.isEmpty();
    if (enc) {
        body = encrypt(body, password);
        if (body.isEmpty())
            return {};
    }
    QByteArray out(kPrefix);
    if (compress)
        out += 'C';
    if (enc)
        out += 'E';
    out += QByteArray::number(modelVersion) + "__";
    return out + body;
}

}
