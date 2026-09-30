#pragma once

#include "task.h"

#include <QString>
#include <QVector>

// Reads a Super Productivity backup ("Settings -> Sync & Export -> Export data",
// a JSON file). Both the plain layout ({ task: {ids, entities}, ... }) and the
// wrapped layout ({ data: { task: ... } }) are accepted. Archived tasks are
// skipped; only the active task list is imported.
namespace spimport {

struct Result {
    QVector<Task> tasks; // parents are always followed by their subtasks
    QString error;
};

Result parse(const QByteArray &json);
Result parseFile(const QString &path);

}
