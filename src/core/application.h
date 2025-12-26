/**
 * @file application.h
 * @brief Main application class for qHexWalker.
 *
 * This file contains the Application class which extends QGuiApplication
 * with application metadata and global event filtering.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#pragma once

#include <QGuiApplication>

/**
 * @class Application
 * @brief Main application class extending QGuiApplication.
 *
 * The Application class provides:
 * - Application metadata (name, version, logger name)
 * - Global event filtering for the entire application
 * - Configuration from build-time settings
 *
 * @par Example Usage
 * @code{.cpp}
 * int main(int argc, char *argv[]) {
 *     Application app(argc, argv);
 *
 *     qDebug() << app.getApplicationName();
 *     qDebug() << app.getApplicationVersion();
 *
 *     return app.exec();
 * }
 * @endcode
 *
 * @see EntryPoint for UI initialization
 */
class Application final : public QGuiApplication {
public:
    /**
     * @brief Constructs the Application.
     *
     * Initializes application metadata from build configuration
     * (config.h) and sets up the event filter.
     *
     * @param argc Command line argument count.
     * @param argv Command line argument values.
     */
    Application(int &argc, char **argv);

    /**
     * @brief Gets the application name.
     * @return Application name from build configuration.
     */
    [[nodiscard]] auto getApplicationName() const noexcept { return appName_; }

    /**
     * @brief Gets the application version.
     * @return Version string (e.g., "0.0.1").
     */
    [[nodiscard]] auto getApplicationVersion() const noexcept { return appVersion_; }

    /**
     * @brief Gets the logger name for spdlog.
     * @return Logger name string.
     */
    [[nodiscard]] auto getLoggerName() const noexcept { return loggerName_; }

protected:
    /**
     * @brief Global event filter for the application.
     *
     * Filters all events before they reach their target objects.
     * Can be used for global keyboard shortcuts, logging, etc.
     *
     * @param obj The object receiving the event.
     * @param event The event being processed.
     * @return true if the event should be filtered out, false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    std::string loggerName_;  ///< Logger name for spdlog.
    QString appName_;         ///< Application name.
    QString appVersion_;      ///< Application version string.
};