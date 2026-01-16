/**
 * @file h3Worker.h
 * @brief Worker thread for asynchronous H3 cell processing and pathfinding.
 *
 * This file contains the H3Worker class which runs pathfinding algorithms
 * in a separate thread to keep the UI responsive.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#ifndef Q_HEX_WALKER_H3WORKER_H
#define Q_HEX_WALKER_H3WORKER_H

#include "astar.h"

/**
 * @namespace H3_VIEWER
 * @brief Namespace containing H3 visualization components.
 */
namespace H3_VIEWER {

/**
 * @class H3Worker
 * @brief Worker thread for asynchronous H3 operations.
 *
 * H3Worker handles CPU-intensive operations in a background thread:
 * - Cell polygon computation
 * - A* pathfinding between waypoints
 * - Maze wall rendering preparation
 *
 * @par Threading Model
 * The worker runs in a dedicated QThread and communicates via signals/slots:
 * - Main thread calls requestCell() to queue work
 * - Worker thread processes requests in doWork()
 * - Results emitted via cellComputed(), searchStats() signals
 *
 * @par Example Usage
 * @code{.cpp}
 * auto worker = new H3Worker();
 * auto thread = new QThread();
 * worker->moveToThread(thread);
 *
 * connect(thread, &QThread::started, worker, &H3Worker::doWork);
 * connect(worker, &H3Worker::cellComputed, this, &MyClass::onCell);
 *
 * thread->start();
 * worker->requestCell(cellIndexes);
 * @endcode
 *
 * @see H3Model for the main consumer of worker results
 * @see H3AStar for the pathfinding algorithm
 */
class H3Worker final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Worker)

public:
    /**
     * @brief Constructs an H3Worker.
     * @param parent Optional parent QObject.
     */
    explicit H3Worker(QObject *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~H3Worker() override;

    /**
     * @struct PendingRequest
     * @brief Structure holding queued cell computation requests.
     */
    struct PendingRequest {
        std::vector<H3Index> indexes;  ///< Cell indices to process.
        bool has{};                    ///< Flag indicating pending request.
    };

public slots:
    /**
     * @brief Main work loop executed in the worker thread.
     *
     * Continuously processes pending requests and performs pathfinding.
     * Uses condition variable to wait efficiently when no work is available.
     */
    void doWork();

    /**
     * @brief Requests computation for a set of H3 cells.
     *
     * Queues cells for polygon computation and pathfinding.
     * Thread-safe - can be called from any thread.
     *
     * @param index Vector of H3 cell indices to process.
     */
    void requestCell(const std::vector<H3Index> &index);

    /**
     * @brief Sets maze walls for pathfinding obstacle avoidance.
     *
     * @param mazeWalls Set of H3 cell indices representing walls.
     */
    void setWalls(const std::unordered_set<H3Index> &mazeWalls);

private slots:
    /**
     * @brief Handles cell exploration events from A* algorithm.
     *
     * Called during pathfinding to enable real-time visualization.
     *
     * @param cell The H3 index of the newly explored cell.
     */
    void onAStarNewCell(H3Index cell);

signals:
    /**
     * @brief Emitted when the worker thread finishes.
     */
    void finished();

    /**
     * @brief Emitted when a single cell computation completes.
     *
     * @param res Cell resolution (3-15).
     * @param id H3 cell index.
     * @param polygon Cell boundary polygon coordinates.
     * @param isSearching true if emitted during active pathfinding.
     */
    void cellComputed(quint8 res, H3Index id, const QVariantList &polygon, bool isSearching);

    /**
     * @brief Emitted when batch cell computation completes.
     * @param polygon List of computed polygon data.
     */
    void cellsComputed(const QVariantList &polygon);

    /**
     * @brief Emitted with merged maze wall polygons for rendering.
     *
     * Polygons are pre-merged for efficient map rendering.
     *
     * @param polygons Vector of polygon coordinate lists.
     */
    void mazePolygonsComputed(const std::vector<QVariantList> &polygons);

    /**
     * @brief Emitted with pathfinding statistics after search completes.
     *
     * @param exploredCells Number of cells explored during search.
     * @param timeMs Search duration in milliseconds.
     * @param pathLength Number of cells in the found path.
     */
    void searchStats(int exploredCells, double timeMs, int pathLength);

    /**
     * @brief Emitted with batch of path cells for efficient rendering.
     *
     * @param cells Vector of cell data tuples (resolution, index, polygon).
     */
    void pathCellsBatch(const std::vector<std::tuple<quint8, H3Index, QVariantList>> &cells);

private:
    /// @brief Maze wall cells for obstacle avoidance.
    std::unordered_set<H3Index> walls;

    /// @brief A* pathfinding algorithm instance.
    H3AStar *astar_{};

    /// @brief Flag indicating a request is pending.
    std::atomic_bool isRequested{false};

    /// @brief Mutex for thread synchronization.
    std::mutex mutex_;

    /// @brief Condition variable for efficient waiting.
    std::condition_variable cv_;

    /// @brief Currently pending request.
    PendingRequest pending_;

    /// @brief Counter for explored cells during current search.
    std::atomic<int> exploredCellsCount_{0};
};

}  // namespace H3_VIEWER

#endif  // Q_HEX_WALKER_H3WORKER_H