/**
 * @file entryPoint.h
 * @brief Application entry point and initialization orchestrator.
 *
 * This file contains the EntryPoint class which handles all initialization
 * steps for the qHexWalker application.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#ifndef Q_HEX_WALKER_ENTRYPOINT_H
#define Q_HEX_WALKER_ENTRYPOINT_H

class QQuickWindow;
class QQmlApplicationEngine;
class MapProvider;

namespace TD {
class Logger;
}

class H3Model;
class H3TargetsModel;

/**
 * @class EntryPoint
 * @brief Application initialization orchestrator.
 *
 * EntryPoint is responsible for initializing all application components
 * in the correct order:
 * 1. Logger initialization
 * 2. Data models (H3Model, H3TargetsModel)
 * 3. Map provider and style
 * 4. QML engine and UI
 *
 * @par Initialization Flow
 * @msc
 *   EntryPoint,Logger,H3Model,H3TargetsModel,MapProvider,QmlEngine;
 *   EntryPoint->Logger [label="InitLogger()"];
 *   EntryPoint->H3Model [label="InitDataModels()"];
 *   EntryPoint->H3TargetsModel [label="create"];
 *   EntryPoint->MapProvider [label="InitMap()"];
 *   EntryPoint->QmlEngine [label="InitEngine()"];
 *   QmlEngine->EntryPoint [label="UI ready"];
 * @endmsc
 *
 * @par Example Usage
 * @code{.cpp}
 * int main(int argc, char *argv[]) {
 *     Application app(argc, argv);
 *     EntryPoint entry(app.getLoggerName());
 *     return app.exec();
 * }
 * @endcode
 *
 * @see Application for the main application class
 * @see H3Model for hexagonal cell data
 * @see H3TargetsModel for waypoint management
 */
class EntryPoint final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(EntryPoint)

public:
    /**
     * @brief Constructs and initializes the application.
     *
     * Performs complete application initialization sequence:
     * logger, models, map, and QML engine.
     *
     * @param loggerName Name for the spdlog logger instance.
     * @param parent Optional parent QObject.
     */
    explicit EntryPoint(const std::string &loggerName, QObject *parent = nullptr);

    /**
     * @brief Destructor.
     *
     * Cleans up all initialized components.
     */
    ~EntryPoint() override;

private:
    /**
     * @brief Initializes the spdlog logging system.
     * @param loggerName Name for the logger instance.
     */
    void InitLogger(const std::string &loggerName);

    /**
     * @brief Initializes data models (H3Model, H3TargetsModel).
     *
     * Creates and connects the hexagonal cell model and
     * waypoint target model.
     */
    void InitDataModels();

    /**
     * @brief Initializes the map provider and style.
     *
     * Sets up MapLibre map with online or offline tile source.
     */
    void InitMap();

    /// @brief Default MapTiler API URL for map tiles.
    const QString pathUrl_c{"https://api.maptiler.com/maps/base-v4/style.json?key=bFpEhpcbtSI3j1gzj2Is"};

    /**
     * @brief Initializes the QML engine and loads the UI.
     *
     * Creates QQmlApplicationEngine, registers C++ types,
     * and loads main.qml.
     */
    void InitEngine();

    QQmlApplicationEngine *engine_{};   ///< QML application engine.
    QQuickWindow *rootWindow_{};        ///< Root window reference.

    std::unique_ptr<TD::Logger> logger_;  ///< Logger instance.
    H3Model *h3Model_{};                  ///< Hexagonal cell model.
    H3TargetsModel *targetsModel_{};      ///< Waypoint targets model.
    MapProvider *mapProvider_{};          ///< Map tile provider.
};

#endif  // Q_HEX_WALKER_ENTRYPOINT_H