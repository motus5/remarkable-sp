#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QStringList>

// Local copy of the Super Productivity app state (the "state" object of
// sync-data.json) plus an outbox of operations that have not been uploaded yet.
//
// Every change goes through an operation in SP's compact op format, applied
// by applyOp() - a port of the few desktop reducers this app needs. After a
// download the remote state becomes the new base and pending ops are replayed
// on top, so remote changes of any kind never need a reducer here.
// Unknown slices and fields are kept verbatim, which keeps us forward
// compatible with newer SP versions.
class SpStore : public QObject
{
    Q_OBJECT

public:
    static constexpr int kSchemaVersion = 4;
    static inline const QString kToday = QStringLiteral("TODAY");
    static inline const QString kInbox = QStringLiteral("INBOX_PROJECT");

    explicit SpStore(const QString &dataDir, QObject *parent = nullptr);

    const QJsonObject &state() const { return m_state; }
    QString dataDir() const { return m_dataDir; }

    // Read helpers
    QJsonObject slice(const QString &name) const { return m_state.value(name).toObject(); }
    QJsonObject entity(const QString &sliceName, const QString &id) const;
    QStringList ids(const QString &sliceName) const;
    QJsonObject task(const QString &id) const { return entity(QStringLiteral("task"), id); }
    bool isInToday(const QJsonObject &task) const;
    QString defaultProjectId() const;

    // Mutations (each creates one op, applies it and queues it for upload)
    QString addTask(const QString &title, const QString &projectId, const QStringList &tagIds,
                    const QString &dueDay, const QString &parentId = {});
    void updateTask(const QString &id, const QJsonObject &changes);
    void deleteTask(const QString &id);
    void addTimeSpent(const QString &taskId, const QString &date, qint64 ms);

    // Sync support
    QJsonArray pendingOps() const { return m_pending; }
    int pendingCount() const { return m_pending.size(); }
    // Called after a successful upload of the first `count` pending ops.
    void dropPending(int count);
    // Drop pending ops the remote already contains (upload that we could not confirm).
    int dropPendingIds(const QSet<QString> &ids);
    // Our own vector clock counter must stay above what the remote has seen.
    void raiseClock(int atLeast);
    // Replace the state with a downloaded one and replay the still pending ops.
    void rebase(const QJsonObject &remoteState);
    QString clientId() const { return m_clientId; }
    int nextClock();
    bool hasSyncedState() const { return m_synced; }

    bool save() const;

    static bool applyOp(QJsonObject &state, const QJsonObject &op);
    static QString todayStr(qint64 ms = -1);
    static QString newTaskId();
    static QString newOpId();
    static QJsonObject emptyState();

signals:
    void changed();
    void pendingChanged();

private:
    void dispatch(const QString &code, const QString &opType, const QString &entityId, const QJsonObject &payload,
                  const QStringList &extraEntityIds = {});
    void load();
    void migrateLegacyTasks();

    QString m_dataDir;
    QJsonObject m_state;
    QJsonArray m_pending;
    QString m_clientId;
    int m_clock = 0;
    bool m_synced = false;
};
