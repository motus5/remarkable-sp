#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Self-update from GitHub Releases. A release provides one binary per CPU
// architecture named "remarkable-sp-<arch>" (arch as QSysInfo reports it:
// arm, arm64, x86_64) plus an optional "<asset>.sha256" checksum file.
// The running executable is replaced atomically (rename), then restarted.
class Updater : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool readyToRestart READ readyToRestart NOTIFY stateChanged)

public:
    explicit Updater(QObject *parent = nullptr);

    QString currentVersion() const;
    QString latestVersion() const { return m_latest; }
    QString status() const { return m_status; }
    bool busy() const { return m_busy; }
    bool updateAvailable() const { return !m_assetUrl.isEmpty(); }
    bool readyToRestart() const { return m_ready; }

    Q_INVOKABLE void check();
    Q_INVOKABLE void install();
    Q_INVOKABLE void restart();

    // Exposed for tests. Returns <0, 0, >0 like strcmp; ignores a leading "v".
    static int compareVersions(const QString &a, const QString &b);
    static QString assetName();

signals:
    void stateChanged();

private:
    void setStatus(const QString &s, bool busy);
    void onReleaseInfo(QNetworkReply *reply);
    void onBinary(QNetworkReply *reply);

    QNetworkAccessManager *m_net = nullptr;
    QString m_repo;
    QString m_latest;
    QString m_assetUrl;
    QString m_checksumUrl;
    QString m_expectedSha;
    QString m_status;
    bool m_busy = false;
    bool m_ready = false;
};
