#include "spstore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSaveFile>

// ---------------------------------------------------------------------------
// JSON helpers

namespace {

const QString kTask = QStringLiteral("task");
const QString kProject = QStringLiteral("project");
const QString kTag = QStringLiteral("tag");
const QString kPlanner = QStringLiteral("planner");
const QString kIds = QStringLiteral("ids");
const QString kEntities = QStringLiteral("entities");

QJsonObject entityOf(const QJsonObject &state, const QString &slice, const QString &id)
{
    return state.value(slice).toObject().value(kEntities).toObject().value(id).toObject();
}

bool hasEntity(const QJsonObject &state, const QString &slice, const QString &id)
{
    return state.value(slice).toObject().value(kEntities).toObject().contains(id);
}

void putEntity(QJsonObject &state, const QString &slice, const QJsonObject &e)
{
    QJsonObject s = state.value(slice).toObject();
    QJsonObject ents = s.value(kEntities).toObject();
    const QString id = e.value(QStringLiteral("id")).toString();
    QJsonArray ids = s.value(kIds).toArray();
    if (!ents.contains(id))
        ids.append(id);
    ents.insert(id, e);
    s.insert(kEntities, ents);
    s.insert(kIds, ids);
    state.insert(slice, s);
}

void removeEntity(QJsonObject &state, const QString &slice, const QString &id)
{
    QJsonObject s = state.value(slice).toObject();
    QJsonObject ents = s.value(kEntities).toObject();
    ents.remove(id);
    QJsonArray ids;
    for (const QJsonValue &v : s.value(kIds).toArray())
        if (v.toString() != id)
            ids.append(v);
    s.insert(kEntities, ents);
    s.insert(kIds, ids);
    state.insert(slice, s);
}

QStringList strings(const QJsonValue &v)
{
    QStringList out;
    for (const QJsonValue &x : v.toArray())
        out.append(x.toString());
    return out;
}

QJsonArray addToList(const QJsonArray &list, const QString &id, bool bottom)
{
    QJsonArray out;
    for (const QJsonValue &v : list)
        if (v.toString() != id)
            out.append(v);
    if (bottom)
        out.append(id);
    else
        out.prepend(id);
    return out;
}

QJsonArray removeFromList(const QJsonArray &list, const QStringList &ids)
{
    QJsonArray out;
    for (const QJsonValue &v : list)
        if (!ids.contains(v.toString()))
            out.append(v);
    return out;
}

qint64 sumDays(const QJsonObject &days)
{
    qint64 sum = 0;
    for (auto it = days.begin(); it != days.end(); ++it)
        sum += qint64(it.value().toDouble());
    return sum;
}

void updateList(QJsonObject &state, const QString &slice, const QString &id, const QString &field,
                const std::function<QJsonArray(const QJsonArray &)> &fn)
{
    if (!hasEntity(state, slice, id))
        return;
    QJsonObject e = entityOf(state, slice, id);
    e.insert(field, fn(e.value(field).toArray()));
    putEntity(state, slice, e);
}

void addTimeToTask(QJsonObject &state, const QString &id, const QString &date, qint64 ms)
{
    QJsonObject t = entityOf(state, kTask, id);
    QJsonObject days = t.value(QStringLiteral("timeSpentOnDay")).toObject();
    days.insert(date, qint64(days.value(date).toDouble()) + ms);
    t.insert(QStringLiteral("timeSpentOnDay"), days);
    t.insert(QStringLiteral("timeSpent"), sumDays(days));
    putEntity(state, kTask, t);
}

// --- reducers, mirroring the SP desktop ones (see README "Sync") ----------

void reduceAddTask(QJsonObject &state, const QJsonObject &p)
{
    QJsonObject task = p.value(QStringLiteral("task")).toObject();
    const QString id = task.value(QStringLiteral("id")).toString();
    if (id.isEmpty() || hasEntity(state, kTask, id))
        return;
    const bool bottom = p.value(QStringLiteral("isAddToBottom")).toBool(true);
    task.insert(QStringLiteral("tagIds"), removeFromList(task.value(QStringLiteral("tagIds")).toArray(), {SpStore::kToday}));
    task.insert(QStringLiteral("timeSpent"), sumDays(task.value(QStringLiteral("timeSpentOnDay")).toObject()));
    const QString projectId = task.value(QStringLiteral("projectId")).toString();
    task.insert(QStringLiteral("projectId"), projectId);
    putEntity(state, kTask, task);

    if (!projectId.isEmpty() && hasEntity(state, kProject, projectId) && task.value(QStringLiteral("parentId")).toString().isEmpty()) {
        const QJsonObject project = entityOf(state, kProject, projectId);
        const QString list = p.value(QStringLiteral("isAddToBacklog")).toBool()
                && project.value(QStringLiteral("isEnableBacklog")).toBool()
            ? QStringLiteral("backlogTaskIds")
            : QStringLiteral("taskIds");
        updateList(state, kProject, projectId, list, [&](const QJsonArray &l) { return addToList(l, id, bottom); });
    }
    const QString dueDay = task.value(QStringLiteral("dueDay")).toString();
    QStringList tagIds = strings(task.value(QStringLiteral("tagIds")));
    if (dueDay == SpStore::todayStr())
        tagIds.append(SpStore::kToday);
    for (const QString &tagId : std::as_const(tagIds))
        updateList(state, kTag, tagId, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return addToList(l, id, bottom); });

    if (!dueDay.isEmpty() && dueDay != SpStore::todayStr()) {
        QJsonObject planner = state.value(kPlanner).toObject();
        QJsonObject days = planner.value(QStringLiteral("days")).toObject();
        days.insert(dueDay, addToList(days.value(dueDay).toArray(), id, bottom));
        planner.insert(QStringLiteral("days"), days);
        state.insert(kPlanner, planner);
    }
}

void reduceAddSubTask(QJsonObject &state, const QJsonObject &p)
{
    QJsonObject task = p.value(QStringLiteral("task")).toObject();
    const QString id = task.value(QStringLiteral("id")).toString();
    const QString parentId = p.value(QStringLiteral("parentId")).toString();
    if (!hasEntity(state, kTask, parentId) || id.isEmpty())
        return;
    QJsonObject parent = entityOf(state, kTask, parentId);
    const bool first = parent.value(QStringLiteral("subTaskIds")).toArray().isEmpty();
    if (first && task.value(QStringLiteral("timeSpentOnDay")).toObject().isEmpty()) {
        const QJsonObject days = parent.value(QStringLiteral("timeSpentOnDay")).toObject();
        task.insert(QStringLiteral("timeSpentOnDay"), days);
        task.insert(QStringLiteral("timeSpent"), sumDays(days));
    }
    if (first && qint64(task.value(QStringLiteral("timeEstimate")).toDouble()) == 0)
        task.insert(QStringLiteral("timeEstimate"), parent.value(QStringLiteral("timeEstimate")));
    task.insert(QStringLiteral("parentId"), parentId);
    task.insert(QStringLiteral("tagIds"), QJsonArray());
    task.insert(QStringLiteral("projectId"), parent.value(QStringLiteral("projectId")));
    putEntity(state, kTask, task);
    parent.insert(QStringLiteral("subTaskIds"), addToList(parent.value(QStringLiteral("subTaskIds")).toArray(), id, true));
    putEntity(state, kTask, parent);
}

void reduceUpdateTask(QJsonObject &state, const QJsonObject &p)
{
    const QJsonObject upd = p.value(QStringLiteral("task")).toObject();
    const QString id = upd.value(QStringLiteral("id")).toString();
    if (!hasEntity(state, kTask, id))
        return;
    QJsonObject changes = upd.value(QStringLiteral("changes")).toObject();
    QJsonObject task = entityOf(state, kTask, id);

    if (changes.contains(QStringLiteral("tagIds"))) {
        const QStringList oldTags = strings(task.value(QStringLiteral("tagIds")));
        QStringList newTags = strings(changes.value(QStringLiteral("tagIds")));
        newTags.removeAll(SpStore::kToday);
        changes.insert(QStringLiteral("tagIds"), QJsonArray::fromStringList(newTags));
        for (const QString &t : oldTags)
            if (!newTags.contains(t))
                updateList(state, kTag, t, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return removeFromList(l, {id}); });
        for (const QString &t : std::as_const(newTags))
            if (!oldTags.contains(t))
                updateList(state, kTag, t, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return addToList(l, id, true); });
    }
    if (changes.contains(QStringLiteral("projectId"))) {
        // Like SP: only top-level tasks move, subtasks follow; unknown targets are ignored.
        const QString target = changes.value(QStringLiteral("projectId")).toString();
        if (!task.value(QStringLiteral("parentId")).toString().isEmpty() || (!target.isEmpty() && !hasEntity(state, kProject, target))) {
            changes.remove(QStringLiteral("projectId"));
        } else if (target != task.value(QStringLiteral("projectId")).toString()) {
            QStringList moved{id};
            moved += strings(task.value(QStringLiteral("subTaskIds")));
            for (const QString &pid : strings(state.value(kProject).toObject().value(kIds))) {
                updateList(state, kProject, pid, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return removeFromList(l, moved); });
                updateList(state, kProject, pid, QStringLiteral("backlogTaskIds"), [&](const QJsonArray &l) { return removeFromList(l, moved); });
            }
            updateList(state, kProject, target, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return addToList(l, id, true); });
            for (const QString &sub : strings(task.value(QStringLiteral("subTaskIds")))) {
                if (!hasEntity(state, kTask, sub))
                    continue;
                QJsonObject s = entityOf(state, kTask, sub);
                s.insert(QStringLiteral("projectId"), target);
                putEntity(state, kTask, s);
            }
        }
    }
    if (changes.value(QStringLiteral("isDone")).toBool() && !changes.contains(QStringLiteral("doneOn")))
        changes.insert(QStringLiteral("doneOn"), QDateTime::currentMSecsSinceEpoch());
    for (auto it = changes.begin(); it != changes.end(); ++it)
        task.insert(it.key(), it.value());
    if (changes.contains(QStringLiteral("timeSpentOnDay")))
        task.insert(QStringLiteral("timeSpent"), sumDays(task.value(QStringLiteral("timeSpentOnDay")).toObject()));
    task.insert(QStringLiteral("modified"), QDateTime::currentMSecsSinceEpoch());
    putEntity(state, kTask, task);
}

void reduceDeleteTask(QJsonObject &state, const QJsonObject &p)
{
    const QJsonObject task = p.value(QStringLiteral("task")).toObject();
    const QString id = task.value(QStringLiteral("id")).toString();
    QStringList doomed{id};
    doomed += strings(task.value(QStringLiteral("subTaskIds")));
    for (const QJsonValue &s : task.value(QStringLiteral("subTasks")).toArray())
        doomed.append(s.toObject().value(QStringLiteral("id")).toString());
    doomed.removeDuplicates();

    const QString parentId = entityOf(state, kTask, id).value(QStringLiteral("parentId")).toString();
    if (!parentId.isEmpty())
        updateList(state, kTask, parentId, QStringLiteral("subTaskIds"), [&](const QJsonArray &l) { return removeFromList(l, {id}); });
    for (const QString &d : std::as_const(doomed))
        removeEntity(state, kTask, d);
    for (const QString &projectId : strings(state.value(kProject).toObject().value(kIds))) {
        updateList(state, kProject, projectId, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return removeFromList(l, doomed); });
        updateList(state, kProject, projectId, QStringLiteral("backlogTaskIds"), [&](const QJsonArray &l) { return removeFromList(l, doomed); });
    }
    for (const QString &tagId : strings(state.value(kTag).toObject().value(kIds)))
        updateList(state, kTag, tagId, QStringLiteral("taskIds"), [&](const QJsonArray &l) { return removeFromList(l, doomed); });
    QJsonObject planner = state.value(kPlanner).toObject();
    QJsonObject days = planner.value(QStringLiteral("days")).toObject();
    for (auto it = days.begin(); it != days.end(); ++it)
        it.value() = removeFromList(it.value().toArray(), doomed);
    planner.insert(QStringLiteral("days"), days);
    if (state.contains(kPlanner))
        state.insert(kPlanner, planner);
}

void reduceSyncTimeSpent(QJsonObject &state, const QJsonObject &p)
{
    const QString id = p.value(QStringLiteral("taskId")).toString();
    const QString date = p.value(QStringLiteral("date")).toString();
    const qint64 ms = qint64(p.value(QStringLiteral("duration")).toDouble());
    if (!hasEntity(state, kTask, id) || ms <= 0)
        return;
    addTimeToTask(state, id, date, ms);
    const QString parentId = entityOf(state, kTask, id).value(QStringLiteral("parentId")).toString();
    if (!parentId.isEmpty() && hasEntity(state, kTask, parentId))
        addTimeToTask(state, parentId, date, ms);
}

} // namespace

// ---------------------------------------------------------------------------

SpStore::SpStore(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir)
{
    QDir().mkpath(m_dataDir);
    load();
}

QString SpStore::todayStr(qint64 ms)
{
    const QDateTime dt = ms < 0 ? QDateTime::currentDateTime() : QDateTime::fromMSecsSinceEpoch(ms);
    return dt.date().toString(Qt::ISODate);
}

QString SpStore::newTaskId()
{
    // nanoid alphabet and length, like the desktop app
    static const char alphabet[] = "useandom-26T198340PX75pxJACKVERYMINDBUSHWOLF_GQZbfghjklqvwyzrict";
    QString id;
    for (int i = 0; i < 21; ++i)
        id.append(QLatin1Char(alphabet[QRandomGenerator::system()->bounded(64)]));
    return id;
}

QString SpStore::newOpId()
{
    // UUIDv7: 48 bit unix ms timestamp, version 7, variant 10, random rest
    quint8 b[16];
    const quint64 ms = quint64(QDateTime::currentMSecsSinceEpoch());
    for (int i = 0; i < 6; ++i)
        b[i] = quint8(ms >> (8 * (5 - i)));
    for (int i = 6; i < 16; ++i)
        b[i] = quint8(QRandomGenerator::system()->bounded(256));
    b[6] = (b[6] & 0x0F) | 0x70;
    b[8] = (b[8] & 0x3F) | 0x80;
    const QByteArray hex = QByteArray(reinterpret_cast<const char *>(b), 16).toHex();
    return QString::fromLatin1(hex.left(8) + '-' + hex.mid(8, 4) + '-' + hex.mid(12, 4) + '-' + hex.mid(16, 4) + '-' + hex.mid(20));
}

QJsonObject SpStore::emptyState()
{
    // Minimal local state until the first download from Super Productivity.
    auto slice = [](const QJsonObject &entity = {}) {
        QJsonObject s;
        QJsonArray ids;
        QJsonObject ents;
        if (!entity.isEmpty()) {
            ids.append(entity.value(QStringLiteral("id")));
            ents.insert(entity.value(QStringLiteral("id")).toString(), entity);
        }
        s.insert(QStringLiteral("ids"), ids);
        s.insert(QStringLiteral("entities"), ents);
        return s;
    };
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QJsonObject inbox{{"id", kInbox}, {"title", "Inbox"}, {"icon", "inbox"}, {"taskIds", QJsonArray()},
                      {"backlogTaskIds", QJsonArray()}, {"noteIds", QJsonArray()}, {"isEnableBacklog", false},
                      {"isHiddenFromMenu", false}, {"isArchived", false}, {"created", now}};
    QJsonObject today{{"id", kToday}, {"title", "Today"}, {"icon", "wb_sunny"}, {"taskIds", QJsonArray()},
                      {"created", now}, {"color", QJsonValue::Null}};
    QJsonObject task = slice();
    task.insert(QStringLiteral("currentTaskId"), QJsonValue::Null);
    QJsonObject planner{{"days", QJsonObject()}};
    return QJsonObject{{"task", task}, {"project", slice(inbox)}, {"tag", slice(today)}, {"planner", planner}};
}

QJsonObject SpStore::entity(const QString &sliceName, const QString &id) const
{
    return entityOf(m_state, sliceName, id);
}

QStringList SpStore::ids(const QString &sliceName) const
{
    return strings(m_state.value(sliceName).toObject().value(kIds));
}

bool SpStore::isInToday(const QJsonObject &t) const
{
    const QJsonValue withTime = t.value(QStringLiteral("dueWithTime"));
    if (withTime.isDouble())
        return todayStr(qint64(withTime.toDouble())) == todayStr();
    return t.value(QStringLiteral("dueDay")).toString() == todayStr();
}

QString SpStore::defaultProjectId() const
{
    if (hasEntity(m_state, kProject, kInbox))
        return kInbox;
    const QStringList p = ids(kProject);
    return p.isEmpty() ? QString() : p.first();
}

bool SpStore::applyOp(QJsonObject &state, const QJsonObject &op)
{
    const QString a = op.value(QStringLiteral("a")).toString();
    QJsonObject p = op.value(QStringLiteral("p")).toObject();
    if (p.contains(QStringLiteral("actionPayload")))
        p = p.value(QStringLiteral("actionPayload")).toObject();
    if (a == QLatin1String("HA"))
        reduceAddTask(state, p);
    else if (a == QLatin1String("TA"))
        reduceAddSubTask(state, p);
    else if (a == QLatin1String("HU"))
        reduceUpdateTask(state, p);
    else if (a == QLatin1String("HD"))
        reduceDeleteTask(state, p);
    else if (a == QLatin1String("KT"))
        reduceSyncTimeSpent(state, p);
    else
        return false;
    return true;
}

void SpStore::dispatch(const QString &code, const QString &opType, const QString &entityId, const QJsonObject &payload,
                       const QStringList &extraEntityIds)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QJsonObject op{
        {"id", newOpId()},
        {"a", code},
        {"o", opType},
        {"e", "TASK"},
        {"d", entityId},
        {"ds", QJsonArray::fromStringList(QStringList{entityId} + extraEntityIds)},
        {"p", payload},
        {"c", m_clientId},
        {"t", now},
        {"s", kSchemaVersion},
    };
    applyOp(m_state, op);

    // Coalesce with the op queued last for the same task where SP semantics
    // allow it: additive time (KT) and shallow task updates (HU). This keeps
    // minute-wise time tracking from flooding the 2000-op buffer.
    if (!m_pending.isEmpty() && extraEntityIds.isEmpty()) {
        QJsonObject last = m_pending.last().toObject();
        if (last.value(QStringLiteral("a")).toString() == code && last.value(QStringLiteral("d")).toString() == entityId) {
            QJsonObject lp = last.value(QStringLiteral("p")).toObject();
            QJsonObject la = lp.value(QStringLiteral("actionPayload")).toObject();
            const QJsonObject na = payload.value(QStringLiteral("actionPayload")).toObject();
            bool merged = false;
            if (code == QLatin1String("KT") && la.value(QStringLiteral("date")) == na.value(QStringLiteral("date"))) {
                const qint64 sum = qint64(la.value(QStringLiteral("duration")).toDouble())
                    + qint64(na.value(QStringLiteral("duration")).toDouble());
                la.insert(QStringLiteral("duration"), sum);
                QJsonObject change = lp.value(QStringLiteral("entityChanges")).toArray().at(0).toObject();
                QJsonObject cc = change.value(QStringLiteral("changes")).toObject();
                cc.insert(QStringLiteral("duration"), sum);
                change.insert(QStringLiteral("changes"), cc);
                lp.insert(QStringLiteral("entityChanges"), QJsonArray{change});
                merged = true;
            } else if (code == QLatin1String("HU") && !la.contains(QStringLiteral("projectMoveSubTaskIds"))
                       && !na.contains(QStringLiteral("projectMoveSubTaskIds"))) {
                QJsonObject lt = la.value(QStringLiteral("task")).toObject();
                QJsonObject lc = lt.value(QStringLiteral("changes")).toObject();
                const QJsonObject nc = na.value(QStringLiteral("task")).toObject().value(QStringLiteral("changes")).toObject();
                for (auto it = nc.begin(); it != nc.end(); ++it)
                    lc.insert(it.key(), it.value());
                lt.insert(QStringLiteral("changes"), lc);
                la.insert(QStringLiteral("task"), lt);
                merged = true;
            }
            if (merged) {
                lp.insert(QStringLiteral("actionPayload"), la);
                last.insert(QStringLiteral("p"), lp);
                last.insert(QStringLiteral("t"), now);
                m_pending[m_pending.size() - 1] = last;
                save();
                emit changed();
                emit pendingChanged();
                return;
            }
        }
    }
    m_pending.append(op);
    save();
    emit changed();
    emit pendingChanged();
}

QString SpStore::addTask(const QString &title, const QString &projectId, const QStringList &tagIds,
                         const QString &dueDay, const QString &parentId)
{
    const QString id = newTaskId();
    QJsonObject task{
        {"id", id},
        {"title", title},
        {"subTaskIds", QJsonArray()},
        {"timeSpentOnDay", QJsonObject()},
        {"timeSpent", 0},
        {"timeEstimate", 0},
        {"isDone", false},
        {"tagIds", QJsonArray::fromStringList(tagIds)},
        {"created", QDateTime::currentMSecsSinceEpoch()},
        {"attachments", QJsonArray()},
        {"projectId", projectId},
    };
    if (!parentId.isEmpty()) {
        QJsonObject a{{"task", task}, {"parentId", parentId}, {"isIgnoreShortSyntax", true}};
        dispatch(QStringLiteral("TA"), QStringLiteral("CRT"), id, QJsonObject{{"actionPayload", a}, {"entityChanges", QJsonArray()}});
        return id;
    }
    if (!dueDay.isEmpty())
        task.insert(QStringLiteral("dueDay"), dueDay);
    const bool isTag = !tagIds.isEmpty() && projectId.isEmpty();
    QJsonObject a{
        {"task", task},
        {"workContextId", isTag ? tagIds.first() : (dueDay == todayStr() && projectId.isEmpty() ? kToday : projectId)},
        {"workContextType", isTag || projectId.isEmpty() ? "TAG" : "PROJECT"},
        {"isAddToBacklog", false},
        {"isAddToBottom", false},
        {"isIgnoreShortSyntax", true},
    };
    dispatch(QStringLiteral("HA"), QStringLiteral("CRT"), id, QJsonObject{{"actionPayload", a}, {"entityChanges", QJsonArray()}});
    return id;
}

void SpStore::updateTask(const QString &id, const QJsonObject &changes)
{
    if (!hasEntity(m_state, kTask, id) || changes.isEmpty())
        return;
    QJsonObject c = changes;
    if (c.value(QStringLiteral("isDone")).toBool() && !c.contains(QStringLiteral("doneOn")))
        c.insert(QStringLiteral("doneOn"), QDateTime::currentMSecsSinceEpoch());
    QJsonObject a{{"task", QJsonObject{{"id", id}, {"changes", c}}}};
    QStringList moved;
    if (c.contains(QStringLiteral("projectId"))) {
        // Like SP's TaskService.update: name the subtasks that move along, they
        // are part of the op's entity footprint for conflict resolution.
        for (const QString &s : strings(task(id).value(QStringLiteral("subTaskIds"))))
            if (hasEntity(m_state, kTask, s))
                moved.append(s);
        a.insert(QStringLiteral("projectMoveSubTaskIds"), QJsonArray::fromStringList(moved));
    }
    dispatch(QStringLiteral("HU"), QStringLiteral("UPD"), id, QJsonObject{{"actionPayload", a}, {"entityChanges", QJsonArray()}}, moved);
}

void SpStore::deleteTask(const QString &id)
{
    if (!hasEntity(m_state, kTask, id))
        return;
    QJsonObject t = task(id);
    QJsonArray subs;
    for (const QString &s : strings(t.value(QStringLiteral("subTaskIds"))))
        if (hasEntity(m_state, kTask, s))
            subs.append(task(s));
    t.insert(QStringLiteral("subTasks"), subs);
    dispatch(QStringLiteral("HD"), QStringLiteral("DEL"), id,
             QJsonObject{{"actionPayload", QJsonObject{{"task", t}}}, {"entityChanges", QJsonArray()}});
}

void SpStore::addTimeSpent(const QString &taskId, const QString &date, qint64 ms)
{
    if (!hasEntity(m_state, kTask, taskId) || ms <= 0)
        return;
    const QJsonObject a{{"taskId", taskId}, {"date", date}, {"duration", ms}};
    const QJsonObject change{{"entityType", "TASK"}, {"entityId", taskId}, {"opType", "UPD"}, {"changes", a}};
    dispatch(QStringLiteral("KT"), QStringLiteral("UPD"), taskId,
             QJsonObject{{"actionPayload", a}, {"entityChanges", QJsonArray{change}}});
}

void SpStore::dropPending(int count)
{
    QJsonArray rest;
    for (int i = count; i < m_pending.size(); ++i)
        rest.append(m_pending.at(i));
    m_pending = rest;
    save();
    emit pendingChanged();
}

int SpStore::dropPendingIds(const QSet<QString> &ids)
{
    QJsonArray rest;
    for (const QJsonValue &v : std::as_const(m_pending))
        if (!ids.contains(v.toObject().value(QStringLiteral("id")).toString()))
            rest.append(v);
    const int dropped = m_pending.size() - rest.size();
    if (dropped) {
        m_pending = rest;
        save();
        emit pendingChanged();
    }
    return dropped;
}

void SpStore::raiseClock(int atLeast)
{
    if (atLeast > m_clock) {
        m_clock = atLeast;
        save();
    }
}

void SpStore::rebase(const QJsonObject &remoteState)
{
    m_state = remoteState;
    m_synced = true;
    for (const QJsonValue &op : std::as_const(m_pending))
        applyOp(m_state, op.toObject());
    save();
    emit changed();
}

int SpStore::nextClock()
{
    ++m_clock;
    save();
    return m_clock;
}

bool SpStore::save() const
{
    auto write = [this](const QString &name, const QJsonObject &obj) {
        QSaveFile f(m_dataDir + QLatin1Char('/') + name);
        if (!f.open(QIODevice::WriteOnly))
            return false;
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        return f.commit();
    };
    return write(QStringLiteral("sp-state.json"), m_state)
        && write(QStringLiteral("sp-client.json"),
                 QJsonObject{{"clientId", m_clientId}, {"clock", m_clock}, {"synced", m_synced}, {"pending", m_pending}});
}

void SpStore::load()
{
    auto read = [this](const QString &name) {
        QFile f(m_dataDir + QLatin1Char('/') + name);
        return f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()).object() : QJsonObject();
    };
    const QJsonObject client = read(QStringLiteral("sp-client.json"));
    m_clientId = client.value(QStringLiteral("clientId")).toString();
    m_clock = client.value(QStringLiteral("clock")).toInt();
    m_synced = client.value(QStringLiteral("synced")).toBool();
    m_pending = client.value(QStringLiteral("pending")).toArray();
    if (m_clientId.isEmpty()) {
        // SP client ids: [a-zA-Z0-9_-]{5,}; prefix marks the reMarkable.
        m_clientId = QStringLiteral("RM_") + newTaskId().left(10).replace(QLatin1Char('-'), QLatin1Char('x'));
    }
    m_state = read(QStringLiteral("sp-state.json"));
    if (m_state.isEmpty()) {
        m_state = emptyState();
        migrateLegacyTasks();
    }
    save();
}

void SpStore::migrateLegacyTasks()
{
    // v0.1 stored its own tasks.json; turn those tasks into ops once.
    QFile f(m_dataDir + QStringLiteral("/tasks.json"));
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("tasks")).toArray();
    f.close();
    QHash<QString, QString> idMap;
    for (const QJsonValue &v : arr) {
        const QJsonObject t = v.toObject();
        const QString parent = idMap.value(t.value(QStringLiteral("parentId")).toString());
        const QString id = addTask(t.value(QStringLiteral("title")).toString(), parent.isEmpty() ? defaultProjectId() : QString(),
                                   {}, parent.isEmpty() && !t.value(QStringLiteral("isDone")).toBool() ? todayStr() : QString(), parent);
        idMap.insert(t.value(QStringLiteral("id")).toString(), id);
        QJsonObject changes;
        if (t.value(QStringLiteral("isDone")).toBool()) {
            changes.insert(QStringLiteral("isDone"), true);
            if (t.contains(QStringLiteral("doneOn")))
                changes.insert(QStringLiteral("doneOn"), t.value(QStringLiteral("doneOn")));
        }
        if (qint64(t.value(QStringLiteral("timeEstimate")).toDouble()) > 0)
            changes.insert(QStringLiteral("timeEstimate"), t.value(QStringLiteral("timeEstimate")));
        if (!t.value(QStringLiteral("notes")).toString().isEmpty())
            changes.insert(QStringLiteral("notes"), t.value(QStringLiteral("notes")));
        updateTask(id, changes);
        const QJsonObject days = t.value(QStringLiteral("timeSpentOnDay")).toObject();
        for (auto it = days.begin(); it != days.end(); ++it)
            addTimeSpent(id, it.key(), qint64(it.value().toDouble()));
        // keep handwriting: ink files are keyed by task id
        for (const char *kind : {"title", "notes"}) {
            const QString from = QStringLiteral("%1/ink/%2-%3.json").arg(m_dataDir, t.value(QStringLiteral("id")).toString(), QLatin1String(kind));
            QFile::rename(from, QStringLiteral("%1/ink/%2-%3.json").arg(m_dataDir, id, QLatin1String(kind)));
        }
    }
    QFile::rename(f.fileName(), f.fileName() + QStringLiteral(".migrated"));
}
