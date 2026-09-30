#include "taskmodel.h"

#include "spimport.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTextStream>
#include <QUrl>
#include <QUuid>

// E-ink: redraw the clock once per minute, never per second.
static constexpr int kTickMs = 60 * 1000;

TaskModel::TaskModel(const QString &dataDir, QObject *parent)
    : QAbstractListModel(parent)
    , m_dataDir(dataDir)
{
    QDir().mkpath(m_dataDir + QStringLiteral("/ink"));
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, [this] {
        commitTracking();
        checkFocus();
    });
    load();
}

TaskModel::~TaskModel()
{
    commitTracking();
    save();
}

qint64 TaskModel::now()
{
    return QDateTime::currentMSecsSinceEpoch();
}

QString TaskModel::dayKey(qint64 ms)
{
    return QDateTime::fromMSecsSinceEpoch(ms).date().toString(Qt::ISODate);
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};
    const Task &t = m_tasks.at(m_visible.at(index.row()));
    switch (role) {
    case IdRole: return t.id;
    case TitleRole: return t.title;
    case NotesRole: return t.notes;
    case IsDoneRole: return t.isDone;
    case IsSubTaskRole: return !t.parentId.isEmpty();
    case TimeSpentRole: return t.timeSpent;
    case TimeEstimateRole: return t.timeEstimate;
    case TodaySpentRole: return t.timeSpentOnDay.value(dayKey(now()));
    case IsTrackingRole: return t.id == m_currentId;
    case InkTitlePathRole: return inkPath(t.id, "title");
    case InkNotesPathRole: return inkPath(t.id, "notes");
    case InkRevisionRole: return m_inkRevision.value(t.id);
    }
    return {};
}

QHash<int, QByteArray> TaskModel::roleNames() const
{
    return {
        {IdRole, "taskId"},
        {TitleRole, "title"},
        {NotesRole, "notes"},
        {IsDoneRole, "isDone"},
        {IsSubTaskRole, "isSubTask"},
        {TimeSpentRole, "timeSpent"},
        {TimeEstimateRole, "timeEstimate"},
        {TodaySpentRole, "todaySpent"},
        {IsTrackingRole, "isTracking"},
        {InkTitlePathRole, "inkTitlePath"},
        {InkNotesPathRole, "inkNotesPath"},
        {InkRevisionRole, "inkRevision"},
    };
}

QString TaskModel::inkPath(const QString &id, const char *kind) const
{
    return QStringLiteral("%1/ink/%2-%3.json").arg(m_dataDir, id, QLatin1String(kind));
}

int TaskModel::indexOf(const QString &id) const
{
    for (int i = 0; i < m_tasks.size(); ++i)
        if (m_tasks[i].id == id)
            return i;
    return -1;
}

void TaskModel::rebuildVisible()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_tasks.size(); ++i) {
        const Task &t = m_tasks[i];
        if (!t.parentId.isEmpty())
            continue;
        if (t.isDone == m_showDone)
            m_visible.append(i);
        for (const QString &sub : t.subTaskIds) {
            const int si = indexOf(sub);
            if (si >= 0 && m_tasks[si].isDone == m_showDone)
                m_visible.append(si);
        }
    }
    endResetModel();
    emit statsChanged();
}

void TaskModel::taskChanged(const QString &id)
{
    const int ti = indexOf(id);
    const int row = m_visible.indexOf(ti);
    if (row >= 0)
        emit dataChanged(index(row), index(row));
    emit statsChanged();
    save();
}

void TaskModel::setShowDone(bool v)
{
    if (v == m_showDone)
        return;
    m_showDone = v;
    emit showDoneChanged();
    rebuildVisible();
}

QString TaskModel::currentTaskTitle() const
{
    const int i = indexOf(m_currentId);
    return i >= 0 ? m_tasks[i].title : QString();
}

int TaskModel::openCount() const
{
    int n = 0;
    for (const Task &t : m_tasks)
        n += t.isDone ? 0 : 1;
    return n;
}

int TaskModel::doneTodayCount() const
{
    const QString today = dayKey(now());
    int n = 0;
    for (const Task &t : m_tasks)
        n += (t.isDone && t.doneOn && dayKey(t.doneOn) == today) ? 1 : 0;
    return n;
}

qint64 TaskModel::todayTotal() const
{
    const QString today = dayKey(now());
    qint64 sum = 0;
    for (const Task &t : m_tasks) {
        // Subtask time is already contained in the parent in SP's model only
        // for display; here each task books its own time, so plain sum is right.
        sum += t.timeSpentOnDay.value(today);
    }
    return sum;
}

QString TaskModel::addTask(const QString &title, const QString &parentId)
{
    Task t;
    t.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(21);
    t.title = title;
    t.created = now();
    const int pi = indexOf(parentId);
    if (pi >= 0) {
        t.parentId = parentId;
        m_tasks[pi].subTaskIds.append(t.id);
        m_tasks.append(t);
    } else {
        m_tasks.prepend(t); // new tasks go on top, like SP's "add to top"
    }
    if (m_showDone)
        setShowDone(false);
    else
        rebuildVisible();
    save();
    return t.id;
}

void TaskModel::setTitle(const QString &id, const QString &title)
{
    const int i = indexOf(id);
    if (i < 0 || m_tasks[i].title == title)
        return;
    m_tasks[i].title = title;
    taskChanged(id);
    if (id == m_currentId)
        emit trackingChanged();
}

void TaskModel::setNotes(const QString &id, const QString &notes)
{
    const int i = indexOf(id);
    if (i < 0 || m_tasks[i].notes == notes)
        return;
    m_tasks[i].notes = notes;
    taskChanged(id);
}

void TaskModel::setEstimateMinutes(const QString &id, int minutes)
{
    const int i = indexOf(id);
    if (i < 0)
        return;
    m_tasks[i].timeEstimate = qint64(qMax(0, minutes)) * 60 * 1000;
    taskChanged(id);
}

void TaskModel::toggleDone(const QString &id)
{
    const int i = indexOf(id);
    if (i < 0)
        return;
    Task &t = m_tasks[i];
    t.isDone = !t.isDone;
    t.doneOn = t.isDone ? now() : 0;
    if (t.isDone && id == m_currentId)
        stopTracking();
    rebuildVisible();
    save();
}

void TaskModel::removeTask(const QString &id)
{
    const int i = indexOf(id);
    if (i < 0)
        return;
    if (id == m_currentId)
        stopTracking();
    const Task t = m_tasks.at(i);
    QStringList doomed{id};
    doomed += t.subTaskIds;
    if (!t.parentId.isEmpty()) {
        const int pi = indexOf(t.parentId);
        if (pi >= 0)
            m_tasks[pi].subTaskIds.removeAll(id);
    }
    for (const QString &d : std::as_const(doomed)) {
        const int di = indexOf(d);
        if (di >= 0)
            m_tasks.removeAt(di);
        QFile::remove(inkPath(d, "title"));
        QFile::remove(inkPath(d, "notes"));
    }
    rebuildVisible();
    save();
}

void TaskModel::moveTask(const QString &id, int delta)
{
    const int i = indexOf(id);
    if (i < 0 || delta == 0)
        return;
    Task &t = m_tasks[i];
    if (!t.parentId.isEmpty()) {
        const int pi = indexOf(t.parentId);
        if (pi < 0)
            return;
        QStringList &subs = m_tasks[pi].subTaskIds;
        const int si = subs.indexOf(id);
        const int to = qBound(0, si + delta, int(subs.size()) - 1);
        subs.move(si, to);
    } else {
        // Swap with the next top-level task in the requested direction.
        int j = i;
        do {
            j += delta > 0 ? 1 : -1;
        } while (j >= 0 && j < m_tasks.size() && !m_tasks[j].parentId.isEmpty());
        if (j < 0 || j >= m_tasks.size())
            return;
        m_tasks.swapItemsAt(i, j);
    }
    rebuildVisible();
    save();
}

QVariantMap TaskModel::get(const QString &id) const
{
    const int i = indexOf(id);
    if (i < 0)
        return {};
    const Task &t = m_tasks[i];
    return {
        {QStringLiteral("taskId"), t.id},
        {QStringLiteral("title"), t.title},
        {QStringLiteral("notes"), t.notes},
        {QStringLiteral("isDone"), t.isDone},
        {QStringLiteral("isSubTask"), !t.parentId.isEmpty()},
        {QStringLiteral("timeSpent"), t.timeSpent},
        {QStringLiteral("timeEstimate"), t.timeEstimate},
        {QStringLiteral("todaySpent"), t.timeSpentOnDay.value(dayKey(now()))},
        {QStringLiteral("isTracking"), t.id == m_currentId},
        {QStringLiteral("inkTitlePath"), inkPath(t.id, "title")},
        {QStringLiteral("inkNotesPath"), inkPath(t.id, "notes")},
    };
}

void TaskModel::inkSaved(const QString &id)
{
    ++m_inkRevision[id];
    const int row = m_visible.indexOf(indexOf(id));
    if (row >= 0)
        emit dataChanged(index(row), index(row), {InkRevisionRole});
}

void TaskModel::toggleTracking(const QString &id)
{
    if (id == m_currentId) {
        stopTracking();
        return;
    }
    commitTracking();
    const QString prev = m_currentId;
    if (indexOf(id) < 0)
        return;
    m_currentId = id;
    m_trackSince = now();
    m_tick.start();
    emit trackingChanged();
    if (!prev.isEmpty())
        taskChanged(prev);
    taskChanged(id);
}

void TaskModel::stopTracking()
{
    if (m_currentId.isEmpty())
        return;
    commitTracking();
    const QString prev = m_currentId;
    m_currentId.clear();
    m_trackSince = 0;
    if (!focusActive())
        m_tick.stop();
    emit trackingChanged();
    taskChanged(prev);
}

void TaskModel::commitTracking(qint64 nowMs)
{
    if (m_currentId.isEmpty() || m_trackSince == 0)
        return;
    if (nowMs < 0)
        nowMs = now();
    const int i = indexOf(m_currentId);
    if (i < 0 || nowMs <= m_trackSince)
        return;
    Task &t = m_tasks[i];
    t.timeSpentOnDay[dayKey(nowMs)] += nowMs - m_trackSince;
    t.recalcTimeSpent();
    m_trackSince = nowMs;
    taskChanged(t.id);
}

void TaskModel::setFocusMinutes(int m)
{
    m = qBound(1, m, 180);
    if (m == m_focusMinutes)
        return;
    m_focusMinutes = m;
    emit focusChanged();
}

int TaskModel::focusRemaining() const
{
    if (!focusActive())
        return m_focusMinutes;
    const qint64 left = m_focusEnd - now();
    return int(qMax<qint64>(0, (left + 59999) / 60000));
}

void TaskModel::startFocus()
{
    m_focusEnd = now() + qint64(m_focusMinutes) * 60 * 1000;
    m_tick.start();
    emit focusChanged();
}

void TaskModel::stopFocus()
{
    if (!focusActive())
        return;
    m_focusEnd = 0;
    if (m_currentId.isEmpty())
        m_tick.stop();
    emit focusChanged();
}

void TaskModel::checkFocus(qint64 nowMs)
{
    if (!focusActive())
        return;
    if (nowMs < 0)
        nowMs = now();
    if (nowMs >= m_focusEnd) {
        stopFocus();
        emit focusFinished();
    } else {
        emit focusChanged();
    }
}

int TaskModel::importSuperProductivity(const QString &path)
{
    QString local = path;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();
    const spimport::Result r = spimport::parseFile(local);
    if (!r.error.isEmpty()) {
        m_lastError = r.error;
        return -1;
    }
    int added = 0;
    for (const Task &t : r.tasks) {
        if (t.id.isEmpty() || indexOf(t.id) >= 0)
            continue; // already known: keep local state
        m_tasks.append(t);
        ++added;
    }
    m_lastError.clear();
    rebuildVisible();
    save();
    return added;
}

QString TaskModel::importFromInbox()
{
    const QString inbox = m_dataDir + QStringLiteral("/import");
    QDir dir(inbox);
    dir.mkpath(QStringLiteral("done"));
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    if (files.isEmpty())
        return QStringLiteral("Keine Datei in %1 gefunden.").arg(inbox);
    int total = 0;
    QStringList errors;
    for (const QString &name : files) {
        const int n = importSuperProductivity(dir.filePath(name));
        if (n < 0) {
            errors << QStringLiteral("%1: %2").arg(name, m_lastError);
            continue;
        }
        total += n;
        dir.rename(name, QStringLiteral("done/") + name);
    }
    QString msg = QStringLiteral("%1 Aufgabe(n) importiert.").arg(total);
    if (!errors.isEmpty())
        msg += QLatin1Char('\n') + errors.join(QLatin1Char('\n'));
    return msg;
}

QString TaskModel::exportNow() const
{
    QDir().mkpath(m_dataDir + QStringLiteral("/export"));
    const QString path = QStringLiteral("%1/export/aufgaben-%2.md").arg(m_dataDir, dayKey(now()));
    return exportMarkdown(path) ? QStringLiteral("Exportiert nach %1").arg(path)
                                : QStringLiteral("Export fehlgeschlagen: %1").arg(path);
}

QString TaskModel::formatDuration(qint64 ms) const
{
    const qint64 min = ms / 60000;
    if (min < 60)
        return QStringLiteral("%1m").arg(min);
    return QStringLiteral("%1h %2m").arg(min / 60).arg(min % 60, 2, 10, QLatin1Char('0'));
}

bool TaskModel::exportMarkdown(const QString &path) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << "# Aufgaben (" << dayKey(now()) << ")\n\n";
    for (const Task &t : m_tasks) {
        if (!t.parentId.isEmpty())
            continue;
        auto line = [&](const Task &x, const char *indent) {
            out << indent << (x.isDone ? "- [x] " : "- [ ] ")
                << (x.title.isEmpty() ? QStringLiteral("(handschriftlich)") : x.title);
            if (x.timeSpent)
                out << " — " << formatDuration(x.timeSpent);
            out << "\n";
        };
        line(t, "");
        for (const QString &sub : t.subTaskIds) {
            const int si = indexOf(sub);
            if (si >= 0)
                line(m_tasks[si], "  ");
        }
    }
    out.flush();
    return f.commit();
}

bool TaskModel::save() const
{
    QJsonArray arr;
    for (const Task &t : m_tasks)
        arr.append(t.toJson());
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("tasks"), arr);
    QSaveFile f(m_dataDir + QStringLiteral("/tasks.json"));
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

bool TaskModel::load()
{
    QFile f(m_dataDir + QStringLiteral("/tasks.json"));
    if (!f.open(QIODevice::ReadOnly)) {
        rebuildVisible();
        return false;
    }
    m_tasks.clear();
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("tasks")).toArray();
    for (const QJsonValue &v : arr)
        m_tasks.append(Task::fromJson(v.toObject()));
    rebuildVisible();
    return true;
}
