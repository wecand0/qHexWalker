#include "application.h"
#include "config.h"

Application::Application(int &argc, char **argv) : QGuiApplication(argc, argv) {
    loggerName_ = LOGGER_NAME;
    appName_ = APP_NAME;
    appVersion_ = VERSION;
}

bool Application::eventFilter(QObject *obj, QEvent *event) { return QGuiApplication::eventFilter(obj, event); }
