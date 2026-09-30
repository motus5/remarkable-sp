#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QTimer>
#include <QVariantList>

class SpStore;

// Rows of one work context (Today, a project or a tag), SP ordering:
// parents in context order, each followed by its subtasks.
class TaskListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        NotesRole,
        IsDoneRole,
        IsSubTaskRole,
        IsBacklogRole,
        TimeSpentRole,
        TimeEstimateRole,
        DueDayRole,
        PriorityRole,
        ProjectTitleRole,
        TagTitlesRole,
        IsTrackingRole,
        InkTitlePathRole,
        InkRevisionRole,
    };

    TaskListModel(SpStore *store, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setContext(const QString &type, const QString &id, bool showDone);
    void setTrackingId(const QString &id);
    int rowOf(const QString &id) const;
    QString idAt(int row) const { return row >= 0 && row < m_rows.size() ? m_rows[row].id : QString(); }
    void bumpInk(const QString &id);
    void refresh();

signals:
    void countChanged();

private:
    struct Row {
        QString id;
        bool backlog = false;
    };
    SpStore *m_store;
    QString m_type = QStringLiteral("TAG");
    QString m_id = QStringLiteral("TODAY");
    bool m_showDone = false;
    QString m_tracking;
    QVector<Row> m_rows;
    QHash<QString, int> m_inkRev;
};

// Everything QML needs: contexts, task actions, time tracking, focus timer,
// worklog. Exposed as "app".
class Workspace : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TaskListModel *tasks READ tasks CONSTANT)
    Q_PROPERTY(QString contextType READ contextType NOTIFY contextChanged)
    Q_PROPERTY(QString contextId READ contextId NOTIFY contextChanged)
    Q_PROPERTY(QString contextTitle READ contextTitle NOTIFY contextChanged)
    Q_PROPERTY(bool showDone READ showDone WRITE setShowDone NOTIFY contextChanged)
    Q_PROPERTY(QVariantList projects READ projects NOTIFY dataChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY dataChanged)
    Q_PROPERTY(int openCount READ openCount NOTIFY dataChanged)
    Q_PROPERTY(int doneTodayCount READ doneTodayCount NOTIFY dataChanged)
    Q_PROPERTY(qint64 todayTotal READ todayTotal NOTIFY dataChanged)
    Q_PROPERTY(qint64 todayEstimate READ todayEstimate NOTIFY dataChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY dataChanged)
    Q_PROPERTY(QString currentTaskId READ currentTaskId NOTIFY trackingChanged)
    Q_PROPERTY(QString currentTaskTitle READ currentTaskTitle NOTIFY trackingChanged)
    Q_PROPERTY(bool focusActive READ focusActive NOTIFY focusChanged)
    Q_PROPERTY(int focusMinutes READ focusMinutes WRITE setFocusMinutes NOTIFY focusChanged)
    Q_PROPERTY(int focusRemaining READ focusRemaining NOTIFY focusChanged)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)
    // reMarkable-style pen settings, persisted: width 1 = fine, 2 = medium, 3 = thick
    Q_PROPERTY(int penWidth READ penWidth WRITE setPenWidth NOTIFY prefsChanged)
    // Paper template of the notes page: lined, grid, dots, blank
    Q_PROPERTY(QString paperTemplate READ paperTemplate WRITE setPaperTemplate NOTIFY prefsChanged)

public:
    Workspace(SpStore *store, const QString &settingsPath = {}, QObject *parent = nullptr);
    ~Workspace() override;

    TaskListModel *tasks() { return &m_list; }
    QString contextType() const { return m_type; }
    QString contextId() const { return m_id; }
    QString contextTitle() const;
    bool showDone() const { return m_showDone; }
    void setShowDone(bool v);
    QVariantList projects() const;
    QVariantList tags() const;
    int openCount() const;
    int doneTodayCount() const;
    qint64 todayTotal() const;
    qint64 todayEstimate() const;
    int pendingCount() const;
    QString currentTaskId() const { return m_current; }
    QString currentTaskTitle() const;
    bool focusActive() const { return m_focusEnd > 0; }
    int focusMinutes() const { return m_focusMinutes; }
    void setFocusMinutes(int m);
    int focusRemaining() const;
    QString dataDir() const;
    int penWidth() const { return m_penWidth; }
    void setPenWidth(int w);
    QString paperTemplate() const { return m_template; }
    void setPaperTemplate(const QString &t);

    // Paging through the current list like pages of a notebook.
    Q_INVOKABLE QString neighbourTask(const QString &id, int delta) const;
    Q_INVOKABLE int positionOf(const QString &id) const { return m_list.rowOf(id); }

    Q_INVOKABLE void openContext(const QString &type, const QString &id);

    Q_INVOKABLE QString addTask(const QString &title = {});
    Q_INVOKABLE QString addSubTask(const QString &parentId, const QString &title = {});
    Q_INVOKABLE QVariantMap get(const QString &id) const;
    Q_INVOKABLE void setTitle(const QString &id, const QString &title);
    Q_INVOKABLE void setNotes(const QString &id, const QString &notes);
    Q_INVOKABLE void setEstimateMinutes(const QString &id, int minutes);
    Q_INVOKABLE void setPriority(const QString &id, int priority);
    Q_INVOKABLE void toggleDone(const QString &id);
    Q_INVOKABLE void schedule(const QString &id, int daysFromToday); // <0: unschedule
    Q_INVOKABLE void setProject(const QString &id, const QString &projectId);
    Q_INVOKABLE void toggleTag(const QString &id, const QString &tagId);
    Q_INVOKABLE void removeTask(const QString &id);
    Q_INVOKABLE void inkSaved(const QString &id, bool titleInkEmpty);

    Q_INVOKABLE void toggleTracking(const QString &id);
    Q_INVOKABLE void stopTracking();
    Q_INVOKABLE void commitTracking(qint64 nowMs = -1);

    Q_INVOKABLE void startFocus();
    Q_INVOKABLE void stopFocus();
    Q_INVOKABLE void checkFocus(qint64 nowMs = -1);

    // [{day, total, tasks: [{title, ms}]}] for the last `days` days with time.
    Q_INVOKABLE QVariantList worklog(int days = 14) const;
    Q_INVOKABLE QString exportNow() const;
    Q_INVOKABLE QString formatDuration(qint64 ms) const;
    Q_INVOKABLE QString formatDay(const QString &isoDay) const;

    static inline const QString kHandwritingTitle = QStringLiteral("✍ Handschrift (reMarkable)");

signals:
    void contextChanged();
    void dataChanged();
    void trackingChanged();
    void focusChanged();
    void focusFinished();
    void prefsChanged();

private:
    void onStoreChanged();
    QString inkPath(const QString &id, const char *kind) const;

    SpStore *m_store;
    TaskListModel m_list;
    QString m_type = QStringLiteral("TAG");
    QString m_id = QStringLiteral("TODAY");
    bool m_showDone = false;

    QString m_current;
    qint64 m_trackSince = 0;
    QTimer m_tick;
    int m_focusMinutes = 25;
    qint64 m_focusEnd = 0;
    QString m_settingsPath;
    int m_penWidth = 2;
    QString m_template = QStringLiteral("lined");
};
