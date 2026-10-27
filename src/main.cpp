#include "icon.h"
#include "inkcanvas.h"
#include "latency.h"
#include "palmfilter.h"
#include "peninput.h"
#include "spstore.h"
#include "syncengine.h"
#include "updater.h"
#include "workspace.h"

#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QQuickWindow>
#include <QTimer>
#include <QStandardPaths>

#include <memory>

#ifdef Q_OS_UNIX
#include <QSocketNotifier>
#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

// Ctrl+C, kill and a dropped SSH session end the app in order (#14): the
// handler only writes to a socket, the event loop then quits normally, so
// aboutToQuit runs (time tracking, LATENZ-SUMME) and pages save their ink.
static int s_signalFd[2] = {-1, -1};

static void onQuitSignal(int sig)
{
    const char c = char(sig);
    [[maybe_unused]] const ssize_t n = ::write(s_signalFd[0], &c, 1);
}

static void quitOnSignals(QCoreApplication *app)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signalFd) != 0)
        return;
    auto *notifier = new QSocketNotifier(s_signalFd[1], QSocketNotifier::Read, app);
    QObject::connect(notifier, &QSocketNotifier::activated, app, [notifier] {
        char c = 0;
        [[maybe_unused]] const ssize_t n = ::read(s_signalFd[1], &c, 1);
        notifier->setEnabled(false);
        fprintf(stderr, "Signal %d: beende geordnet\n", int(c));
        QCoreApplication::quit();
    });
    struct sigaction sa = {};
    sa.sa_handler = onQuitSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    for (int sig : {SIGINT, SIGTERM, SIGHUP})
        sigaction(sig, &sa, nullptr);
}
#else
static void quitOnSignals(QCoreApplication *) {}
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("remarkable-sp"));
    app.setOrganizationName(QStringLiteral("remarkable-sp"));
    app.setApplicationVersion(QStringLiteral(RMSP_VERSION));
    quitOnSignals(&app);
    // Bundled Noto Sans: the device image ships no fonts for third-party apps.
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/NotoSans-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/NotoSans-Bold.ttf"));
    QFont font(QStringLiteral("Noto Sans"));
    font.setStyleHint(QFont::SansSerif);
    app.setFont(font);

    QString dataDir = qEnvironmentVariable("RMSP_DATA_DIR");
    if (dataDir.isEmpty())
        dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir + QStringLiteral("/ink"));

    qmlRegisterType<InkCanvas>("RemarkableSP.Core", 1, 0, "InkCanvas");
    qmlRegisterType<Icon>("RemarkableSP.Core", 1, 0, "Icon");
    qmlRegisterUncreatableType<TaskListModel>("RemarkableSP.Core", 1, 0, "TaskListModel", QStringLiteral("from app.tasks"));

    SpStore store(dataDir);
    Workspace workspace(&store, dataDir + QStringLiteral("/settings.ini"));
    SyncEngine sync(&store, dataDir + QStringLiteral("/settings.ini"));
    Updater updater;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &workspace, [&] { workspace.stopTracking(); });

    // Desktop platforms get a window, everything else (linuxfb, epaper, eglfs,
    // vnc, ...) is assumed to be the tablet and runs full screen.
    const QString platform = QGuiApplication::platformName();
    const bool desktop = platform == QLatin1String("xcb") || platform == QLatin1String("wayland")
        || platform == QLatin1String("cocoa") || platform == QLatin1String("windows")
        || platform == QLatin1String("offscreen");
    const bool fullscreen = qEnvironmentVariableIsSet("RMSP_FULLSCREEN") || !desktop;
    const bool epaper = platform == QLatin1String("epaper");
    // Device test v0.3.1: ~400 ms ink latency; grey antialiasing pixels force
    // slower grey-level e-ink refreshes. Keep ink black/white on the device.
    if ((epaper && !qEnvironmentVariableIsSet("RMSP_INK_ANTIALIAS")) || qEnvironmentVariableIsSet("RMSP_INK_MONOCHROME"))
        InkCanvas::setMonochrome(true);
    if (epaper) {
        // reMarkable's own scene graph backend (libqsgepaper) drives the e-ink refresh.
        if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))
            QQuickWindow::setSceneGraphBackend(QStringLiteral("epaper"));
    } else if (!desktop || qEnvironmentVariableIsSet("RMSP_SOFTWARE")) {
        // No usable GPU on other embedded platforms: render in software.
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    }

    // The epaper platform plugin handles touch only; read the pen ourselves.
    std::unique_ptr<PenInput> pen;
    const QString replay = qEnvironmentVariable("RMSP_PEN_REPLAY");
    if (epaper || qEnvironmentVariableIsSet("RMSP_PEN_DEVICE") || !replay.isEmpty()) {
        pen = std::make_unique<PenInput>();
        // RMSP_PALM_REJECTION=0 switches it off (for comparison measurements).
        if (qEnvironmentVariable("RMSP_PALM_REJECTION") != QLatin1String("0")) {
            auto *palm = new PalmFilter(&app);
            app.installEventFilter(palm);
            pen->setPalmFilter(palm);
        }
        if (replay.isEmpty())
            pen->open(qEnvironmentVariable("RMSP_PEN_DEVICE"));
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &workspace);
    engine.rootContext()->setContextProperty(QStringLiteral("sync"), &sync);
    engine.rootContext()->setContextProperty(QStringLiteral("updater"), &updater);
    engine.rootContext()->setContextProperty(QStringLiteral("rmFullscreen"), fullscreen);
    engine.rootContext()->setContextProperty(QStringLiteral("rmIgnoresFullScreen"),
                                             epaper || qEnvironmentVariableIsSet("RMSP_SIMULATE_EPAPER"));
    // On-screen keyboard: on the tablet, or on request for testing.
    engine.rootContext()->setContextProperty(QStringLiteral("rmOsk"), fullscreen || qEnvironmentVariableIsSet("RMSP_OSK"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/RemarkableSP/Main.qml")));

    QQuickWindow *rootWindow = engine.rootObjects().isEmpty()
        ? nullptr
        : qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (LatencyProbe *probe = LatencyProbe::instance(); probe && rootWindow)
        probe->attach(rootWindow);

    // Measurement without a device: open a fresh task page and play a
    // synthetic pen stroke (see PenInput::startReplay), then quit.
    if (rootWindow && qEnvironmentVariable("RMSP_OPEN_TASK") == QLatin1String("new"))
        rootWindow->setProperty("openTaskId", workspace.addTask(QString()));
    if (pen && !replay.isEmpty()) {
        QTimer::singleShot(2500, pen.get(), [&pen, replay] { pen->startReplay(replay); });
        QObject::connect(pen.get(), &PenInput::replayFinished, &app, [] {
            QTimer::singleShot(1000, [] { QCoreApplication::quit(); });
        });
    }

    // Self-check for CI and emulation: render once, save the window as PNG,
    // report window and screen size, quit.
    const QString screenshot = qEnvironmentVariable("RMSP_SCREENSHOT");
    if (!screenshot.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        QTimer::singleShot(1500, &app, [window, screenshot] {
            const QImage image = window->grabWindow();
            const QSize screen = window->screen() ? window->screen()->size() : QSize();
            const bool saved = image.save(screenshot);
            printf("RMSP_GEOMETRY window=%dx%d screen=%dx%d image=%dx%d saved=%d\n", window->width(),
                   window->height(), screen.width(), screen.height(), image.width(), image.height(), saved ? 1 : 0);
            fflush(stdout);
            QCoreApplication::exit(saved ? 0 : 2);
        });
    }

    return app.exec();
}
