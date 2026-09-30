#include "inkcanvas.h"
#include "spstore.h"
#include "syncengine.h"
#include "updater.h"
#include "workspace.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("remarkable-sp"));
    app.setOrganizationName(QStringLiteral("remarkable-sp"));
    app.setApplicationVersion(QStringLiteral(RMSP_VERSION));

    QString dataDir = qEnvironmentVariable("RMSP_DATA_DIR");
    if (dataDir.isEmpty())
        dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir + QStringLiteral("/ink"));

    qmlRegisterType<InkCanvas>("RemarkableSP.Core", 1, 0, "InkCanvas");
    qmlRegisterUncreatableType<TaskListModel>("RemarkableSP.Core", 1, 0, "TaskListModel", QStringLiteral("from app.tasks"));

    SpStore store(dataDir);
    Workspace workspace(&store);
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
    // The tablets have no usable GPU for Qt Quick: render in software.
    if (!desktop || qEnvironmentVariableIsSet("RMSP_SOFTWARE"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &workspace);
    engine.rootContext()->setContextProperty(QStringLiteral("sync"), &sync);
    engine.rootContext()->setContextProperty(QStringLiteral("updater"), &updater);
    engine.rootContext()->setContextProperty(QStringLiteral("rmFullscreen"), fullscreen);
    // On-screen keyboard: on the tablet, or on request for testing.
    engine.rootContext()->setContextProperty(QStringLiteral("rmOsk"), fullscreen || qEnvironmentVariableIsSet("RMSP_OSK"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/RemarkableSP/qml/Main.qml")));

    return app.exec();
}
