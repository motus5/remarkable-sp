#include "inkcanvas.h"
#include "taskmodel.h"

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

    QString dataDir = qEnvironmentVariable("RMSP_DATA_DIR");
    if (dataDir.isEmpty())
        dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    qmlRegisterType<InkCanvas>("RemarkableSP.Core", 1, 0, "InkCanvas");

    TaskModel tasks(dataDir);

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
    engine.rootContext()->setContextProperty(QStringLiteral("tasks"), &tasks);
    engine.rootContext()->setContextProperty(QStringLiteral("rmFullscreen"), fullscreen);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/RemarkableSP/qml/Main.qml")));

    return app.exec();
}
