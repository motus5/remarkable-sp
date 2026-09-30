#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>

// Field names mirror Super Productivity's task entity so data can be imported
// (and later synced) without translation tables.
struct Task {
    QString id;
    QString title;
    QString notes;
    QString projectId;
    QString parentId;
    QStringList subTaskIds;
    bool isDone = false;
    qint64 created = 0;      // ms since epoch
    qint64 doneOn = 0;       // ms since epoch, 0 = not done
    qint64 timeEstimate = 0; // ms
    qint64 timeSpent = 0;    // ms, sum of timeSpentOnDay
    QMap<QString, qint64> timeSpentOnDay; // "yyyy-MM-dd" -> ms

    QJsonObject toJson() const;
    static Task fromJson(const QJsonObject &o);
    void recalcTimeSpent();
};
