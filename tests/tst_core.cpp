#include "inkcanvas.h"
#include "spimport.h"
#include "stroke.h"
#include "taskmodel.h"
#include "updater.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class TstCore : public QObject
{
    Q_OBJECT

private slots:
    void strokeRoundTrip()
    {
        Stroke s;
        s.width = 0.01f;
        s.points = {{0.1f, 0.2f, 0.5f}, {0.3f, 0.4f, 1.0f}};
        const QVector<Stroke> back = ink::deserialize(ink::serialize({s}));
        QCOMPARE(back.size(), 1);
        QCOMPARE(back[0].points.size(), 2);
        QVERIFY(qAbs(back[0].points[1].x - 0.3f) < 1e-4);
        QVERIFY(qAbs(back[0].points[0].pressure - 0.5f) < 1e-3);
        QVERIFY(back[0].hitTest(QPointF(0.2, 0.3), 0.01f));
        QVERIFY(!back[0].hitTest(QPointF(0.9, 0.9), 0.01f));
    }

    void canvasDrawEraseUndoSave()
    {
        QTemporaryDir dir;
        InkCanvas c;
        c.setSize(QSizeF(200, 100));
        c.setSource(dir.filePath("ink/a.json"));
        c.beginStroke(10, 10, 0.5);
        c.extendStroke(50, 50, 0.8);
        c.endStroke();
        QCOMPARE(c.strokes().size(), 1);
        QVERIFY(c.save());
        c.eraseAt(30, 30, 5);
        QVERIFY(c.isEmpty());
        c.undo();
        QCOMPARE(c.strokes().size(), 1);
        c.reload();
        QCOMPARE(c.strokes().size(), 1);
    }

    void tasksAndTracking()
    {
        QTemporaryDir dir;
        {
            TaskModel m(dir.path());
            const QString a = m.addTask("Schreiben");
            const QString sub = m.addTask("Gliederung", a);
            QCOMPARE(m.rowCount(), 2);
            QCOMPARE(m.data(m.index(1), TaskModel::IsSubTaskRole).toBool(), true);

            m.toggleTracking(a);
            QCOMPARE(m.currentTaskId(), a);
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            m.commitTracking(now + 5 * 60 * 1000);
            QVERIFY(m.get(a).value("timeSpent").toLongLong() >= 5 * 60 * 1000);

            m.toggleDone(a);
            QVERIFY(m.currentTaskId().isEmpty());
            QCOMPARE(m.openCount(), 1);
            QCOMPARE(m.rowCount(), 1); // only the open subtask is visible
            m.setShowDone(true);
            QCOMPARE(m.rowCount(), 1);
            Q_UNUSED(sub);
        }
        TaskModel reloaded(dir.path());
        QCOMPARE(reloaded.tasks().size(), 2);
        QVERIFY(reloaded.tasks()[0].timeSpent >= 5 * 60 * 1000);
    }

    void versionCompare()
    {
        QVERIFY(Updater::compareVersions("v0.2.0", "0.1.9") > 0);
        QCOMPARE(Updater::compareVersions("v1.0.0", "1.0.0"), 0);
        QVERIFY(Updater::compareVersions("0.1.0", "0.1.1") < 0);
        QVERIFY(Updater::assetName().startsWith("remarkable-sp-"));
    }

    void superProductivityImport()
    {
        const QByteArray backup = R"({
          "data": {
            "task": {
              "ids": ["sub1", "t1", "t2"],
              "entities": {
                "t1": {"id": "t1", "title": "Steuer", "isDone": false, "subTaskIds": ["sub1"],
                       "timeEstimate": 3600000, "timeSpentOnDay": {"2026-09-29": 600000}},
                "sub1": {"id": "sub1", "title": "Belege", "parentId": "t1", "subTaskIds": []},
                "t2": {"id": "t2", "title": "Sport", "isDone": true, "subTaskIds": []}
              }
            },
            "project": {"ids": [], "entities": {}}
          }
        })";
        const spimport::Result r = spimport::parse(backup);
        QVERIFY(r.error.isEmpty());
        QCOMPARE(r.tasks.size(), 3);
        QCOMPARE(r.tasks[0].id, QString("t1"));
        QCOMPARE(r.tasks[1].id, QString("sub1"));
        QCOMPARE(r.tasks[0].timeSpent, 600000);

        QTemporaryDir dir;
        QFile f(dir.filePath("backup.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(backup);
        f.close();
        TaskModel m(dir.filePath("data"));
        QCOMPARE(m.importSuperProductivity(f.fileName()), 3);
        QCOMPARE(m.importSuperProductivity(f.fileName()), 0); // idempotent
        QCOMPARE(m.openCount(), 2);

        QVERIFY(!spimport::parse("{\"foo\": 1}").error.isEmpty());
    }
};

QTEST_MAIN(TstCore)
#include "tst_core.moc"
