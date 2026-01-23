/**
 * @file astar.h
 * @brief Bidirectional A* pathfinding algorithm for H3 hexagonal grids.
 *
 * This file contains the H3AStar class which implements an optimized
 * bidirectional A* search algorithm with hierarchical resolution refinement.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#ifndef Q_HEX_WALKER_ASTAR_H
#define Q_HEX_WALKER_ASTAR_H

#include <unordered_set>

/**
 * @class H3AStar
 * @brief Bidirectional A* pathfinding algorithm for H3 hexagonal cells.
 *
 * The H3AStar class provides an efficient pathfinding implementation that:
 * - Uses bidirectional search (from start and goal simultaneously)
 * - Employs hierarchical resolution refinement (coarse to fine)
 * - Supports obstacle avoidance via blocked cells
 *
 * @par Algorithm Complexity
 * - Standard A*: O(b^d)
 * - Bidirectional A*: O(b^(d/2))
 *
 * Where b = branching factor (6 for hexagons), d = path depth
 *
 * @par Example Usage
 * @code{.cpp}
 * H3AStar astar;
 * astar.setBlockedCells(mazeWalls);
 *
 * // Connect to visualization signal
 * connect(&astar, &H3AStar::newCell, this, &MyClass::onCellExplored);
 *
 * // Find path
 * auto path = astar.findShortestPath(startCell, endCell);
 * @endcode
 *
 * @see H3MazeGenerator for maze generation
 * @see H3Worker for async execution
 */
class H3AStar final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3AStar)

public:
    /**
     * @brief Constructs an H3AStar pathfinder.
     * @param parent Optional parent QObject for memory management.
     */
    explicit H3AStar(QObject *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~H3AStar() override;

    /**
     * @brief Finds the shortest path between two H3 cells.
     *
     * Uses bidirectional A* search with hierarchical resolution refinement:
     * 1. Searches at resolution 3 for coarse path
     * 2. Refines start and end segments to target resolution
     *
     * @param start The starting H3 cell index.
     * @param end The destination H3 cell index.
     * @return Vector of H3 cell indices representing the path (empty if no path found).
     *
     * @note Emits newCell() signal for each explored cell during search.
     * @warning Both start and end must be valid H3 indices at the same resolution.
     */
    std::vector<H3Index> findShortestPath(H3Index start, H3Index end);

    /**
     * @brief Sets cells that cannot be traversed (obstacles).
     *
     * Call this method before findShortestPath() to define obstacles
     * such as maze walls.
     *
     * @param blocked Set of H3 cell indices that are impassable.
     */
    void setBlockedCells(const std::unordered_set<H3Index> &blocked);

signals:
    /**
     * @brief Emitted when a new cell is explored during pathfinding.
     *
     * Connect to this signal to visualize the search progress in real-time.
     *
     * @param h3Index The H3 index of the newly explored cell.
     */
    void newCell(H3Index h3Index);

private:
    /// @brief Maximum number of cells at resolution 3 (~41K cells globally).
    static constexpr uint16_t MAX_CELLS_RES_3{41'162};

    /// @brief Maximum neighbors for a hexagonal cell (always 6).
    static constexpr int MAX_NEIGHBORS{6};

    /**
     * @struct Node
     * @brief Internal node structure for A* priority queue.
     */
    struct Node {
        H3Index cell{};   ///< The H3 cell index.
        double gScore{};  ///< Cost from start to this node.
        double fScore{};  ///< Estimated total cost (gScore + heuristic).

        /**
         * @brief Comparison operator for priority queue (min-heap).
         * @param other The other node to compare.
         * @return true if this node has higher fScore (lower priority).
         */
        bool operator>(const Node &other) const { return fScore > other.fScore; }
    };

    /**
     * @struct H3IndexHash
     * @brief Hash function for H3Index in unordered containers.
     */
    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };

    /// @brief Set of blocked (impassable) cells.
    std::unordered_set<H3Index> blockedCells{};

    /**
     * @brief Finds path at resolution 3 using bidirectional A*.
     * @param start Start cell (will be converted to res 3).
     * @param end End cell (will be converted to res 3).
     * @param endCoord Geographic coordinates of end point for heuristic.
     * @return Path at resolution 3.
     */
    std::vector<H3Index> findPathAtResolution3(H3Index start, H3Index end, const LatLng &endCoord);

    /**
     * @brief Refines path segment near the end point to target resolution.
     * @param prevInPath Previous cell in the coarse path.
     * @param parentEnd Parent cell at resolution 3.
     * @param originalEnd Original end cell at target resolution.
     * @param endRes Target resolution.
     * @return Refined path segment.
     */
    std::vector<H3Index> refineEndSegmentGradual(H3Index prevInPath, H3Index parentEnd, H3Index originalEnd,
                                                 int endRes);

    /**
     * @brief Refines path segment near the start point to target resolution.
     * @param originalStart Original start cell at target resolution.
     * @param nextInPath Next cell in the coarse path.
     * @param startRes Target resolution.
     * @return Refined path segment.
     */
    std::vector<H3Index> refineStartSegmentGradual(H3Index originalStart, H3Index nextInPath, int startRes) const;

    /**
     * @brief Refines entire coarse path to target resolution.
     * @param pathRes3 Coarse path at resolution 3.
     * @param originalStart Original start cell.
     * @param originalEnd Original end cell.
     * @param startRes Start cell resolution.
     * @param endRes End cell resolution.
     * @return Fully refined path.
     */
    std::vector<H3Index> refinePath(const std::vector<H3Index> &pathRes3, H3Index originalStart, H3Index originalEnd,
                                    int startRes, int endRes);

    /**
     * @brief Finds boundary cell in a direction for path refinement.
     */
    static H3Index findBoundaryCellInDirection(const std::vector<H3Index> &cells, H3Index from, H3Index direction);

    /**
     * @brief Finds local path within a parent cell boundary.
     */
    std::vector<H3Index> findLocalPathAtResolution(H3Index start, H3Index end, H3Index limitParent) const;

    /**
     * @brief Gets all children of a cell at specified resolution.
     * @param parent Parent cell index.
     * @param resolution Target resolution.
     * @return Vector of child cell indices.
     */
    static std::vector<H3Index> getChildrenAtResolution(H3Index parent, int resolution);

    /**
     * @brief Gets the 6 neighbors of a hexagonal cell.
     * @param cell The cell to get neighbors for.
     * @return Array of 6 neighbor indices (0 for invalid/pentagon edges).
     */
    static std::array<H3Index, MAX_NEIGHBORS> getNeighbors(H3Index cell);

    /**
     * @brief Calculates geographic distance between two cells.
     * @param cell1 First cell index.
     * @param cell2 Second cell index.
     * @return Distance in radians.
     */
    static double getDistanceBetweenCells(H3Index cell1, H3Index cell2);

    /**
     * @brief Heuristic function for A* (distance to target).
     * @param cell Current cell.
     * @param targetCoord Target coordinates.
     * @return Estimated distance to target.
     */
    static double heuristic(H3Index cell, const LatLng &targetCoord);

    /**
     * @brief Reconstructs path from the visited nodes map.
     * @param previous Map of cell -> previous cell in path.
     * @param start Start cell.
     * @param end End cell.
     * @return Reconstructed path from start to end.
     */
    static std::vector<H3Index> reconstructPath(const std::unordered_map<H3Index, H3Index, H3IndexHash> &previous,
                                                H3Index start, H3Index end);

    /**
     * @brief Converts any cell to its parent at resolution 3.
     */
    static H3Index cellToParentRes3(H3Index index);

    /**
     * @brief Converts resolution 3 cell to child at original resolution.
     */
    static H3Index cellToChildRes3(H3Index index);
};

#endif  // Q_HEX_WALKER_ASTAR_H