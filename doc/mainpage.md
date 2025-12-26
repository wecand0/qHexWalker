# qHexWalker {#mainpage}

@tableofcontents

## Overview

**qHexWalker** is a Qt 6 desktop application that combines Uber's H3 hexagonal indexing system
with MapLibre maps to provide interactive pathfinding and maze visualization on hexagonal grids.

## Key Features

- **H3 Hexagonal Grid Visualization** - Display hierarchical hexagonal cells at resolutions 3-15
- **Bidirectional A* Pathfinding** - Fast pathfinding with hierarchical resolution refinement
- **Procedural Maze Generation** - Generate perfect mazes using Prim's algorithm on hex grids
- **Multi-Waypoint Routing** - Plan routes through multiple destinations
- **Real-time Visualization** - Watch the algorithm explore cells during search

## Architecture

The application follows a layered architecture:

@dot
digraph architecture {
    rankdir=TB;
    node [shape=box, style=filled];

    subgraph cluster_ui {
        label="UI Layer";
        style=filled;
        color=lightblue;
        QML [label="main.qml\n(QML UI)"];
    }

    subgraph cluster_models {
        label="Models Layer";
        style=filled;
        color=lightgreen;
        H3Model [label="H3Model"];
        H3TargetsModel [label="H3TargetsModel"];
    }

    subgraph cluster_business {
        label="Business Logic Layer";
        style=filled;
        color=lightyellow;
        H3Worker [label="H3Worker\n(Threading)"];
        H3MazeAdapter [label="H3MazeAdapter"];
        MapProvider [label="MapProvider"];
    }

    subgraph cluster_algo {
        label="Algorithm Layer";
        style=filled;
        color=lightpink;
        H3AStar [label="H3AStar\n(Bidirectional A*)"];
        H3MazeGenerator [label="H3MazeGenerator\n(Prim's Algorithm)"];
    }

    QML -> H3Model;
    QML -> H3TargetsModel;
    H3Model -> H3Worker;
    H3Model -> H3MazeAdapter;
    H3Worker -> H3AStar;
    H3MazeAdapter -> H3MazeGenerator;
}
@enddot

## Module Structure

### Core Module

The core module contains application initialization and configuration:

| Class | Description |
|-------|-------------|
| @ref Application | QGuiApplication subclass with app metadata |
| @ref EntryPoint | UI initialization orchestrator |
| @ref MapProvider | Map style and source management |

### Models Module

The models module implements data models and algorithms:

| Class | Description |
|-------|-------------|
| @ref H3Model | QAbstractListModel for hexagonal cells |
| @ref H3TargetsModel | QAbstractListModel for waypoints |
| @ref H3Cell | Data model for a single H3 cell |
| @ref H3Target | Data model for a waypoint target |
| @ref H3Worker | Threading wrapper for async operations |
| @ref H3AStar | Bidirectional A* pathfinding algorithm |
| @ref H3MazeGenerator | Maze generation using Prim's algorithm |
| @ref H3MazeAdapter | Async wrapper for maze generation |

## Algorithms

### Bidirectional A* Search

The H3AStar class implements bidirectional A* with hierarchical resolution refinement:

@code{.cpp}
// Example usage
H3AStar astar;
astar.setBlockedCells(mazeWalls);
auto path = astar.findShortestPath(startCell, endCell);
@endcode

**Algorithm steps:**

1. **Coarse search at Resolution 3** - Find approximate path on coarse grid
2. **Bidirectional expansion** - Search from both start and goal simultaneously
3. **Path refinement** - Refine path to target resolution

**Complexity:** O(b^(d/2)) instead of O(b^d) for standard A*

### Maze Generation (Prim's Algorithm)

The H3MazeGenerator class creates perfect mazes on hexagonal grids:

@code{.cpp}
// Example usage
H3MazeGenerator generator;
auto result = generator.generateMazeWithEntrances(centerCell, radius);
// result.walls - set of wall cells
// result.entrance - maze entrance
// result.exit - maze exit
@endcode

**Algorithm steps:**

1. Create room grid with 2-cell spacing
2. Initialize with random room
3. Add walls to frontier
4. Connect rooms through walls
5. Repeat until all rooms connected

## Threading Model

The application uses a worker thread for pathfinding to keep UI responsive:

@msc
    UI,H3Model,H3Worker,H3AStar;
    UI->H3Model [label="click"];
    H3Model->H3Worker [label="requestPath()"];
    H3Worker->H3AStar [label="findShortestPath()"];
    H3AStar->H3Worker [label="newCell()"];
    H3Worker->H3Model [label="cellComputed()"];
    H3Model->UI [label="update"];
    H3AStar->H3Worker [label="path result"];
    H3Worker->H3Model [label="searchStats()"];
    H3Model->UI [label="statistics"];
@endmsc

## Building

### Requirements

- CMake >= 3.19
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2022+)
- Qt 6.5+ (QuickControls2, Sql, Positioning)
- vcpkg with packages: h3, spdlog, gtest, benchmark
- MapLibre Native Qt

### Build Commands

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH="<Qt6>;<MapLibre>"

cmake --build build -j
```

### Build Documentation

```bash
cd /path/to/qHexWalker
doxygen Doxyfile
# Documentation generated in docs/html/
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| Qt 6 | 6.5+ | UI framework |
| H3 | 4.4.0+ | Hexagonal indexing |
| MapLibre Native Qt | Latest | Map rendering |
| spdlog | 1.16.0+ | Logging |
| GTest | 1.17.0+ | Unit testing |
| Google Benchmark | 1.9.4+ | Performance testing |

## License

MIT License - See LICENSE file for details.

---

@author qHexWalker Development Team
@version 0.0.1
@date 2025