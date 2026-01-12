#include "application.h"

#ifndef __APPLE__
#include <QSurfaceFormat>
#endif

#include <csignal>
#include <entryPoint.h>

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

#ifdef __APPLE__
    QQuickWindow::setGraphicsApi(QSGRendererInterface::MetalRhi);
#elif __linux__ or _WIN64
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
#endif

    Application app(argc, argv);

    Application::setApplicationDisplayName(app.getApplicationName());
    Application::setApplicationVersion(app.getApplicationVersion());

    QQuickStyle::setStyle("Universal");

    app.installEventFilter(&app);

    spdlog::info("{} {}", app.getApplicationName().toStdString(), app.getApplicationVersion().toStdString());

    auto entryPoint = new EntryPoint(app.getLoggerName());
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [entryPoint]() { entryPoint->deleteLater(); });

    return Application::exec();
}
void SigintCallbackHandler(const int signum) {
    printf("%s %d", "Приложение остановлено с кодом: ", signum);
    exit(signum);
}
