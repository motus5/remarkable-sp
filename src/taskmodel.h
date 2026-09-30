#pragma once

#include "task.h"

#include <QAbstractListModel>
#include <QTimer>
#include <QVector>

class TaskModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool showDone READ showDone WRITE setShowDone NOTIFY showDoneChanged)
    Q_PROPERTY(QString currentTaskId READ currentTaskId NOTIFY trackingChanged)
    Q_PROPERTY(QString currentTaskTitle READ currentTaskTitle NOTIFY trackingChanged)
    Q_PROPERTY(int openCount READ openCount NOTIFY statsChanged)
    Q_PROPERTY(int doneTodayCount READ doneTodayCount NOTIFY statsChanged)
    Q_PROPERTY(qint64 todayTotal READ todayTotal NOTIFY statsChanged)
    Q_PROPERTY(bool focusActive READ focusActive NOTIFY focusChanged)
    Q_PROPERTY(int focusMinutes READ focusMinutes WRITE setFocusMinutes NOTIFY focusChanged)
    Q_PROPERTY(int focusRemaining READ focusRemaining NOTIFY focusChanged)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        NotesRole,
        IsDoneRole,
        IsSubTaskRole,
        TimeSpentRole,
        TimeEstimateRole,
        TodaySpentRole,
        IsTrackingRole,
        InkTitlePathRole,
        InkNotesPathRole,
        InkRevisionRole,
    };

    explicit TaskModel(const QString &dataDir, QObject *parent = nullptr);
    ~TaskModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool showDone() const { return m_showDone; }
    void setShowDone(bool v);
    QString currentTaskId() const { return m_currentId; }
    QString currentTaskTitle() const;
    int openCount() const;
    int doneTodayCount() const;
    qint64 todayTotal() const;
    bool focusActive() const { return m_focusEnd > 0; }
    int focusMinutes() const { return m_focusMinutes; }
    void setFocusMinutes(int m);
    int focusRemaining() const;
    QString dataDir() const { return m_dataDir; }

    Q_INVOKABLE QString addTask(const QString &title = {}, const QString &parentId = {});
    Q_INVOKABLE void setTitle(const QString &id, const QString &title);
    Q_INVOKABLE void setNotes(const QString &id, const QString &notes);
    Q_INVOKABLE void setEstimateMinutes(const QString &id, int minutes);
    Q_INVOKABLE void toggleDone(const QString &id);
    Q_INVOKABLE void removeTask(const QString &id);
    Q_INVOKABLE void moveTask(const QString &id, int delta);
    Q_INVOKABLE QVariantMap get(const QString &id) const;
    // Called after ink of a task was saved so list thumbnails reload it.
    Q_INVOKABLE void inkSaved(const QString &id);

    Q_INVOKABLE void toggleTracking(const QString &id);
    Q_INVOKABLE void stopTracking();
    // Books the time since the last commit onto the current task. Called by the
    // tick timer; tests pass an explicit clock value.
    Q_INVOKABLE void commitTracking(qint64 nowMs = -1);

    Q_INVOKABLE void startFocus();
    Q_INVOKABLE void stopFocus();
    Q_INVOKABLE void checkFocus(qint64 nowMs = -1);

    Q_INVOKABLE int importSuperProductivity(const QString &path);
    Q_INVOKABLE QString lastError() const { return m_lastError; }
    Q_INVOKABLE bool exportMarkdown(const QString &path) const;
    // Device-friendly variants without file dialogs: every *.json dropped into
    // <dataDir>/import is imported and moved to <dataDir>/import/done.
    Q_INVOKABLE QString importFromInbox();
    Q_INVOKABLE QString exportNow() const;
    Q_INVOKABLE QString formatDuration(qint64 ms) const;

    Q_INVOKABLE bool save() const;
    bool load();

    const QVector<Task> &tasks() const { return m_tasks; }
    static QString dayKey(qint64 ms);

signals:
    void showDoneChanged();
    void trackingChanged();
    void statsChanged();
    void focusChanged();
    void focusFinished();

private:
    int indexOf(const QString &id) const;
    void rebuildVisible();
    void taskChanged(const QString &id);
    QString inkPath(const QString &id, const char *kind) const;
    static qint64 now();

    QString m_dataDir;
    QVector<Task> m_tasks;
    QVector<int> m_visible; // rows -> indices into m_tasks
    QHash<QString, int> m_inkRevision;
    bool m_showDone = false;

    QString m_currentId;
    qint64 m_trackSince = 0;
    QTimer m_tick;

    int m_focusMinutes = 25;
    qint64 m_focusEnd = 0;

    QString m_lastError;
};
