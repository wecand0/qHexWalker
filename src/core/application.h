#pragma once

#include <QGuiApplication>

class Application final : public QGuiApplication {
public:
    Application(int &argc, char **argv);
    [[nodiscard]]
    auto getApplicationName() const noexcept {
        return appName_;
    }
    [[nodiscard]]
    auto getApplicationVersion() const noexcept {
        return appVersion_;
    }
    [[nodiscard]]
    auto getLoggerName() const noexcept {
        return loggerName_;
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    std::string loggerName_;
    QString appName_;
    QString appVersion_;
};
