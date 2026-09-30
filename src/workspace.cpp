#include "workspace.h"

#include "spstore.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QLocale>
#include <QSaveFile>
#include <QTextStream>

namespace {
QStringList strings(const QJsonValue &v)
{
    QStringList out;
    for (const QJsonValue &x : v.toArray())
        out.append(x.toString());
    return out;
}
qint64 num(const QJsonObject &o, const char *key)
{
    return qint64(o.value(QLatin1String(key)).toDouble());
}
constexpr int kTickMs = 60 * 1000; // e-ink: minute resolution is plenty
}

// ---------------------------------------------------------------------------
// TaskListModel

TaskListModel::TaskListModel(SpStore *store, QObject *parent)
    : QAbstractListModel(parent)
    , m_store(store)
{
}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QHash<int, QByteArray> TaskListModel::roleNames() const
{
    return {
        {IdRole, "taskId"}, {TitleRole, "title"}, {NotesRole, "notes"}, {IsDoneRole, "isDone"},
        {IsSubTaskRole, "isSubTask"}, {IsBacklogRole, "isBacklog"}, {TimeSpentRole, "timeSpent"},
        {TimeEstimateRole, "timeEstimate"}, {DueDayRole, "dueDay"}, {PriorityRole, "priority"},
        {ProjectTitleRole, "projectTitle"}, {TagTitlesRole, "tagTitles"}, {IsTrackingRole, "isTracking"},
        {InkTitlePathRole, "inkTitlePath"}, {InkRevisionRole, "inkRevision"},
    };
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows.at(index.row());
    const QJsonObject t = m_store->task(row.id);
    switch (role) {
    case IdRole: return row.id;
    case TitleRole: return t.value(QStringLiteral("title")).toString();
    case NotesRole: return t.value(QStringLiteral("notes")).toString();
    case IsDoneRole: return t.value(QStringLiteral("isDone")).toBool();
    case IsSubTaskRole: return !t.value(QStringLiteral("parentId")).toString().isEmpty();
    case IsBacklogRole: return row.backlog;
    case TimeSpentRole: return num(t, "timeSpent");
    case TimeEstimateRole: return num(t, "timeEstimate");
    case DueDayRole: return t.value(QStringLiteral("dueDay")).toString();
    case PriorityRole: return t.value(QStringLiteral("priority")).toInt();
    case ProjectTitleRole:
        return m_type == QLatin1String("PROJECT") ? QString()
                                                  : m_store->entity(QStringLiteral("project"), t.value(QStringLiteral("projectId")).toString())
                                                        .value(QStringLiteral("title")).toString();
    case TagTitlesRole: {
        QStringList titles;
        for (const QString &tag : strings(t.value(QStringLiteral("tagIds"))))
            if (!(m_type == QLatin1String("TAG") && tag == m_id))
                titles.append(m_store->entity(QStringLiteral("tag"), tag).value(QStringLiteral("title")).toString());
        titles.removeAll(QString());
        return titles.join(QStringLiteral(", "));
    }
    case IsTrackingRole: return row.id == m_tracking;
    case InkTitlePathRole: return QStringLiteral("%1/ink/%2-title.json").arg(m_store->dataDir(), row.id);
    case InkRevisionRole: return m_inkRev.value(row.id);
    }
    return {};
}

void TaskListModel::setContext(const QString &type, const QString &id, bool showDone)
{
    m_type = type;
    m_id = id;
    m_showDone = showDone;
    refresh();
}

void TaskListModel::setTrackingId(const QString &id)
{
    m_tracking = id;
    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(m_rows.size() - 1), {IsTrackingRole});
}

void TaskListModel::bumpInk(const QString &id)
{
    ++m_inkRev[id];
    for (int i = 0; i < m_rows.size(); ++i)
        if (m_rows[i].id == id)
            emit dataChanged(index(i), index(i), {InkRevisionRole, TitleRole});
}

void TaskListModel::refresh()
{
    const QString kTaskSlice = QStringLiteral("task");
    QStringList parents;
    QSet<QString> backlog;
    QSet<QString> seen;
    auto take = [&](const QString &id, bool isBacklog) {
        if (seen.contains(id))
            return;
        const QJsonObject t = m_store->task(id);
        if (t.isEmpty())
            return;
        seen.insert(id);
        parents.append(id);
        if (isBacklog)
            backlog.insert(id);
    };

    if (m_type == QLatin1String("TAG") && m_id == SpStore::kToday) {
        // Membership by dueDay/dueWithTime, order from the virtual TODAY tag.
        QStringList members;
        for (const QString &id : m_store->ids(kTaskSlice)) {
            const QJsonObject t = m_store->task(id);
            if (t.value(QStringLiteral("parentId")).toString().isEmpty() && m_store->isInToday(t))
                members.append(id);
        }
        for (const QString &id : strings(m_store->entity(QStringLiteral("tag"), SpStore::kToday).value(QStringLiteral("taskIds"))))
            if (members.contains(id))
                take(id, false);
        for (const QString &id : std::as_const(members))
            take(id, false);
    } else if (m_type == QLatin1String("PROJECT")) {
        const QJsonObject p = m_store->entity(QStringLiteral("project"), m_id);
        for (const QString &id : strings(p.value(QStringLiteral("taskIds"))))
            take(id, false);
        for (const QString &id : strings(p.value(QStringLiteral("backlogTaskIds"))))
            take(id, true);
    } else {
        for (const QString &id : strings(m_store->entity(QStringLiteral("tag"), m_id).value(QStringLiteral("taskIds"))))
            take(id, false);
    }

    beginResetModel();
    m_rows.clear();
    for (const QString &id : std::as_const(parents)) {
        const QJsonObject t = m_store->task(id);
        const bool done = t.value(QStringLiteral("isDone")).toBool();
        const bool isSub = !t.value(QStringLiteral("parentId")).toString().isEmpty();
        if (done != m_showDone)
            continue;
        m_rows.append({id, backlog.contains(id)});
        if (isSub)
            continue;
        for (const QString &sub : strings(t.value(QStringLiteral("subTaskIds")))) {
            const QJsonObject s = m_store->task(sub);
            if (s.isEmpty() || (!m_showDone && s.value(QStringLiteral("isDone")).toBool()))
                continue;
            m_rows.append({sub, backlog.contains(id)});
        }
    }
    endResetModel();
    emit countChanged();
}

// ---------------------------------------------------------------------------
// Workspace

Workspace::Workspace(SpStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_list(store)
{
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, [this] {
        commitTracking();
        checkFocus();
    });
    connect(m_store, &SpStore::changed, this, &Workspace::onStoreChanged);
    connect(m_store, &SpStore::pendingChanged, this, &Workspace::dataChanged);
    m_list.setContext(m_type, m_id, m_showDone);
}

Workspace::~Workspace()
{
    commitTracking();
}

QString Workspace::dataDir() const
{
    return m_store->dataDir();
}

void Workspace::onStoreChanged()
{
    // Context may have vanished remotely (project deleted): fall back to Today.
    if (m_type != QLatin1String("TAG") || m_id != SpStore::kToday) {
        const QString slice = m_type == QLatin1String("PROJECT") ? QStringLiteral("project") : QStringLiteral("tag");
        if (m_store->entity(slice, m_id).isEmpty()) {
            m_type = QStringLiteral("TAG");
            m_id = SpStore::kToday;
            emit contextChanged();
        }
    }
    if (!m_current.isEmpty() && m_store->task(m_current).isEmpty()) {
        m_current.clear();
        m_trackSince = 0;
        m_list.setTrackingId({});
        emit trackingChanged();
    }
    m_list.refresh();
    emit dataChanged();
}

QString Workspace::contextTitle() const
{
    if (m_type == QLatin1String("TAG") && m_id == SpStore::kToday)
        return QStringLiteral("Heute");
    const QString slice = m_type == QLatin1String("PROJECT") ? QStringLiteral("project") : QStringLiteral("tag");
    return m_store->entity(slice, m_id).value(QStringLiteral("title")).toString();
}

void Workspace::openContext(const QString &type, const QString &id)
{
    m_type = type;
    m_id = id;
    m_showDone = false;
    m_list.setContext(m_type, m_id, m_showDone);
    emit contextChanged();
}

void Workspace::setShowDone(bool v)
{
    if (v == m_showDone)
        return;
    m_showDone = v;
    m_list.setContext(m_type, m_id, m_showDone);
    emit contextChanged();
}

QVariantList Workspace::projects() const
{
    QVariantList out;
    for (const QString &id : m_store->ids(QStringLiteral("project"))) {
        const QJsonObject p = m_store->entity(QStringLiteral("project"), id);
        if (p.value(QStringLiteral("isArchived")).toBool() || p.value(QStringLiteral("isHiddenFromMenu")).toBool())
            continue;
        int open = 0;
        for (const QString &t : strings(p.value(QStringLiteral("taskIds"))))
            open += m_store->task(t).value(QStringLiteral("isDone")).toBool() ? 0 : 1;
        out.append(QVariantMap{{"id", id}, {"title", p.value(QStringLiteral("title")).toString()}, {"count", open}});
    }
    return out;
}

QVariantList Workspace::tags() const
{
    QVariantList out;
    for (const QString &id : m_store->ids(QStringLiteral("tag"))) {
        if (id == SpStore::kToday)
            continue;
        const QJsonObject t = m_store->entity(QStringLiteral("tag"), id);
        out.append(QVariantMap{{"id", id}, {"title", t.value(QStringLiteral("title")).toString()},
                               {"count", t.value(QStringLiteral("taskIds")).toArray().size()}});
    }
    return out;
}

int Workspace::openCount() const
{
    int n = 0;
    for (const QString &id : m_store->ids(QStringLiteral("task"))) {
        const QJsonObject t = m_store->task(id);
        n += (!t.value(QStringLiteral("isDone")).toBool() && t.value(QStringLiteral("parentId")).toString().isEmpty()
              && m_store->isInToday(t)) ? 1 : 0;
    }
    return n;
}

int Workspace::doneTodayCount() const
{
    const QString today = SpStore::todayStr();
    int n = 0;
    for (const QString &id : m_store->ids(QStringLiteral("task"))) {
        const QJsonObject t = m_store->task(id);
        n += (t.value(QStringLiteral("isDone")).toBool() && num(t, "doneOn") > 0 && SpStore::todayStr(num(t, "doneOn")) == today) ? 1 : 0;
    }
    return n;
}

qint64 Workspace::todayTotal() const
{
    // Parents already contain their subtasks' time.
    const QString today = SpStore::todayStr();
    qint64 sum = 0;
    for (const QString &id : m_store->ids(QStringLiteral("task"))) {
        const QJsonObject t = m_store->task(id);
        if (t.value(QStringLiteral("parentId")).toString().isEmpty())
            sum += qint64(t.value(QStringLiteral("timeSpentOnDay")).toObject().value(today).toDouble());
    }
    return sum;
}

qint64 Workspace::todayEstimate() const
{
    qint64 sum = 0;
    for (const QString &id : m_store->ids(QStringLiteral("task"))) {
        const QJsonObject t = m_store->task(id);
        if (t.value(QStringLiteral("parentId")).toString().isEmpty() && !t.value(QStringLiteral("isDone")).toBool()
            && m_store->isInToday(t))
            sum += qMax<qint64>(0, num(t, "timeEstimate") - num(t, "timeSpent"));
    }
    return sum;
}

int Workspace::pendingCount() const
{
    return m_store->pendingCount();
}

QString Workspace::currentTaskTitle() const
{
    return m_store->task(m_current).value(QStringLiteral("title")).toString();
}

QString Workspace::inkPath(const QString &id, const char *kind) const
{
    return QStringLiteral("%1/ink/%2-%3.json").arg(m_store->dataDir(), id, QLatin1String(kind));
}

QString Workspace::addTask(const QString &title)
{
    QString projectId;
    QStringList tagIds;
    QString dueDay;
    if (m_type == QLatin1String("PROJECT")) {
        projectId = m_id;
    } else if (m_id == SpStore::kToday) {
        projectId = m_store->defaultProjectId();
        dueDay = SpStore::todayStr();
    } else {
        projectId = m_store->defaultProjectId();
        tagIds.append(m_id);
    }
    if (m_showDone)
        setShowDone(false);
    return m_store->addTask(title, projectId, tagIds, dueDay);
}

QString Workspace::addSubTask(const QString &parentId, const QString &title)
{
    if (m_store->task(parentId).isEmpty())
        return {};
    return m_store->addTask(title, {}, {}, {}, parentId);
}

QVariantMap Workspace::get(const QString &id) const
{
    const QJsonObject t = m_store->task(id);
    if (t.isEmpty())
        return {};
    QVariantMap m = t.toVariantMap();
    m.insert(QStringLiteral("taskId"), id);
    m.insert(QStringLiteral("isSubTask"), !t.value(QStringLiteral("parentId")).toString().isEmpty());
    m.insert(QStringLiteral("isTracking"), id == m_current);
    m.insert(QStringLiteral("isToday"), m_store->isInToday(t));
    m.insert(QStringLiteral("todaySpent"), qint64(t.value(QStringLiteral("timeSpentOnDay")).toObject().value(SpStore::todayStr()).toDouble()));
    m.insert(QStringLiteral("projectTitle"), m_store->entity(QStringLiteral("project"), t.value(QStringLiteral("projectId")).toString()).value(QStringLiteral("title")).toString());
    m.insert(QStringLiteral("inkTitlePath"), inkPath(id, "title"));
    m.insert(QStringLiteral("inkNotesPath"), inkPath(id, "notes"));
    return m;
}

void Workspace::setTitle(const QString &id, const QString &title)
{
    const QJsonObject t = m_store->task(id);
    if (t.isEmpty() || t.value(QStringLiteral("title")).toString() == title)
        return;
    m_store->updateTask(id, {{"title", title}});
    if (id == m_current)
        emit trackingChanged();
}

void Workspace::setNotes(const QString &id, const QString &notes)
{
    if (m_store->task(id).value(QStringLiteral("notes")).toString() != notes)
        m_store->updateTask(id, {{"notes", notes}});
}

void Workspace::setEstimateMinutes(const QString &id, int minutes)
{
    m_store->updateTask(id, {{"timeEstimate", qint64(qMax(0, minutes)) * 60 * 1000}});
}

void Workspace::setPriority(const QString &id, int priority)
{
    m_store->updateTask(id, {{"priority", priority >= 1 && priority <= 3 ? QJsonValue(priority) : QJsonValue()}});
}

void Workspace::toggleDone(const QString &id)
{
    const QJsonObject t = m_store->task(id);
    if (t.isEmpty())
        return;
    const bool done = !t.value(QStringLiteral("isDone")).toBool();
    if (done && id == m_current)
        stopTracking();
    QJsonObject c{{"isDone", done}};
    if (done)
        c.insert(QStringLiteral("doneOn"), QDateTime::currentMSecsSinceEpoch());
    else
        c.insert(QStringLiteral("doneOn"), QJsonValue());
    m_store->updateTask(id, c);
}

void Workspace::schedule(const QString &id, int daysFromToday)
{
    QJsonObject c{{"dueWithTime", QJsonValue()}};
    if (daysFromToday < 0)
        c.insert(QStringLiteral("dueDay"), QJsonValue());
    else
        c.insert(QStringLiteral("dueDay"), QDate::currentDate().addDays(daysFromToday).toString(Qt::ISODate));
    m_store->updateTask(id, c);
}

void Workspace::setProject(const QString &id, const QString &projectId)
{
    const QJsonObject t = m_store->task(id);
    if (t.isEmpty() || !t.value(QStringLiteral("parentId")).toString().isEmpty()
        || m_store->entity(QStringLiteral("project"), projectId).isEmpty()
        || t.value(QStringLiteral("projectId")).toString() == projectId)
        return;
    m_store->updateTask(id, {{"projectId", projectId}});
}

void Workspace::toggleTag(const QString &id, const QString &tagId)
{
    const QJsonObject t = m_store->task(id);
    if (t.isEmpty() || tagId == SpStore::kToday)
        return;
    QStringList tagIds = strings(t.value(QStringLiteral("tagIds")));
    if (tagIds.contains(tagId))
        tagIds.removeAll(tagId);
    else
        tagIds.append(tagId);
    m_store->updateTask(id, {{"tagIds", QJsonArray::fromStringList(tagIds)}});
}

void Workspace::removeTask(const QString &id)
{
    if (id == m_current)
        stopTracking();
    const QJsonObject t = m_store->task(id);
    QStringList doomed{id};
    doomed += strings(t.value(QStringLiteral("subTaskIds")));
    m_store->deleteTask(id);
    for (const QString &d : std::as_const(doomed)) {
        QFile::remove(inkPath(d, "title"));
        QFile::remove(inkPath(d, "notes"));
    }
}

void Workspace::inkSaved(const QString &id, bool titleInkEmpty)
{
    m_list.bumpInk(id);
    // Desktop SP needs a readable title; mark handwritten-only tasks.
    if (!titleInkEmpty && m_store->task(id).value(QStringLiteral("title")).toString().isEmpty())
        setTitle(id, kHandwritingTitle);
}

void Workspace::toggleTracking(const QString &id)
{
    if (id == m_current) {
        stopTracking();
        return;
    }
    commitTracking();
    if (m_store->task(id).isEmpty())
        return;
    m_current = id;
    m_trackSince = QDateTime::currentMSecsSinceEpoch();
    m_tick.start();
    m_list.setTrackingId(id);
    emit trackingChanged();
}

void Workspace::stopTracking()
{
    if (m_current.isEmpty())
        return;
    commitTracking();
    m_current.clear();
    m_trackSince = 0;
    if (!focusActive())
        m_tick.stop();
    m_list.setTrackingId({});
    emit trackingChanged();
}

void Workspace::commitTracking(qint64 nowMs)
{
    if (m_current.isEmpty() || m_trackSince == 0)
        return;
    if (nowMs < 0)
        nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs <= m_trackSince)
        return;
    // Book time on the day it was spent, split at midnight.
    qint64 from = m_trackSince;
    while (from < nowMs) {
        const QDateTime fromDt = QDateTime::fromMSecsSinceEpoch(from);
        const qint64 midnight = QDateTime(fromDt.date().addDays(1), QTime(0, 0)).toMSecsSinceEpoch();
        const qint64 to = qMin(nowMs, midnight);
        m_store->addTimeSpent(m_current, SpStore::todayStr(from), to - from);
        from = to;
    }
    m_trackSince = nowMs;
}

void Workspace::setFocusMinutes(int m)
{
    m = qBound(5, m, 180);
    if (m == m_focusMinutes)
        return;
    m_focusMinutes = m;
    emit focusChanged();
}

int Workspace::focusRemaining() const
{
    if (!focusActive())
        return m_focusMinutes;
    const qint64 left = m_focusEnd - QDateTime::currentMSecsSinceEpoch();
    return int(qMax<qint64>(0, (left + 59999) / 60000));
}

void Workspace::startFocus()
{
    m_focusEnd = QDateTime::currentMSecsSinceEpoch() + qint64(m_focusMinutes) * 60 * 1000;
    m_tick.start();
    emit focusChanged();
}

void Workspace::stopFocus()
{
    if (!focusActive())
        return;
    m_focusEnd = 0;
    if (m_current.isEmpty())
        m_tick.stop();
    emit focusChanged();
}

void Workspace::checkFocus(qint64 nowMs)
{
    if (!focusActive())
        return;
    if (nowMs < 0)
        nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs >= m_focusEnd) {
        stopFocus();
        emit focusFinished();
    } else {
        emit focusChanged();
    }
}

QVariantList Workspace::worklog(int days) const
{
    QMap<QString, QVector<QPair<QString, qint64>>> byDay;
    for (const QString &id : m_store->ids(QStringLiteral("task"))) {
        const QJsonObject t = m_store->task(id);
        if (!t.value(QStringLiteral("subTaskIds")).toArray().isEmpty())
            continue; // leaf tasks only, parents just sum them up
        const QJsonObject spent = t.value(QStringLiteral("timeSpentOnDay")).toObject();
        for (auto it = spent.begin(); it != spent.end(); ++it)
            if (it.value().toDouble() > 0)
                byDay[it.key()].append({t.value(QStringLiteral("title")).toString(), qint64(it.value().toDouble())});
    }
    QVariantList out;
    const QString oldest = QDate::currentDate().addDays(-days).toString(Qt::ISODate);
    for (auto it = byDay.end(); it != byDay.begin();) {
        --it;
        if (it.key() < oldest)
            break;
        qint64 total = 0;
        QVariantList tasks;
        auto entries = it.value();
        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
        for (const auto &e : std::as_const(entries)) {
            total += e.second;
            tasks.append(QVariantMap{{"title", e.first}, {"ms", e.second}});
        }
        out.append(QVariantMap{{"day", it.key()}, {"total", total}, {"tasks", tasks}});
    }
    return out;
}

QString Workspace::exportNow() const
{
    QDir().mkpath(m_store->dataDir() + QStringLiteral("/export"));
    const QString path = QStringLiteral("%1/export/aufgaben-%2.md").arg(m_store->dataDir(), SpStore::todayStr());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return QStringLiteral("Export fehlgeschlagen: %1").arg(path);
    QTextStream out(&f);
    out << "# Aufgaben (" << SpStore::todayStr() << ")\n";
    for (const QString &pid : m_store->ids(QStringLiteral("project"))) {
        const QJsonObject p = m_store->entity(QStringLiteral("project"), pid);
        const QStringList ids = strings(p.value(QStringLiteral("taskIds"))) + strings(p.value(QStringLiteral("backlogTaskIds")));
        if (ids.isEmpty())
            continue;
        out << "\n## " << p.value(QStringLiteral("title")).toString() << "\n\n";
        for (const QString &id : ids) {
            const QJsonObject t = m_store->task(id);
            if (t.isEmpty())
                continue;
            auto line = [&](const QJsonObject &x, const char *indent) {
                out << indent << (x.value(QStringLiteral("isDone")).toBool() ? "- [x] " : "- [ ] ")
                    << x.value(QStringLiteral("title")).toString();
                if (num(x, "timeSpent") > 0)
                    out << " — " << formatDuration(num(x, "timeSpent"));
                out << "\n";
            };
            line(t, "");
            for (const QString &sub : strings(t.value(QStringLiteral("subTaskIds"))))
                if (!m_store->task(sub).isEmpty())
                    line(m_store->task(sub), "  ");
        }
    }
    out.flush();
    return f.commit() ? QStringLiteral("Exportiert nach %1").arg(path) : QStringLiteral("Export fehlgeschlagen: %1").arg(path);
}

QString Workspace::formatDuration(qint64 ms) const
{
    const qint64 min = ms / 60000;
    if (min < 60)
        return QStringLiteral("%1m").arg(min);
    return QStringLiteral("%1h %2m").arg(min / 60).arg(min % 60, 2, 10, QLatin1Char('0'));
}

QString Workspace::formatDay(const QString &isoDay) const
{
    const QDate d = QDate::fromString(isoDay, Qt::ISODate);
    if (!d.isValid())
        return isoDay;
    const qint64 diff = QDate::currentDate().daysTo(d);
    if (diff == 0)
        return QStringLiteral("Heute");
    if (diff == 1)
        return QStringLiteral("Morgen");
    if (diff == -1)
        return QStringLiteral("Gestern");
    return QLocale(QLocale::German).toString(d, QStringLiteral("ddd, d. MMM"));
}
