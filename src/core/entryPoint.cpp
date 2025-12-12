// Precompiled header must go first
#include "pch.h"

#include "entryPoint.h"
#include "h3Model.h"
#include "logger.h"
#include "mapProvider.h"

EntryPoint::EntryPoint(const std::string &loggerName, QObject *parent) : QObject(parent) {
    InitLogger(loggerName);

    engine_ = new QQmlApplicationEngine(this);

    InitDataModels();
    InitMap();
    InitEngine();

    connect(engine_, &QQmlApplicationEngine::quit, &QGuiApplication::quit);
}

EntryPoint::~EntryPoint() = default;

void EntryPoint::InitEngine() {
    const QUrl url("qrc:/QHexWalker/ui/main.qml");
    connect(
        engine_, &QQmlApplicationEngine::objectCreated, this,
        [this, url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
            if (url == objUrl) {
                rootWindow_ = qobject_cast<QQuickWindow *>(obj);
                if (!rootWindow_) {
                    qWarning() << "QQuickWindow is not root-object!";
                }
            }
        },
        Qt::QueuedConnection);

    engine_->load(url);
}
void EntryPoint::InitMap() {
    mapProvider_ = new MapProvider(this);
    engine_->rootContext()->setContextProperty("mapProvider", mapProvider_);
    const QString pathUrl = "https://api.maptiler.com/maps/base-v4/style.json?key=bFpEhpcbtSI3j1gzj2Is";
    logger_->GetLoggerInstance()->info("pathToMap: {}", pathUrl.toStdString());
    mapProvider_->setUrl(pathUrl);
}
void EntryPoint::InitLogger(const std::string &loggerName) {
    logger_ = std::make_unique<TD::Logger>(loggerName);
    try {
        logger_->Init();
    } catch (const std::exception &e) {
        printf("%s", e.what());
    }
}
void EntryPoint::InitDataModels() {
    h3Model_ = new H3Model(this);
    engine_->rootContext()->setContextProperty("h3Model", h3Model_);
    try {
        h3Model_->Init();
    } catch (const std::exception &e) {
        spdlog::critical(e.what());
    }
}