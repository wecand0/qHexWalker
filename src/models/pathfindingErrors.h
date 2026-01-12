/**
 * @file pathfindingErrors.h
 * @brief Error handling strategy for pathfinding algorithms
 *
 * This file defines a consistent error handling approach across all pathfinding
 * algorithms in the project.
 *
 * @author qHexWalker Team
 * @date 2025
 */

#ifndef Q_HEX_WALKER_PATHFINDING_ERRORS_H
#define Q_HEX_WALKER_PATHFINDING_ERRORS_H

#include <stdexcept>
#include <string>

/**
 * @enum PathfindingError
 * @brief Error codes for pathfinding operations
 *
 * Consistent error codes across all pathfinding algorithms (A*, Dijkstra, etc.)
 */
enum class PathfindingError {
    None = 0,          ///< No error occurred
    InvalidCell,       ///< One or both cells are invalid (H3_NULL or failed isValidCell)
    SameStartEnd,      ///< Start and end points are identical
    BlockedStartCell,  ///< Start cell is blocked by an obstacle
    BlockedEndCell,    ///< End cell is blocked by an obstacle
    NoPathFound,       ///< No path exists between start and end (disconnected graph)
    ConversionError,   ///< Error converting between H3 resolutions
    CoordinateError,   ///< Error getting cell coordinates
    MaxIterations,     ///< Search exceeded maximum iteration limit
    InternalError      ///< Internal algorithm error
};

/**
 * @brief Converts PathfindingError to human-readable string
 * @param error The error code
 * @return String description of the error
 */
inline std::string pathfindingErrorToString(PathfindingError error) {
    switch (error) {
    case PathfindingError::None:
        return "No error";
    case PathfindingError::InvalidCell:
        return "Invalid H3 cell index";
    case PathfindingError::SameStartEnd:
        return "Start and end points are identical";
    case PathfindingError::BlockedStartCell:
        return "Start cell is blocked";
    case PathfindingError::BlockedEndCell:
        return "End cell is blocked";
    case PathfindingError::NoPathFound:
        return "No path found between points";
    case PathfindingError::ConversionError:
        return "Error converting between H3 resolutions";
    case PathfindingError::CoordinateError:
        return "Error getting cell coordinates";
    case PathfindingError::MaxIterations:
        return "Search exceeded maximum iterations";
    case PathfindingError::InternalError:
        return "Internal algorithm error";
    default:
        return "Unknown error";
    }
}

/**
 * @class PathfindingException
 * @brief Exception class for pathfinding errors
 *
 * Wraps PathfindingError enum for exception-based error handling.
 */
class PathfindingException final : public std::runtime_error {
public:
    explicit PathfindingException(const PathfindingError error)
        : std::runtime_error(pathfindingErrorToString(error)), errorCode_(error) {}

    [[nodiscard("known error")]] PathfindingError errorCode() const noexcept { return errorCode_; }

private:
    PathfindingError errorCode_;
};

/**
 * @brief Error Handling Strategy
 *
 * PROJECT-WIDE CONVENTIONS:
 *
 * 1. **For critical errors that prevent operation**:
 *    - Throw PathfindingException with appropriate error code
 *    - Log with spdlog::error() before throwing
 *    - Use for: InvalidCell, ConversionError, CoordinateError
 *
 *    Example:
 *    @code
 *    if (!isValidCell(start)) {
 *        spdlog::error("Invalid start cell: 0x{:x}", start);
 *        throw PathfindingException(PathfindingError::InvalidCell);
 *    }
 *    @endcode
 *
 * 2. **For expected failures (no path, blocked cells)**:
 *    - Return empty result (std::vector<H3Index>{})
 *    - Log with spdlog::warn()
 *    - Use for: NoPathFound, BlockedStartCell, BlockedEndCell
 *
 *    Example:
 *    @code
 *    if (blockedCells.contains(start)) {
 *        spdlog::warn("Start cell is blocked: 0x{:x}", start);
 *        return {};
 *    }
 *    @endcode
 *
 * 3. **For H3 API failures**:
 *    - Check error code immediately after call
 *    - Log with spdlog::error() and return/throw based on severity
 *    - Use describeH3Error() for error messages
 *
 *    Example:
 *    @code
 *    if (H3Error err = gridRing(cell, radius, data); err != E_SUCCESS) {
 *        spdlog::error("gridRing failed: {}", describeH3Error(err));
 *        return;  // or throw depending on context
 *    }
 *    @endcode
 *
 * 4. **Logging levels**:
 *    - error: Critical failures, invalid input, API errors
 *    - warn: Expected failures, blocked paths
 *    - info: Successful operations, statistics
 *
 * @note C++23 std::expected will replace this approach in future versions
 */

#endif  // Q_HEX_WALKER_PATHFINDING_ERRORS_H