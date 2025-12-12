//
// Created by user on 02/12/2025.
//

#ifndef Q_HEX_WALKER_ENTRYPOINT_H
#define Q_HEX_WALKER_ENTRYPOINT_H

class QQuickWindow;
class QQmlApplicationEngine;
class MapProvider;

namespace TD {
class Logger;
}

class H3Model;

class EntryPoint final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(EntryPoint)
public:
    explicit EntryPoint(const std::string &loggerName, QObject *parent = nullptr);
    ~EntryPoint() override;

private:
    void InitEngine();
    void InitMap();
    void InitLogger(const std::string &loggerName);
    void InitDataModels();

private:
    QQmlApplicationEngine *engine_{};
    QQuickWindow *rootWindow_{};

    std::unique_ptr<TD::Logger> logger_;
    H3Model *h3Model_{};
    MapProvider *mapProvider_{};
};

#endif  // Q_HEX_WALKER_ENTRYPOINT_H