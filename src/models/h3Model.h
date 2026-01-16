/**
 * @file h3Model.h
 * @brief Qt list model for managing H3 hexagonal cells and pathfinding visualization.
 *
 * This file contains the H3Model class which provides a QAbstractListModel
 * implementation for displaying hexagonal cells on a map.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#ifndef Q_HEX_WALKER_H3MODEL_H
#define Q_HEX_WALKER_H3MODEL_H

#include <QAbstractListModel>

class H3Cell;
class H3MazeAdapter;

namespace H3_VIEWER {
class H3Worker;
}

/**
 * @class H3Model
 * @brief Qt model for H3 hexagonal cell visualization and management.
 *
 * H3Model serves as the central data model connecting the QML UI with
 * the pathfinding algorithms and maze generation. It manages:
 * - Hexagonal cell display data
 * - Path visualization
 * - Maze polygons for rendering
 * - Search statistics
 *
 * @par QML Properties
 * - coordinates: List of cell coordinates for map display
 * - mazePolygons: List of polygon coordinates for maze walls
 * - searchStatsText: Human-readable search statistics
 * - mazeCenter: Geographic center of the maze
 * - mazeRadius: Radius of the maze boundary in meters
 *
 * @par Example QML Usage
 * @code{.qml}
 * MapPolygon {
 *     path: h3Model.coordinates
 *     color: "blue"
 * }
 *
 * Text {
 *     text: h3Model.searchStatsText
 * }
 * @endcode
 *
 * @see H3Cell for individual cell data
 * @see H3Worker for async pathfinding
 * @see H3MazeAdapter for maze generation
 */
class H3Model final : public QAbstractListModel {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Model)

    /// @brief Cell polygon coordinates for QML map display.
    Q_PROPERTY(QVariantList coordinates READ coordinates NOTIFY coordinatesChanged)

    /// @brief Maze wall polygons for QML rendering.
    Q_PROPERTY(QList<QVariantList> mazePolygons READ mazePolygons NOTIFY mazePolygonsChanged)

    /// @brief Human-readable search statistics text.
    Q_PROPERTY(QString searchStatsText READ searchStatsText NOTIFY searchStatsChanged)

    /// @brief Geographic center of the generated maze.
    Q_PROPERTY(QGeoCoordinate mazeCenter READ mazeCenter NOTIFY mazeCenterChanged)

    /// @brief Maze boundary radius in meters.
    Q_PROPERTY(double mazeRadius READ mazeRadius NOTIFY mazeRadiusChanged)

public:
    /**
     * @enum Roles
     * @brief Custom roles for the list model.
     */
    enum Roles {
        ResRole = Qt::UserRole + 1,  ///< Cell resolution (3-15).
        IndexRole,                   ///< H3 cell index.
        CellColor,                   ///< Cell display color.
        PathRole                     ///< Whether cell is part of path.
    };

    /**
     * @brief Constructs an H3Model.
     * @param parent Optional parent QObject.
     */
    explicit H3Model(QObject *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~H3Model() override;

    /**
     * @brief Returns the number of cells in the model.
     * @param parent Parent index (unused for list models).
     * @return Number of cells.
     */
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;

    /**
     * @brief Returns data for a cell at the given index.
     * @param index Model index.
     * @param role Data role to retrieve.
     * @return Cell data as QVariant.
     */
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    /**
     * @brief Returns the role names for QML access.
     * @return Hash mapping role IDs to role names.
     */
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Gets cell coordinates for map display.
     * @return List of coordinate pairs.
     */
    [[nodiscard]] QVariantList coordinates() const noexcept { return coordinates_; }

    /**
     * @brief Gets maze wall polygons.
     * @return List of polygon coordinate lists.
     */
    [[nodiscard]] QList<QVariantList> mazePolygons() const noexcept { return mazePolygons_; }

    /**
     * @brief Gets search statistics as formatted text.
     * @return Statistics string (e.g., "Cells: 847 | Time: 23ms | Path: 156").
     */
    [[nodiscard]] QString searchStatsText() const noexcept { return searchStatsText_; }

    /**
     * @brief Gets the geographic center of the maze.
     * @return Maze center coordinate.
     */
    [[nodiscard]] QGeoCoordinate mazeCenter() const noexcept { return mazeCenter_; }

    /**
     * @brief Gets the maze boundary radius.
     * @return Radius in meters.
     */
    [[nodiscard]] double mazeRadius() const noexcept { return mazeRadius_; }

private slots:
    /**
     * @brief Handles batch cell computation results.
     * @param list List of computed cell data.
     */
    void onCellsComputed(const QVariantList &list);

    /**
     * @brief Handles individual cell computation during search.
     * @param res Cell resolution.
     * @param index H3 cell index.
     * @param polygon Cell polygon coordinates.
     * @param isSearching Whether this is during active search.
     */
    void onCellComputed(quint8 res, H3Index index, const QVariantList &polygon, bool isSearching);

    /**
     * @brief Handles batch cell computation for path visualization (optimized).
     * @param cells Vector of cell data tuples (res, index, polygon).
     */
    void onPathCellsBatch(const std::vector<std::tuple<quint8, H3Index, QVariantList>> &cells);

    /**
     * @brief Handles maze polygon computation results.
     * @param polygons Vector of polygon coordinate lists.
     */
    void onMazePolygonsComputed(const std::vector<QVariantList> &polygons);

    /**
     * @brief Handles search statistics update.
     * @param exploredCells Number of cells explored.
     * @param timeMs Search time in milliseconds.
     * @param pathLength Length of found path.
     */
    void onSearchStats(int exploredCells, double timeMs, int pathLength);

public:
    /**
     * @brief Initializes the model and starts worker thread.
     *
     * Must be called after construction to set up the worker thread
     * and maze adapter.
     */
    void Init();

public slots:
    /**
     * @brief Requests visualization of specific cells.
     * @param indexes Vector of H3 cell indices to display.
     */
    Q_INVOKABLE void requestCells(const std::vector<H3Index> &indexes);

    /**
     * @brief Clears all cells from the model.
     */
    Q_INVOKABLE void clearAllCells();

signals:
    /// @brief Emitted when cell clearing begins.
    void clearingStarted();

    /// @brief Emitted when cell clearing completes.
    void clearingFinished();

    /// @brief Emitted when coordinates property changes.
    void coordinatesChanged();

    /// @brief Emitted when maze polygons change.
    void mazePolygonsChanged();

    /// @brief Emitted when search statistics update.
    void searchStatsChanged();

    /// @brief Emitted when maze walls are generated.
    void mazeWallsGenerated(const std::unordered_set<H3Index> &walls);

    /// @brief Emitted when maze center changes.
    void mazeCenterChanged();

    /// @brief Emitted when maze radius changes.
    void mazeRadiusChanged();

    /// @brief Emitted with maze boundary information.
    void mazeBoundsGenerated(const QGeoCoordinate &center, double radiusMeters);

private:
    /**
     * @brief Finds a cell by its H3 index.
     * @param id H3 cell index.
     * @return Cell pointer if found, nullopt otherwise.
     */
    [[nodiscard]] std::optional<H3Cell *> findCellByID(quint64 id) const;

    /**
     * @brief Finds a cell by its resolution.
     * @param res Cell resolution.
     * @return Cell pointer if found, nullopt otherwise.
     */
    [[nodiscard]] std::optional<H3Cell *> findCellByRes(quint8 res) const;

    /**
     * @brief Validates if a coordinate is a valid target.
     * @param zoom Current zoom level.
     * @param coordinate Target coordinate.
     * @return true if the coordinate is valid.
     */
    [[nodiscard]] bool isCoordinateTargetValid(quint8 zoom, const QGeoCoordinate &coordinate) const;

    /**
     * @brief Gets the display color for a resolution level.
     * @param resolution Cell resolution (3-15).
     * @return Color name string.
     */
    [[nodiscard]] QString getColorForResolution(quint8 resolution) const;

    /**
     * @brief Adds a new cell to the model.
     * @param res Cell resolution.
     * @param index H3 cell index.
     * @param polygon Cell polygon coordinates.
     * @param color Cell display color.
     */
    void addCell(quint8 res, H3Index index, const QVariantList &polygon, const QColor &color);

    /**
     * @brief Adds pentagon cells to the model.
     */
    void addPentagons();

    H3_VIEWER::H3Worker *worker_{};  ///< Worker for async operations.
    QThread *thread_{};              ///< Worker thread.
    H3MazeAdapter *mazeAdapter_{};   ///< Maze generation adapter.

    QList<H3Cell *> pathCells_;         ///< Cells that are part of the current path.
    QVariantList coordinates_;          ///< Cell coordinates for display.
    QList<QVariantList> mazePolygons_;  ///< Maze wall polygons.
    QString searchStatsText_;           ///< Formatted search statistics.
    QGeoCoordinate mazeCenter_;         ///< Maze center coordinate.
    double mazeRadius_{0.0};            ///< Maze boundary radius in meters.

    const uint8_t minZoom_c{3};                       ///< Minimum supported zoom level.
    const uint8_t maxZoom_c{15};                      ///< Maximum supported zoom level.
    std::unordered_map<uint8_t, uint8_t> zoomToRes_;  ///< Zoom to resolution mapping.

    /// @brief Color palette for different resolutions.
    const QHash<int, QString> resolutionColors_c = {
        {3, "limegreen"},   {4, "green"},       {5, "darkGreen"},       {6, "gold"},       {7, "yellow"},
        {8, "greenyellow"}, {9, "limegreen"},   {10, "mediumseagreen"}, {11, "turquoise"}, {12, "deepskyblue"},
        {13, "dodgerblue"}, {14, "mediumblue"}, {15, "darkviolet"}};

    std::atomic_bool isClearing_{false};  ///< Flag indicating clearing in progress.
};

#endif  // Q_HEX_WALKER_H3MODEL_H