#include "application.h"

#include <QtQuick/QSGRendererInterface>

#ifndef __APPLE__
#include <QSurfaceFormat>
#endif

#include <csignal>
#include <entryPoint.h>

#include <QMapLibre/Utils>

void SigintCallbackHandler(int signum);

int main(int argc, char *argv[]) {
    signal(SIGINT, &SigintCallbackHandler);
    signal(SIGILL, &SigintCallbackHandler);
    signal(SIGFPE, &SigintCallbackHandler);
    signal(SIGSEGV, &SigintCallbackHandler);
    signal(SIGTERM, &SigintCallbackHandler);
    signal(SIGABRT, &SigintCallbackHandler);

    qputenv("QSG_RENDER_LOOP", "threaded");
    qputenv("QML_DISK_CACHE", "aot");

    const QMapLibre::RendererType rendererType = QMapLibre::supportedRendererType();
    const auto graphicsApi = static_cast<QSGRendererInterface::GraphicsApi>(rendererType);
    QQuickWindow::setGraphicsApi(graphicsApi);

    if (graphicsApi == QSGRendererInterface::OpenGLRhi) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
        QSurfaceFormat format;
        format.setProfile(QSurfaceFormat::CoreProfile);        // Только Core Profile (без устаревшего кода)
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);  // Двойная буферизация
        format.setSwapInterval(0);       // Отключение VSync для максимального FPS (если не нужна синхронизация)
        format.setDepthBufferSize(24);   // 24-битный буфер глубины (стандарт)
        format.setStencilBufferSize(8);  // 8-битный буфер трафарета
        format.setOption(QSurfaceFormat::DeprecatedFunctions, false);  // Отключить устаревшие функции
        format.setOption(QSurfaceFormat::DebugContext, false);         // Отключить отладочный контекст (если не нужно)
        QSurfaceFormat::setDefaultFormat(format);
    }

    Application app(argc, argv);

    Application::setApplicationDisplayName(app.getApplicationName());
    Application::setApplicationVersion(app.getApplicationVersion());

    QQuickStyle::setStyle("Universal");

    app.installEventFilter(&app);

    spdlog::info("{} {}", app.getApplicationName().toStdString(), app.getApplicationVersion().toStdString());

    auto entryPoint = new EntryPoint(app.getLoggerName());
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [entryPoint] { entryPoint->deleteLater(); });

    return Application::exec();
}
void SigintCallbackHandler(const int signum) {
    printf("%s %d", "Приложение остановлено с кодом: ", signum);
    exit(signum);
}
