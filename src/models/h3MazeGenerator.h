/**
 * @file h3MazeGenerator.h
 * @brief Procedural maze generation on H3 hexagonal grids using Prim's algorithm.
 *
 * This file contains the H3MazeGenerator class which creates perfect mazes
 * on hexagonal grids using the randomized Prim's algorithm.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#pragma once

#include <random>

/**
 * @class H3MazeGenerator
 * @brief Generates perfect mazes on H3 hexagonal grids.
 *
 * The H3MazeGenerator uses a modified Prim's algorithm to create
 * perfect mazes (mazes with exactly one solution path) on hexagonal grids.
 *
 * @par Algorithm Overview
 * 1. Create a room grid with minimum 2-cell spacing
 * 2. Initialize with a random room
 * 3. Add walls to frontier
 * 4. Randomly select walls and connect rooms
 * 5. Repeat until all rooms are connected
 *
 * @par Example Usage
 * @code{.cpp}
 * H3MazeGenerator generator;
 *
 * // Generate maze centered at a cell with radius 20
 * auto result = generator.generateMazeWithEntrances(centerCell, 20);
 *
 * // result.walls contains all wall cells
 * // result.entrance is the maze entry point
 * // result.exit is the maze exit point
 * @endcode
 *
 * @see H3AStar for pathfinding through the maze
 * @see H3MazeAdapter for async maze generation
 */
class H3MazeGenerator final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs an H3MazeGenerator.
     * @param parent Optional parent QObject for memory management.
     */
    explicit H3MazeGenerator(QObject *parent = nullptr);

    /**
     * @brief Default destructor.
     */
    ~H3MazeGenerator() override = default;

    /**
     * @struct H3IndexHash
     * @brief Hash function for H3Index in unordered containers.
     */
    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };

    /**
     * @struct MazeResult
     * @brief Result structure containing generated maze data.
     */
    struct MazeResult {
        std::unordered_set<H3Index> walls;  ///< Set of wall cell indices.
        H3Index entrance;                   ///< Entry point cell index.
        H3Index exit;                       ///< Exit point cell index.
    };

    /**
     * @brief Generates a maze centered at the given cell.
     *
     * @param centerCell The H3 cell index at the maze center.
     * @param radius The maze radius in cells.
     * @return Set of H3 cell indices representing maze walls.
     *
     * @note For entrance/exit information, use generateMazeWithEntrances().
     */
    std::unordered_set<H3Index> generateMaze(H3Index centerCell, int radius);

    /**
     * @brief Generates a maze with entrance and exit points.
     *
     * Creates a perfect maze with identified entry and exit points.
     * The entrance and exit are placed at the farthest points from each other
     * within the maze.
     *
     * @param centerCell The H3 cell index at the maze center.
     * @param radius The maze radius in cells.
     * @return MazeResult containing walls, entrance, and exit.
     *
     * @par Algorithm Details
     * - Rooms are placed with minimum 2-cell spacing
     * - Walls fill the gaps between rooms
     * - BFS is used to find the farthest room pair for entrance/exit
     */
    MazeResult generateMazeWithEntrances(H3Index centerCell, int radius);

signals:
    /**
     * @brief Emitted to report maze generation progress.
     * @param percent Progress percentage (0-100).
     */
    void generationProgress(int percent);

    /**
     * @brief Emitted when maze generation is complete.
     * @param walls Set of wall cell indices.
     */
    void mazeGenerated(const std::unordered_set<H3Index> &walls);

private:
    /// @brief Random number generator for maze randomization.
    std::mt19937 rng_;

    /// @brief Temporary storage for neighbor cells.
    std::array<H3Index, 6> neighbors_{};

    /**
     * @brief Gets the 6 neighbors of a hexagonal cell.
     * @param cell The cell to get neighbors for.
     * @return Array of 6 neighbor indices.
     */
    std::array<H3Index, 6> getNeighbors(H3Index cell);

    /**
     * @brief Gets all cells within a radius of the center.
     *
     * @param center The center cell.
     * @param radius The radius in cells.
     * @return Set of all cells within the radius (excluding pentagons).
     */
    static std::unordered_set<H3Index, H3IndexHash> getCellsInRadius(H3Index center, int radius);

    /**
     * @brief Creates a sparse room grid with minimum 2-cell spacing.
     *
     * Rooms are the passable cells in the maze. This method ensures
     * rooms are separated by at least 2 cells to allow walls between them.
     *
     * @param allCells Set of all available cells.
     * @return Set of room cell indices.
     */
    std::unordered_set<H3Index, H3IndexHash> createRoomGrid(const std::unordered_set<H3Index, H3IndexHash> &allCells);

    /**
     * @brief Finds the wall cell between two rooms.
     *
     * Rooms are at distance 2 from each other; this finds the
     * intermediate cell that acts as a wall.
     *
     * @param room1 First room cell.
     * @param room2 Second room cell.
     * @return The wall cell, or nullopt if rooms aren't neighbors.
     */
    std::optional<H3Index> findWallBetween(H3Index room1, H3Index room2);

    /**
     * @brief Generates maze passages using randomized Prim's algorithm.
     *
     * @param rooms Set of room cells.
     * @return Set of passage cells (rooms + connecting corridors).
     */
    std::unordered_set<H3Index, H3IndexHash> generateMazePrim(const std::unordered_set<H3Index, H3IndexHash> &rooms);

    /**
     * @brief Gets rooms that are neighbors at distance 2.
     *
     * @param room The room to find neighbors for.
     * @param rooms Set of all rooms.
     * @return Vector of neighboring room indices.
     */
    static std::vector<H3Index> getRoomNeighbors(H3Index room, const std::unordered_set<H3Index, H3IndexHash> &rooms);

    /**
     * @brief Finds the room farthest from the start using BFS.
     *
     * Used to determine optimal entrance/exit placement.
     *
     * @param start Starting room cell.
     * @param passages Set of all passable cells.
     * @return The farthest room from start.
     */
    H3Index findFarthestRoom(H3Index start, const std::unordered_set<H3Index, H3IndexHash> &passages);

    /**
     * @brief Checks if a cell is on the border of the maze area.
     *
     * @param cell The cell to check.
     * @param center The maze center cell.
     * @param radius The maze radius.
     * @return true if the cell is on the border.
     */
    static bool isOnBorder(H3Index cell, H3Index center, int radius);
};