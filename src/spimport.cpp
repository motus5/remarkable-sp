#include "spimport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace spimport {

static QJsonObject findRoot(const QJsonObject &doc)
{
    if (doc.value(QStringLiteral("task")).isObject())
        return doc;
    const QJsonObject data = doc.value(QStringLiteral("data")).toObject();
    if (data.value(QStringLiteral("task")).isObject())
        return data;
    return {};
}

Result parse(const QByteArray &json)
{
    Result r;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        r.error = QStringLiteral("Keine gültige JSON-Datei: %1").arg(err.errorString());
        return r;
    }
    const QJsonObject root = findRoot(doc.object());
    if (root.isEmpty()) {
        r.error = QStringLiteral("Kein Super-Productivity-Backup (Feld \"task\" fehlt).");
        return r;
    }

    const QJsonObject taskState = root.value(QStringLiteral("task")).toObject();
    const QJsonObject entities = taskState.value(QStringLiteral("entities")).toObject();
    QStringList order;
    for (const QJsonValue &v : taskState.value(QStringLiteral("ids")).toArray())
        order.append(v.toString());
    if (order.isEmpty())
        order = entities.keys();

    auto load = [&](const QString &id) { return Task::fromJson(entities.value(id).toObject()); };

    for (const QString &id : std::as_const(order)) {
        if (!entities.contains(id))
            continue;
        const Task t = load(id);
        if (!t.parentId.isEmpty() && entities.contains(t.parentId))
            continue; // emitted right after its parent below
        r.tasks.append(t);
        for (const QString &subId : t.subTaskIds) {
            if (entities.contains(subId))
                r.tasks.append(load(subId));
        }
    }
    return r;
}

Result parseFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        Result r;
        r.error = QStringLiteral("Datei kann nicht geöffnet werden: %1").arg(path);
        return r;
    }
    return parse(f.readAll());
}

}
