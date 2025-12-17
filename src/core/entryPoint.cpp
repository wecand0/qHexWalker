#include "entryPoint.h"

#include "h3Model.h"
#include "h3TargetsModel.h"

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

    targetsModel_ = new H3TargetsModel(this);
    engine_->rootContext()->setContextProperty("targetsModel", targetsModel_);


    connect(targetsModel_, &H3TargetsModel::onCompute, h3Model_, &H3Model::requestCells);
}
void EntryPoint::InitMap() {
    mapProvider_ = new MapProvider(this);
    engine_->rootContext()->setContextProperty("mapProvider", mapProvider_);
    logger_->GetLoggerInstance()->info("Map url -> {}", pathUrl_c.toStdString());
    mapProvider_->setUrl(pathUrl_c);
}

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