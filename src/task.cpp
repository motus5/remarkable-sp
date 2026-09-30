#include "task.h"

#include <QJsonArray>

QJsonObject Task::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("id"), id);
    o.insert(QStringLiteral("title"), title);
    o.insert(QStringLiteral("notes"), notes);
    o.insert(QStringLiteral("projectId"), projectId.isEmpty() ? QJsonValue() : QJsonValue(projectId));
    o.insert(QStringLiteral("parentId"), parentId.isEmpty() ? QJsonValue() : QJsonValue(parentId));
    o.insert(QStringLiteral("subTaskIds"), QJsonArray::fromStringList(subTaskIds));
    o.insert(QStringLiteral("isDone"), isDone);
    o.insert(QStringLiteral("created"), created);
    if (doneOn)
        o.insert(QStringLiteral("doneOn"), doneOn);
    o.insert(QStringLiteral("timeEstimate"), timeEstimate);
    o.insert(QStringLiteral("timeSpent"), timeSpent);
    QJsonObject days;
    for (auto it = timeSpentOnDay.cbegin(); it != timeSpentOnDay.cend(); ++it)
        days.insert(it.key(), it.value());
    o.insert(QStringLiteral("timeSpentOnDay"), days);
    return o;
}

Task Task::fromJson(const QJsonObject &o)
{
    Task t;
    t.id = o.value(QStringLiteral("id")).toString();
    t.title = o.value(QStringLiteral("title")).toString();
    t.notes = o.value(QStringLiteral("notes")).toString();
    t.projectId = o.value(QStringLiteral("projectId")).toString();
    t.parentId = o.value(QStringLiteral("parentId")).toString();
    for (const QJsonValue &v : o.value(QStringLiteral("subTaskIds")).toArray())
        t.subTaskIds.append(v.toString());
    t.isDone = o.value(QStringLiteral("isDone")).toBool();
    t.created = qint64(o.value(QStringLiteral("created")).toDouble());
    t.doneOn = qint64(o.value(QStringLiteral("doneOn")).toDouble());
    t.timeEstimate = qint64(o.value(QStringLiteral("timeEstimate")).toDouble());
    const QJsonObject days = o.value(QStringLiteral("timeSpentOnDay")).toObject();
    for (auto it = days.begin(); it != days.end(); ++it)
        t.timeSpentOnDay.insert(it.key(), qint64(it.value().toDouble()));
    t.timeSpent = qint64(o.value(QStringLiteral("timeSpent")).toDouble());
    if (!t.timeSpentOnDay.isEmpty())
        t.recalcTimeSpent();
    return t;
}

void Task::recalcTimeSpent()
{
    qint64 sum = 0;
    for (qint64 v : std::as_const(timeSpentOnDay))
        sum += v;
    timeSpent = sum;
}
