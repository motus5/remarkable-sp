#include "inkcanvas.h"
#include "palmfilter.h"
#include "peninput.h"
#include "spstore.h"
#include "stroke.h"
#include "syncbackend.h"
#include "syncengine.h"
#include "syncfile.h"
#include "updater.h"
#include "workspace.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>
#include <QPointingDevice>
#include <QRawFont>
#include <QWindow>

namespace {

QJsonObject taskJson(const QString &id, const QString &title, const QString &projectId, const QString &dueDay = {})
{
    QJsonObject t{{"id", id}, {"title", title}, {"projectId", projectId}, {"subTaskIds", QJsonArray()},
                  {"tagIds", QJsonArray()}, {"timeSpentOnDay", QJsonObject()}, {"timeSpent", 0},
                  {"timeEstimate", 0}, {"isDone", false}, {"created", 1}, {"attachments", QJsonArray()},
                  {"futureField", "keep me"}};
    if (!dueDay.isEmpty())
        t.insert("dueDay", dueDay);
    return t;
}

// A sync-data.json as written by SP desktop v19 (trimmed, plus unknown data).
QJsonObject desktopFile()
{
    const QString today = SpStore::todayStr();
    QJsonObject tasks{{"ids", QJsonArray{"d1", "d2"}},
                      {"entities", QJsonObject{{"d1", taskJson("d1", "Desktop task", "INBOX_PROJECT", today)},
                                               {"d2", taskJson("d2", "Work task", "P1")}}},
                      {"currentTaskId", QJsonValue()}};
    QJsonObject inbox{{"id", "INBOX_PROJECT"}, {"title", "Inbox"}, {"taskIds", QJsonArray{"d1"}},
                      {"backlogTaskIds", QJsonArray()}, {"noteIds", QJsonArray()}, {"theme", QJsonObject{{"primary", "#abc"}}}};
    QJsonObject work{{"id", "P1"}, {"title", "Work"}, {"taskIds", QJsonArray{"d2"}}, {"backlogTaskIds", QJsonArray()},
                     {"noteIds", QJsonArray()}};
    QJsonObject today_{{"id", "TODAY"}, {"title", "Today"}, {"taskIds", QJsonArray{"d1"}}};
    QJsonObject urgent{{"id", "EM_URGENT"}, {"title", "Urgent"}, {"taskIds", QJsonArray()}};
    QJsonObject state{
        {"task", tasks},
        {"project", QJsonObject{{"ids", QJsonArray{"INBOX_PROJECT", "P1"}}, {"entities", QJsonObject{{"INBOX_PROJECT", inbox}, {"P1", work}}}}},
        {"tag", QJsonObject{{"ids", QJsonArray{"TODAY", "EM_URGENT"}}, {"entities", QJsonObject{{"TODAY", today_}, {"EM_URGENT", urgent}}}}},
        {"planner", QJsonObject{{"days", QJsonObject()}}},
        {"globalConfig", QJsonObject{{"lang", QJsonObject{{"lng", "de"}}}}},
        {"someFutureSlice", QJsonObject{{"x", 1}}},
    };
    QJsonObject op{{"id", "0199-desktop-op"}, {"a", "HA"}, {"o", "CRT"}, {"e", "TASK"}, {"d", "d1"}, {"p", QJsonObject()},
                   {"c", "E_desk1"}, {"v", QJsonObject{{"E_desk1", 7}}}, {"t", 1}, {"s", 4}, {"sv", 3}};
    return QJsonObject{
        {"version", 2}, {"syncVersion", 3}, {"schemaVersion", 4},
        {"vectorClock", QJsonObject{{"E_desk1", 7}}}, {"lastModified", 1}, {"clientId", "E_desk1"},
        {"state", state},
        {"archiveYoung", QJsonObject{{"task", QJsonObject{{"ids", QJsonArray()}, {"entities", QJsonObject()}}}}},
        {"archiveOld", QJsonObject{{"marker", "old"}}},
        {"recentOps", QJsonArray{op}}, {"oldestOpSyncVersion", 3},
        {"snapshotBaseClock", QJsonObject{{"E_desk1", 5}}},
    };
}

void writeFile(const QString &path, const QByteArray &data)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

QJsonObject readSyncFile(const QString &dir, const QString &password = {})
{
    QFile f(dir + "/sync-data.json");
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return syncfile::decode(f.readAll(), password).json;
}

// Simulates another device writing between our download and upload.
class RacingBackend : public FolderBackend
{
public:
    using FolderBackend::FolderBackend;
    int racesLeft = 1;
    std::function<void()> race;
    PutResult put(const QString &name, const QByteArray &body, const RemoteFile &base, bool createOnly) override
    {
        if (name == QLatin1String("sync-data.json") && racesLeft-- > 0)
            race();
        return FolderBackend::put(name, body, base, createOnly);
    }
};

}

// Counts the touch events that get past the PalmFilter.
class TouchSink : public QWindow
{
public:
    int begins = 0, updates = 0, ends = 0, cancels = 0;
protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::TouchBegin: ++begins; return true;
        case QEvent::TouchUpdate: ++updates; return true;
        case QEvent::TouchEnd: ++ends; return true;
        case QEvent::TouchCancel: ++cancels; return true;
        default: return QWindow::event(e);
        }
    }
};

static void sendTouch(QWindow *w, const QPointingDevice *dev, QEvent::Type type, int id, QEventPoint::State st)
{
    QEventPoint p(id, st, QPointF(10, 10), QPointF(10, 10));
    QTouchEvent e(type, dev, Qt::NoModifier, {p});
    QCoreApplication::sendEvent(w, &e);
}

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
        QVERIFY(c.save());
        c.eraseAt(30, 30, 5);
        QVERIFY(c.isEmpty());
        c.undo();
        QCOMPARE(c.strokes().size(), 1);
        c.reload();
        QCOMPARE(c.strokes().size(), 1);
    }

    void penTransform()
    {
        PenInput pen;
        pen.setRanges(20967, 15725, 4095);
        const QSizeF screen(1404, 1872);
        // default rM mapping: raw Y -> screen X, raw X -> inverted screen Y
        QCOMPARE(pen.mapToScreen(20967, 0, screen), QPointF(0, 0));
        QCOMPARE(pen.mapToScreen(0, 15725, screen), QPointF(1404, 1872));
        pen.setTransform("");
        QCOMPARE(pen.mapToScreen(20967, 15725, screen), QPointF(1404, 1872));
        pen.setTransform("invx");
        QCOMPARE(pen.mapToScreen(0, 0, screen), QPointF(1404, 0));
    }

    void penDeliversTabletEvents()
    {
        struct Window : QWindow {
            QList<QPair<QEvent::Type, QPointingDevice::PointerType>> seen;
            qreal lastPressure = -1;
            void tabletEvent(QTabletEvent *e) override
            {
                seen.append({e->type(), e->pointerType()});
                lastPressure = e->pressure();
                e->accept();
            }
        } w;
        w.resize(200, 200);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        PenInput pen;
        pen.setRanges(1000, 1000, 100);
        auto syn = [&] { pen.feed(0x00, 0, 0); }; // EV_SYN/SYN_REPORT
        pen.feed(0x01, 0x140, 1); pen.feed(0x03, 0x00, 500); pen.feed(0x03, 0x01, 500); syn(); // hover
        pen.feed(0x01, 0x14a, 1); pen.feed(0x03, 0x18, 50); syn();                          // touch
        pen.feed(0x03, 0x00, 400); syn();                                                    // move
        pen.feed(0x01, 0x14a, 0); pen.feed(0x03, 0x18, 0); syn();                           // lift
        pen.feed(0x01, 0x140, 0); syn();                                                     // leave
        pen.feed(0x01, 0x141, 1); pen.feed(0x01, 0x14a, 1); pen.feed(0x03, 0x18, 80); syn(); // eraser down
        QTRY_VERIFY(w.seen.size() >= 4);
        QStringList kinds;
        for (const auto &s : std::as_const(w.seen))
            kinds << QString::number(int(s.first));
        QVERIFY2(kinds.contains(QString::number(int(QEvent::TabletPress))), qPrintable(kinds.join(',')));
        QVERIFY(kinds.contains(QString::number(int(QEvent::TabletRelease))));
        QCOMPARE(w.seen.last().second, QPointingDevice::PointerType::Eraser);
        QVERIFY(qAbs(w.lastPressure - 0.8) < 0.01);
    }

    // T2: the title shown is the title stored and synced (#5).
    void titleModesExclusive()
    {
        // typed title wins, also when handwriting exists
        QVERIFY(Workspace::titleIsTyped("Angebot"));
        QCOMPARE(Workspace::storedTitle("Angebot", false), QString("Angebot"));
        QCOMPARE(Workspace::storedTitle("  Angebot ", true), QString("Angebot"));
        // no typed text: the handwriting is the title, SP gets the marker
        QVERIFY(!Workspace::titleIsTyped("   "));
        QCOMPARE(Workspace::storedTitle("", false), Workspace::kHandwritingTitle);
        // neither: empty title
        QCOMPARE(Workspace::storedTitle("", true), QString());
    }

    // T4: with monochrome ink the canvas holds only fully transparent or
    // fully black pixels (no antialiasing greys).
    void inkIsMonochrome()
    {
        InkCanvas::setMonochrome(true);
        InkCanvas c;
        c.setSize(QSizeF(300, 300));
        c.setPenWidth(6);
        c.beginStroke(20, 20, 0.5);
        for (int i = 1; i <= 40; ++i)
            c.extendStroke(20 + i * 6, 20 + i * 3.3, 0.3 + i / 80.0);
        c.endStroke();
        InkCanvas::setMonochrome(false);
        const QImage img = c.buffer().convertToFormat(QImage::Format_ARGB32);
        int black = 0, other = 0;
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x) {
                const QRgb p = img.pixel(x, y);
                if (qAlpha(p) == 0)
                    continue;
                if (qAlpha(p) == 255 && qRed(p) == 0 && qGreen(p) == 0 && qBlue(p) == 0)
                    ++black;
                else
                    ++other;
            }
        QVERIFY(black > 500);
        QCOMPARE(other, 0);
    }

    // T5: cost of painting one pen sample on a page-sized canvas (regression
    // watch; run with -tickcounter or compare the reported msecs per iteration).
    void inkPaintCost()
    {
        InkCanvas c;
        c.setSize(QSizeF(1300, 1500));
        c.beginStroke(100, 100, 0.6);
        qreal x = 100;
        QBENCHMARK {
            x = x > 1200 ? 100 : x + 3;
            c.extendStroke(x, 100 + (int(x) % 50), 0.7);
        }
        c.endStroke();
        QVERIFY(!c.isEmpty());
    }

    // #12: undo, redo and eraser repaint only the changed area, and the
    // result is pixel-identical to a full repaint.
    void inkPartialRepaint()
    {
        for (bool mono : {true, false}) {
            InkCanvas::setMonochrome(mono);
            InkCanvas c;
            c.setSize(QSizeF(1000, 1200));
            // 40 overlapping zig-zag strokes over the page
            for (int k = 0; k < 40; ++k) {
                c.beginStroke(50 + k * 20, 100 + k * 25, 0.6);
                for (int i = 1; i < 30; ++i)
                    c.extendStroke(50 + k * 20 + i * 8, 100 + k * 25 + (i % 2) * 30, 0.4 + (i % 5) * 0.1);
                c.endStroke();
            }
            // The repainted area must equal a repaint of everything there.
            auto fullRepaint = [&] {
                const QRect r = c.lastRepaint();
                QCOMPARE(c.buffer().copy(r), c.renderAll().copy(r));
            };
            const QRect page(0, 0, 1000, 1200);
            c.undo();
            QVERIFY(!c.lastRepaint().isEmpty());
            QVERIFY(c.lastRepaint().width() * c.lastRepaint().height() < page.width() * page.height() / 10);
            fullRepaint();
            c.undo();
            c.redo();
            fullRepaint();
            c.eraseAt(50 + 10 * 20 + 40, 100 + 10 * 25, 6);
            QCOMPARE(c.strokes().size(), 38);
            QVERIFY(c.lastRepaint().width() * c.lastRepaint().height() < page.width() * page.height() / 4);
            fullRepaint();
            c.undo(); // brings the erased stroke back
            fullRepaint();
        }
        InkCanvas::setMonochrome(false);
    }

    // T7 (#12): time of one undo on a full page (compare with v0.3.2:
    // full repaint). Reported in ms per undo.
    void inkUndoCost()
    {
        InkCanvas c;
        c.setSize(QSizeF(1300, 1500));
        for (int k = 0; k < 120; ++k) {
            c.beginStroke(40 + (k % 12) * 100, 60 + (k / 12) * 140, 0.6);
            for (int i = 1; i < 60; ++i)
                c.extendStroke(40 + (k % 12) * 100 + i * 1.5, 60 + (k / 12) * 140 + (i % 7) * 9, 0.6);
            c.endStroke();
        }
        QElapsedTimer t;
        t.start();
        for (int i = 0; i < 20; ++i)
            c.undo();
        const double ms = t.nsecsElapsed() / 1e6 / 20;
        t.restart();
        const QImage all = c.renderAll(); // what every undo did up to v0.3.2
        const double fullMs = t.nsecsElapsed() / 1e6;
        qInfo("T7 Rückgängig: %.2f ms je Schritt, volles Neuzeichnen (bis v0.3.2) %.2f ms, 100 Striche (Desktop)",
              ms, fullMs);
        QVERIFY(!all.isNull());
        QCOMPARE(c.strokes().size(), 100);
    }

    // #13: every non-ASCII character in the QML and C++ sources must exist in
    // the bundled font; the device has no fallback fonts (⇧ and ⌫ were empty
    // boxes on the rM1 while the desktop drew them from system fonts).
    void glyphsInBundledFont()
    {
        const QString root = QStringLiteral(RMSP_SOURCE_DIR);
        for (const QString &fontFile : {QStringLiteral("NotoSans-Regular.ttf"), QStringLiteral("NotoSans-Bold.ttf")}) {
            const QRawFont font(root + QStringLiteral("/third_party/fonts/") + fontFile, 12);
            QVERIFY2(font.isValid(), qPrintable(fontFile));
            QStringList missing;
            for (const QString &dir : {QStringLiteral("/qml"), QStringLiteral("/src")}) {
                const QDir d(root + dir);
                for (const QString &name : d.entryList({QStringLiteral("*.qml"), QStringLiteral("*.cpp"), QStringLiteral("*.h")})) {
                    QFile f(d.filePath(name));
                    QVERIFY(f.open(QIODevice::ReadOnly));
                    // Lines marked "font-check: data" hold data values that are never shown.
                    QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
                    lines.removeIf([](const QString &l) { return l.contains(QLatin1String("font-check: data")); });
                    const QList<uint> cps = lines.join(QLatin1Char('\n')).toUcs4();
                    for (uint cp : cps)
                        if (cp > 0x7f && !font.supportsCharacter(cp))
                            missing << QStringLiteral("%1 U+%2 in %3").arg(QString::fromUcs4(reinterpret_cast<const char32_t *>(&cp), 1))
                                                                    .arg(cp, 4, 16, QLatin1Char('0')).arg(name);
                }
            }
            missing.removeDuplicates();
            QVERIFY2(missing.isEmpty(), qPrintable(fontFile + QStringLiteral(": ") + missing.join(QStringLiteral(", "))));
        }
    }

    // #14: settings.ini is written the moment a setting changes (and only
    // then: before the first change there is no file, which is expected).
    void settingsWrittenImmediately()
    {
        QTemporaryDir data;
        const QString ini = data.filePath("settings.ini");
        SpStore store(data.path());
        Workspace ws(&store, ini);
        QVERIFY(!QFile::exists(ini));
        ws.setPenWidth(ws.penWidth() == 3 ? 1 : 3);
        QVERIFY(QFile::exists(ini));
        QFile f(ini);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(f.readAll().contains("penWidth="));
    }

    // #11: touch is dropped while the pen is near, a held touch is cancelled
    // when the pen comes, and a dropped touch stays dropped until released.
    void palmRejection()
    {
        QPointingDevice dev(QStringLiteral("test touch"), 77, QInputDevice::DeviceType::TouchScreen,
                            QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 5, 0);
        TouchSink w;
        PalmFilter f;
        w.installEventFilter(&f);
        using S = QEventPoint::State;

        // Pen away: a tap passes.
        sendTouch(&w, &dev, QEvent::TouchBegin, 1, S::Pressed);
        sendTouch(&w, &dev, QEvent::TouchEnd, 1, S::Released);
        QCOMPARE(w.begins, 1);
        QCOMPARE(w.ends, 1);

        // Palm already resting, then the pen comes: Qt Quick gets a cancel,
        // the rest of that touch is dropped.
        sendTouch(&w, &dev, QEvent::TouchBegin, 2, S::Pressed);
        QCOMPARE(w.begins, 2);
        f.setPenNear(true);
        QCOMPARE(w.cancels, 1);
        sendTouch(&w, &dev, QEvent::TouchUpdate, 2, S::Updated);
        QCOMPARE(w.updates, 0);

        // Palm lands while writing: dropped.
        sendTouch(&w, &dev, QEvent::TouchBegin, 3, S::Pressed);
        QCOMPARE(w.begins, 2);

        // Pen leaves: still dropped during the grace time ...
        f.setPenNear(false);
        QVERIFY(f.penBlocking());
        sendTouch(&w, &dev, QEvent::TouchUpdate, 3, S::Updated);
        QCOMPARE(w.updates, 0);
        // ... and the dropped touch stays dropped after it, until released.
        QTest::qWait(PalmFilter::kGraceMs + 50);
        QVERIFY(!f.penBlocking());
        sendTouch(&w, &dev, QEvent::TouchUpdate, 3, S::Updated);
        sendTouch(&w, &dev, QEvent::TouchEnd, 3, S::Released);
        sendTouch(&w, &dev, QEvent::TouchEnd, 2, S::Released);
        QCOMPARE(w.updates, 0);
        QCOMPARE(w.ends, 1);

        // A new touch afterwards passes again.
        sendTouch(&w, &dev, QEvent::TouchBegin, 4, S::Pressed);
        QCOMPARE(w.begins, 3);
        QVERIFY(f.droppedEvents() >= 5);
    }

    void versionCompare()
    {
        QVERIFY(Updater::compareVersions("v0.2.0", "0.1.9") > 0);
        QCOMPARE(Updater::compareVersions("v1.0.0", "1.0.0"), 0);
        QVERIFY(Updater::assetName().startsWith("remarkable-sp-"));
    }

    void codec()
    {
        const QJsonObject obj{{"version", 2}, {"text", "Grüße"}};
        bool ok = false;
        QCOMPARE(syncfile::gunzip(syncfile::gzip("hello"), &ok), QByteArray("hello"));
        QVERIFY(ok);

        const QByteArray plain = syncfile::encode(obj, 2, false, {});
        QVERIFY(plain.startsWith("pf_2__{"));
        QCOMPARE(syncfile::decode(plain).json, obj);

        const QByteArray gz = syncfile::encode(obj, 2, true, {});
        QVERIFY(gz.startsWith("pf_C2__H4sI")); // base64 of the gzip magic
        const auto d = syncfile::decode(gz);
        QVERIFY(d.compressed);
        QCOMPARE(d.json, obj);

        QVERIFY(!syncfile::decode("pf_X9__{}").error.isEmpty());
        QVERIFY(syncfile::decode("not json").json.isEmpty());
    }

    void codecEncrypted()
    {
        if (!syncfile::encryptionSupported())
            QSKIP("built without crypto");
        const QJsonObject obj{{"version", 2}, {"a", 1}};
        const QByteArray enc = syncfile::encode(obj, 2, true, "geheim");
        QVERIFY(enc.startsWith("pf_CE2__"));
        QCOMPARE(syncfile::decode(enc, "geheim").json, obj);
        QVERIFY(syncfile::decode(enc, "falsch").needsPassword);
        QVERIFY(syncfile::decode(enc).needsPassword);
    }

    void decryptsFileWrittenBySp()
    {
        if (!syncfile::encryptionSupported())
            QSKIP("built without crypto");
        // Produced by Super Productivity's own encrypt()+gzip code (password "geheim").
        QFile f(QFINDTESTDATA("fixtures/sp-encrypted-by-sp.json"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const auto d = syncfile::decode(f.readAll(), "geheim");
        QVERIFY2(d.error.isEmpty(), qPrintable(d.error));
        QVERIFY(d.compressed && d.encrypted);
        QCOMPARE(d.json.value("version").toInt(), 2);
        QVERIFY(d.json.value("state").toObject().contains("task"));
    }

    void storeOps()
    {
        QTemporaryDir dir;
        SpStore s(dir.path());
        const QString today = SpStore::todayStr();
        const QString a = s.addTask("A", SpStore::kInbox, {}, today);
        QVERIFY(s.entity("project", SpStore::kInbox).value("taskIds").toArray().contains(a));
        QVERIFY(s.entity("tag", SpStore::kToday).value("taskIds").toArray().contains(a));
        QVERIFY(s.isInToday(s.task(a)));

        const QString sub = s.addTask("A.1", {}, {}, {}, a);
        QCOMPARE(s.task(sub).value("parentId").toString(), a);
        QCOMPARE(s.task(sub).value("projectId").toString(), SpStore::kInbox);
        QVERIFY(s.task(a).value("subTaskIds").toArray().contains(sub));

        // time on a subtask also counts for the parent; minute ticks coalesce
        s.addTimeSpent(sub, today, 60000);
        s.addTimeSpent(sub, today, 60000);
        QCOMPARE(qint64(s.task(sub).value("timeSpent").toDouble()), 120000);
        QCOMPARE(qint64(s.task(a).value("timeSpent").toDouble()), 120000);
        const QJsonArray pending = s.pendingOps();
        QCOMPARE(pending.size(), 3); // HA, TA, one merged KT
        const QJsonObject kt = pending.last().toObject();
        QCOMPARE(kt.value("a").toString(), QString("KT"));
        QCOMPARE(kt.value("p").toObject().value("actionPayload").toObject().value("duration").toInt(), 120000);
        QCOMPARE(kt.value("s").toInt(), 4);
        QVERIFY(kt.value("id").toString().at(14) == QLatin1Char('7')); // UUIDv7

        s.updateTask(a, {{"isDone", true}});
        QVERIFY(s.task(a).value("isDone").toBool());
        QVERIFY(s.task(a).value("doneOn").toDouble() > 0);

        s.deleteTask(a);
        QVERIFY(s.task(a).isEmpty());
        QVERIFY(s.task(sub).isEmpty());
        QVERIFY(!s.entity("project", SpStore::kInbox).value("taskIds").toArray().contains(a));

        // persisted across restarts, client id is stable
        const QString client = s.clientId();
        SpStore again(dir.path());
        QCOMPARE(again.clientId(), client);
        QCOMPARE(again.pendingCount(), s.pendingCount());
    }

    void syncRoundTrip()
    {
        QTemporaryDir data, remote;
        writeFile(remote.filePath("sync-data.json"), syncfile::encode(desktopFile(), 2, false, {}));

        SpStore store(data.path());
        SyncEngine engine(&store, data.filePath("settings.ini"));
        FolderBackend backend(remote.path());
        engine.setBackendOverride(&backend);

        // 1) download only
        QVERIFY2(engine.syncNow(), qPrintable(engine.status()));
        QCOMPARE(store.task("d1").value("title").toString(), QString("Desktop task"));
        QCOMPARE(store.state().value("someFutureSlice").toObject().value("x").toInt(), 1);
        Workspace ws(&store);
        QCOMPARE(ws.tasks()->rowCount(), 1); // Today: d1 only
        ws.openContext("PROJECT", "P1");
        QCOMPARE(ws.tasks()->rowCount(), 1);

        // 2) local changes: new task, done, time, move project
        ws.openContext("TAG", "TODAY");
        const QString mine = ws.addTask("From tablet");
        ws.toggleDone("d1");
        store.addTimeSpent("d2", SpStore::todayStr(), 15 * 60000);
        const QString mineSub = ws.addSubTask(mine, "Unteraufgabe");
        ws.setProject(mine, "P1");
        QCOMPARE(store.task(mineSub).value("projectId").toString(), QString("P1"));
        const QJsonObject moveOp = store.pendingOps().last().toObject();
        QCOMPARE(moveOp.value("p").toObject().value("actionPayload").toObject().value("projectMoveSubTaskIds").toArray(), QJsonArray{mineSub});
        QCOMPARE(moveOp.value("ds").toArray(), (QJsonArray{mine, mineSub}));
        QVERIFY(store.pendingCount() >= 3);
        QVERIFY2(engine.syncNow(), qPrintable(engine.status()));
        QCOMPARE(store.pendingCount(), 0);

        const QJsonObject file = readSyncFile(remote.path());
        QCOMPARE(file.value("version").toInt(), 2);
        QCOMPARE(file.value("syncVersion").toInt(), 4);
        QCOMPARE(file.value("clientId").toString(), store.clientId());
        QCOMPARE(file.value("archiveOld").toObject().value("marker").toString(), QString("old"));
        QCOMPARE(file.value("snapshotBaseClock").toObject().value("E_desk1").toInt(), 5);
        const QJsonObject clock = file.value("vectorClock").toObject();
        QCOMPARE(clock.value("E_desk1").toInt(), 7);
        QVERIFY(clock.value(store.clientId()).toInt() >= 3);
        const QJsonArray ops = file.value("recentOps").toArray();
        QCOMPARE(ops.first().toObject().value("id").toString(), QString("0199-desktop-op"));
        for (int i = 1; i < ops.size(); ++i) {
            const QJsonObject op = ops[i].toObject();
            QCOMPARE(op.value("sv").toInt(), 4);
            QCOMPARE(op.value("c").toString(), store.clientId());
            QCOMPARE(op.value("v").toObject().value("E_desk1").toInt(), 7);
        }
        const QJsonObject st = file.value("state").toObject();
        const QJsonObject d1 = st.value("task").toObject().value("entities").toObject().value("d1").toObject();
        QVERIFY(d1.value("isDone").toBool());
        QCOMPARE(d1.value("futureField").toString(), QString("keep me"));
        const QJsonObject d2 = st.value("task").toObject().value("entities").toObject().value("d2").toObject();
        QCOMPARE(qint64(d2.value("timeSpent").toDouble()), 15 * 60000);
        const QJsonObject p1 = st.value("project").toObject().value("entities").toObject().value("P1").toObject();
        QVERIFY(p1.value("taskIds").toArray().contains(mine));
        QCOMPARE(st.value("globalConfig").toObject().value("lang").toObject().value("lng").toString(), QString("de"));
        QVERIFY(QFile::exists(remote.filePath("sync-data.json.bak")));

        // Hand the produced files to the interop check against SP's own code.
        const QString dump = qEnvironmentVariable("RMSP_INTEROP_DIR");
        if (!dump.isEmpty()) {
            QDir().mkpath(dump);
            QFile::remove(dump + "/before.json");
            QFile::remove(dump + "/after-plain.json");
            QFile::remove(dump + "/after-encrypted.json");
            writeFile(dump + "/before.json", QJsonDocument(desktopFile()).toJson());
            QFile::copy(remote.filePath("sync-data.json"), dump + "/after-plain.json");
            if (syncfile::encryptionSupported())
                writeFile(dump + "/after-encrypted.json", syncfile::encode(file, 2, true, "geheim"));
        }
    }

    void syncConflictRetries()
    {
        QTemporaryDir data, remote;
        writeFile(remote.filePath("sync-data.json"), syncfile::encode(desktopFile(), 2, true, {}));
        SpStore store(data.path());
        SyncEngine engine(&store, data.filePath("settings.ini"));
        RacingBackend backend(remote.path());
        engine.setBackendOverride(&backend);
        QVERIFY(engine.syncNow());

        store.addTimeSpent("d2", SpStore::todayStr(), 60000);
        // The desktop writes a new version (renamed d1) right before our upload.
        backend.race = [&] {
            QJsonObject f = desktopFile();
            QJsonObject st = f.value("state").toObject();
            QJsonObject tasks = st.value("task").toObject();
            QJsonObject ents = tasks.value("entities").toObject();
            QJsonObject d1 = ents.value("d1").toObject();
            d1.insert("title", "Renamed on desktop");
            ents.insert("d1", d1);
            tasks.insert("entities", ents);
            st.insert("task", tasks);
            f.insert("state", st);
            f.insert("syncVersion", 5);
            writeFile(remote.filePath("sync-data.json"), syncfile::encode(f, 2, true, {}));
        };
        QVERIFY2(engine.syncNow(), qPrintable(engine.status()));
        const QJsonObject file = readSyncFile(remote.path());
        QCOMPARE(file.value("syncVersion").toInt(), 6); // built on top of the racing write
        const QJsonObject ents = file.value("state").toObject().value("task").toObject().value("entities").toObject();
        QCOMPARE(ents.value("d1").toObject().value("title").toString(), QString("Renamed on desktop"));
        QCOMPARE(qint64(ents.value("d2").toObject().value("timeSpent").toDouble()), 60000);
        QCOMPARE(store.task("d1").value("title").toString(), QString("Renamed on desktop"));
    }

    void syncRefusesUnsupported()
    {
        QTemporaryDir data, remote;
        SpStore store(data.path());
        SyncEngine engine(&store, data.filePath("settings.ini"));
        FolderBackend backend(remote.path());
        engine.setBackendOverride(&backend);
        QVERIFY(!engine.syncNow()); // empty folder: never create SP data ourselves
        QVERIFY(!QFile::exists(remote.filePath("sync-data.json")));

        QJsonObject f = desktopFile();
        f.insert("schemaVersion", 5);
        writeFile(remote.filePath("sync-data.json"), syncfile::encode(f, 2, false, {}));
        QSignalSpy newer(&engine, &SyncEngine::remoteNewer);
        QVERIFY(!engine.syncNow());
        QCOMPARE(newer.count(), 1);
    }
};

QTEST_MAIN(TstCore)
#include "tst_core.moc"
