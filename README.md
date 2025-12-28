<div align="center">

# qHexWalker

**Hexagonal Grid Pathfinding & Maze Visualization on Interactive Maps**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Qt 6](https://img.shields.io/badge/Qt-6.5%2B-green.svg)](https://www.qt.io/)
[![H3](https://img.shields.io/badge/H3-4.4.0-orange.svg)](https://h3geo.org/)
[![MapLibre](https://img.shields.io/badge/MapLibre-Native%20Qt-purple.svg)](https://maplibre.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-Doxygen-blue.svg)](https://wecand0.github.io/qHexWalker/)


<img width="600" height="800" alt="qHexWalker Maze Screenshot" src="https://github.com/user-attachments/assets/475279b0-b57a-450a-99da-4c999c994df7" />


[Features](#features) | [How It Works](#how-it-works) | [Installation](#installation) | [Usage](#usage) | [Architecture](#architecture) | [API Docs](https://wecand0.github.io/qHexWalker/)

</div>

---

## Overview

**qHexWalker** is a Qt 6 desktop application that combines Uber's H3 hexagonal indexing system with MapLibre maps to provide:

- **Interactive hexagonal grid visualization** at multiple resolutions
- **Procedural maze generation** using Prim's algorithm on hex grids
- **Bidirectional A* pathfinding** with hierarchical resolution refinement
- **Multi-waypoint route planning** with real-time visualization

### Demo video


https://github.com/user-attachments/assets/e94c49b7-bd74-475e-b5ab-d93d168e09cf







---

## Features

| Feature | Description |
|---------|-------------|
| **H3 Hexagonal Grid** | Visualize Uber's hierarchical spatial index (resolutions 3-15) |
| **Bidirectional A*** | Fast pathfinding that searches from both ends simultaneously |
| **Maze Generation** | Procedural mazes using randomized Prim's algorithm |
| **Multi-Waypoint Routing** | Plan routes through multiple destinations |
| **Real-time Visualization** | Watch the algorithm explore cells as it searches |
| **Search Statistics** | View explored cells count, time, and path length |
| **Dark Theme UI** | Modern Material Design interface |

---

## How It Works

### H3 Hexagonal Indexing

H3 divides the Earth into hexagonal cells at 16 resolutions (0-15). Hexagons provide uniform distance to neighbors and better tessellation than squares.

```
Resolution  │  Avg. Hex Area  │  Hex Count
────────────┼─────────────────┼──────────────
     3      │   12,392 km²    │     41,162
     7      │      5.16 km²   │    98M
    10      │   14,950 m²     │    23B
    15      │     0.9 m²      │    569T
```

### Bidirectional A* Algorithm

The pathfinding uses bidirectional A* with hierarchical resolution refinement:

```
┌─────────────────────────────────────────────────────────────────────┐
│                    BIDIRECTIONAL A* SEARCH                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│    START ──────────────►  ◄────────────── GOAL                      │
│      │                        │                                     │
│      ▼                        ▼                                     │
│  ┌────────┐              ┌────────┐                                 │
│  │Forward │              │Backward│                                 │
│  │ Search │    MEET      │ Search │                                 │
│  │        │◄────────────►│        │                                 │
│  └────────┘              └────────┘                                 │
│      │                        │                                     │
│      └────────────┬───────────┘                                     │
│                   ▼                                                 │
│           ┌──────────────┐                                          │
│           │ Optimal Path │                                          │
│           └──────────────┘                                          │
│                                                                     │
│  Complexity: O(b^(d/2)) instead of O(b^d)                           │
└─────────────────────────────────────────────────────────────────────┘
```

#### Resolution Hierarchy

To optimize performance, the algorithm uses multi-resolution search:

```
Step 1: Coarse Search (Resolution 3)
┌───────────────────────────────────┐
│  ┌─────┐     ┌─────┐     ┌─────┐  │
│  │     │────►│     │────►│     │  │
│  └─────┘     └─────┘     └─────┘  │
│   START                    GOAL   │
└───────────────────────────────────┘
              │
              ▼
Step 2: Refine Path (Higher Resolution)
┌─────────────────────────────────────────┐
│  ⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡─⬡    │
│  Detailed path at target                │
│  resolution with all cells              │
└─────────────────────────────────────────┘
```

### Maze Generation (Prim's Algorithm)

The maze generator creates perfect mazes on hexagonal grids:

```
┌─────────────────────────────────────────────────────────────────────┐
│                   RANDOMIZED PRIM'S ALGORITHM                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     
│  1. Create Room Grid         2. Initialize with Random Room        
│     ⬡ · ⬡ · ⬡ · ⬡               ⬡ · ⬡ · ⬡ · ⬡                    
│     · · · · · · ·               · · · · · · ·                      
│     ⬡ · ⬡ · ⬡ · ⬡               ⬡ · ■ · ⬡ · ⬡  ← Start room      
│     · · · · · · ·               · · · · · · ·                      
│     ⬡ · ⬡ · ⬡ · ⬡               ⬡ · ⬡ · ⬡ · ⬡                    
│                                                                    
│  3. Add Walls to Frontier    4. Connect Rooms Through Walls        
│     ⬡ · ⬡ · ⬡ · ⬡               ⬡ · ⬡ · ⬡ · ⬡                    
│     · · W · · · ·               · · │ · · · ·                      
│     ⬡ W ■ W ⬡ · ⬡               ⬡───■───⬡ · ⬡                     
│     · · W · · · ·               · · │ · · · ·                      
│     ⬡ · ⬡ · ⬡ · ⬡               ⬡ · ⬡ · ⬡ · ⬡                    
│                                                                    
│  5. Repeat Until All Connected                                     
│     ⬡───⬡───⬡───⬡                                                  
│     │       │                                                      
│     ⬡   ⬡───⬡   ⬡   Perfect maze with                              
│     │   │       │   single solution path                           
│     ⬡───⬡   ⬡───⬡                                                  
│                                                                     
└─────────────────────────────────────────────────────────────────────┘
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         APPLICATION LAYERS                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                      QML UI Layer                           │    │
│  │  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐    │     │
│  │  │ Target List │  │   Map View   │  │   Statistics    │    │     │
│  │  └─────────────┘  └──────────────┘  └─────────────────┘    │     │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                    Qt Models Layer                          │    │
│  │  ┌─────────────────────┐    ┌─────────────────────────┐     │    │
│  │  │      H3Model        │    │    H3TargetsModel       │     │    │
│  │  │ (QAbstractListModel)│    │  (QAbstractListModel)   │     │    │
│  │  └─────────────────────┘    └─────────────────────────┘     │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                  Business Logic Layer                       │    │
│  │  ┌──────────┐  ┌──────────────┐  ┌───────────────────┐      │    │
│  │  │ H3Worker │  │ H3MazeAdapter│  │   MapProvider     │      │    │
│  │  │ (Thread) │  │   (Async)    │  │  (Style/Source)   │      │    │
│  │  └──────────┘  └──────────────┘  └───────────────────┘      │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                   Algorithm Layer                           │    │
│  │  ┌─────────────────────┐    ┌─────────────────────────┐     │    │
│  │  │     H3AStar         │    │   H3MazeGenerator       │     │    │
│  │  │ (Bidirectional A*)  │    │  (Prim's Algorithm)     │     │    │
│  │  └─────────────────────┘    └─────────────────────────┘     │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Threading Model

```
┌─────────────────┐          ┌─────────────────┐
│   Main Thread   │          │  Worker Thread  │
│   (Qt Event     │          │   (H3Worker)    │
│    Loop + UI)   │          │                 │
├─────────────────┤          ├─────────────────┤
│                 │  request │                 │
│  User Click ────┼─────────►│  A* Search      │
│                 │          │       │         │
│                 │  signal  │       ▼         │
│  Update UI  ◄───┼──────────│  Path Result    │
│                 │          │                 │
└─────────────────┘          └─────────────────┘
```

---

## Installation

### Requirements

| Component | Version | Notes |
|-----------|---------|-------|
| CMake | >= 3.19 | Build system |
| C++ Compiler | C++20 | GCC 10+, Clang 12+, MSVC 2022+ |
| Qt | 6.5+ | QuickControls2, Sql, Positioning |
| vcpkg | Latest | Package manager |
| MapLibre Native Qt | Latest | Map rendering |

### Dependencies (via vcpkg)

- **H3** - Hexagonal hierarchical spatial index
- **spdlog** - Fast logging library
- **GTest** - Unit testing (optional)
- **Google Benchmark** - Performance testing (optional)

### Build Steps

#### 1. Install Qt 6

```bash
# Linux (Ubuntu/Debian)
sudo apt install qt6-base-dev qt6-declarative-dev qt6-positioning-dev

# macOS
brew install qt@6

# Windows
# Use Qt Online Installer
```

#### 2. Install vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
# or bootstrap-vcpkg.bat on Windows
```

#### 3. Build MapLibre Native Qt

```bash
git clone https://github.com/maplibre/maplibre-native-qt.git
cd maplibre-native-qt

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/maplibre-native-qt" \
  -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/<platform>" \
  -DCMAKE_TOOLCHAIN_FILE="/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake --build build -j
cmake --install build
```

#### 4. Build qHexWalker

```bash
git clone https://github.com/your-username/qHexWalker.git
cd qHexWalker

export VCPKG_ROOT="/path/to/vcpkg"
export QT_PREFIX="/path/to/Qt/6.x.x/<platform>"
export MAPLIBRE_PREFIX="$HOME/.local/maplibre-native-qt"

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX};${MAPLIBRE_PREFIX}" \
  -DBUILD_TESTS=ON \
  -DBENCHMARK_ENABLE=ON

cmake --build build -j
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | OFF | Build unit tests |
| `BENCHMARK_ENABLE` | OFF | Build benchmarks |
| `DEBUG` | OFF | Debug build with symbols |

---

## Usage

### Running

```bash
./build/QHexWalker
```

### Interface

```
┌────────────────────────────────────────────────────────────────────┐
│  qHexWalker                                                   ─ □ X│
├─────────────┬──────────────────────────────────────────────────────┤
│ WAYPOINTS   │                                                      │
│             │                                                      │
│ 1. [12] ▲▼  │         ╭─────────────────────────╮                  │
│   47.12°N   │        ╱                           ╲                 │
│   38.94°E   │       │      ⬡ ⬡ ⬡ ⬡ ⬡ ⬡          │                │
│             │       │     ⬡ ■ ■ ■ ■ ⬡ ⬡         │                 │
│ 2. [12] ▲▼  │       │    ⬡ ■ ○───────● ⬡        │                 │
│   47.15°N   │       │     ⬡ ■ ■ ■ ■ ⬡ ⬡         │                 │
│   38.97°E   │       │      ⬡ ⬡ ⬡ ⬡ ⬡ ⬡          │                │
│             │        ╲                           ╱                 │
│ [Remove]    │         ╰─────────────────────────╯                  │
│             │                                                      │
├─────────────┤      ○ Start   ● Goal   ■ Wall   ─ Path              │
│ STATISTICS  │                                                      │
│ Cells: 847  │                        MAP                           │
│ Time: 23ms  │                                                      │
│ Path: 156   │                                                      │
└─────────────┴──────────────────────────────────────────────────────┘
```

### Workflow

1. **Click on map** to add waypoints
2. **Generate maze** (optional) for obstacle-based routing
3. **View path** computed automatically between waypoints
4. **Reorder waypoints** using arrow buttons
5. **Monitor statistics** for algorithm performance

---

## Project Structure

```
qHexWalker/
├── src/
│   ├── core/
│   │   ├── main.cpp              # Entry point
│   │   ├── application.h/cpp     # Application class
│   │   ├── entryPoint.h/cpp      # UI initialization
│   │   ├── mapProvider.h/cpp     # Map source management
│   │   └── logger.h              # Logging utilities
│   └── models/
│       ├── astar.h/cpp           # Bidirectional A* (547 lines)
│       ├── dijkstra.h/cpp        # Dijkstra's algorithm
│       ├── h3Model.h/cpp         # Hex cells model
│       ├── h3TargetsModel.h/cpp  # Waypoints model
│       ├── h3Cell.h/cpp          # Cell data class
│       ├── h3Target.h/cpp        # Target data class
│       ├── h3Worker.h/cpp        # Threading wrapper
│       ├── h3MazeGenerator.h/cpp # Maze generation
│       ├── h3MazeAdapter.h/cpp   # Maze orchestration
│       └── helper.h/cpp          # H3 geometry utilities
├── ui/
│   └── main.qml                  # QML interface
├── tests/
│   ├── maze_test.cpp             # Maze tests
│   └── astar_test.cpp            # A* tests
├── benchmark/
│   ├── maze_benchmark.cpp        # Maze benchmarks
│   └── path_benchmark.cpp        # Pathfinding benchmarks
├── CMakeLists.txt
├── vcpkg.json
└── README.md
```

---

## Performance

### A* Pathfinding Complexity

| Approach | Time Complexity | Improvement |
|----------|-----------------|-------------|
| Standard A* | O(b^d) | Baseline |
| Bidirectional A* | O(b^(d/2)) | ~50% reduction |
| + Resolution Hierarchy | O(b^(d/4)) | ~75% reduction |

Where `b` = branching factor (6 for hexagons), `d` = path depth

### Benchmarks

Run benchmarks:
```bash
./build/qhexwalker_benchmark
```

---

## Troubleshooting

| Error | Solution |
|-------|----------|
| `Could not find QMapLibre` | Add MapLibre prefix to `CMAKE_PREFIX_PATH` |
| `Could not find Qt6::Positioning` | Install Qt Positioning module |
| `vcpkg toolchain not found` | Check path to `vcpkg.cmake` |

---

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## License

Distributed under the MIT License. See `LICENSE` for more information.

---

<div align="center">

# qHexWalker

**Поиск пути и визуализация лабиринтов на гексагональных сетках**

[Возможности](#возможности) | [Как это работает](#как-это-работает) | [Установка](#установка) | [Архитектура](#архитектура)

</div>

---

## Обзор

**qHexWalker** — это десктопное Qt 6 приложение, объединяющее систему гексагональной индексации H3 от Uber с картами MapLibre:

- **Интерактивная визуализация** гексагональных сеток разных разрешений
- **Процедурная генерация лабиринтов** алгоритмом Прима
- **Двунаправленный A*** с иерархическим уточнением пути
- **Планирование маршрутов** через множество точек с визуализацией в реальном времени

---

## Возможности

| Функция | Описание |
|---------|----------|
| **H3 Гексагональная сетка** | Визуализация иерархического пространственного индекса (разрешения 3-15) |
| **Двунаправленный A*** | Быстрый поиск пути одновременно с двух концов |
| **Генерация лабиринтов** | Процедурные лабиринты алгоритмом Прима |
| **Многоточечная маршрутизация** | Планирование маршрутов через несколько целей |
| **Визуализация в реальном времени** | Наблюдайте за работой алгоритма |
| **Статистика поиска** | Количество ячеек, время, длина пути |
| **Тёмная тема** | Современный Material Design интерфейс |

---

## Как это работает

### Гексагональная индексация H3

H3 делит Землю на гексагональные ячейки с 16 уровнями разрешения (0-15). Гексагоны обеспечивают равномерное расстояние до соседей и лучшую тесселяцию, чем квадраты.

```
Разрешение  │  Средняя площадь  │  Количество ячеек
────────────┼───────────────────┼───────────────────
     3      │    12,392 км²     │     41,162
     7      │     5.16 км²      │     98 млн
    10      │   14,950 м²       │     23 млрд
    15      │     0.9 м²        │    569 трлн
```

### Алгоритм двунаправленного A*

Поиск пути использует двунаправленный A* с иерархическим уточнением разрешения:

```
┌─────────────────────────────────────────────────────────────────────┐
│                   ДВУНАПРАВЛЕННЫЙ ПОИСК A*                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   СТАРТ ──────────────►  ◄────────────── ЦЕЛЬ                       │
│      │                        │                                     │
│      ▼                        ▼                                     │
│  ┌────────┐              ┌────────┐                                 │
│  │Прямой  │              │Обратный│                                 │
│  │ поиск  │   ВСТРЕЧА    │ поиск  │                                 │
│  │        │◄────────────►│        │                                 │
│  └────────┘              └────────┘                                 │
│      │                        │                                     │
│      └────────────┬───────────┘                                     │
│                   ▼                                                 │
│           ┌───────────────┐                                         │
│           │Оптимальный путь│                                        │
│           └───────────────┘                                         │
│                                                                     │
│  Сложность: O(b^(d/2)) вместо O(b^d)                                │
└─────────────────────────────────────────────────────────────────────┘
```

### Генерация лабиринта (алгоритм Прима)

Генератор создаёт идеальные лабиринты на гексагональных сетках:

```
┌─────────────────────────────────────────────────────────────────────┐
│               РАНДОМИЗИРОВАННЫЙ АЛГОРИТМ ПРИМА                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  1. Создание сетки комнат      2. Инициализация случайной комнатой  │
│     ⬡ · ⬡ · ⬡ · ⬡                  ⬡ · ⬡ · ⬡ · ⬡                 
│     · · · · · · ·                  · · · · · · ·                 
│     ⬡ · ⬡ · ⬡ · ⬡                  ⬡ · ■ · ⬡ · ⬡  ← Старт        
│     · · · · · · ·                  · · · · · · ·                 
│     ⬡ · ⬡ · ⬡ · ⬡                  ⬡ · ⬡ · ⬡ · ⬡                 
│                                                                  
│  3. Добавление стен             4. Соединение комнат             
│     ⬡ · ⬡ · ⬡ · ⬡                  ⬡ · ⬡ · ⬡ · ⬡                 
│     · · W · · · ·                  · · │ · · · ·                 
│     ⬡ W ■ W ⬡ · ⬡                  ⬡───■───⬡ · ⬡                 
│     · · W · · · ·                  · · │ · · · ·                 
│     ⬡ · ⬡ · ⬡ · ⬡                  ⬡ · ⬡ · ⬡ · ⬡                 
│                                                                  
│  5. Повторение до соединения всех комнат                         
│     ⬡───⬡───⬡───⬡                                                
│     │       │                                                    
│     ⬡   ⬡───⬡   ⬡   Идеальный лабиринт с                         
│     │   │       │   единственным решением                        
│     ⬡───⬡   ⬡───⬡                                                
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Архитектура

```
┌─────────────────────────────────────────────────────────────────────┐
│                         СЛОИ ПРИЛОЖЕНИЯ                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                     Слой QML UI                             │    │
│  │  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐     │    │
│  │  │Список точек │  │    Карта     │  │   Статистика    │     │    │
│  │  └─────────────┘  └──────────────┘  └─────────────────┘     │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                   Слой Qt моделей                           │    │
│  │  ┌─────────────────────┐    ┌─────────────────────────┐     │    │
│  │  │      H3Model        │    │    H3TargetsModel       │     │    │
│  │  │ (QAbstractListModel)│    │  (QAbstractListModel)   │     │    │
│  │  └─────────────────────┘    └─────────────────────────┘     │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                Слой бизнес-логики                           │    │
│  │  ┌──────────┐  ┌──────────────┐  ┌───────────────────┐      │    │
│  │  │ H3Worker │  │ H3MazeAdapter│  │   MapProvider     │      │    │
│  │  │ (Поток)  │  │   (Async)    │  │ (Стили/источники) │      │    │
│  │  └──────────┘  └──────────────┘  └───────────────────┘      │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                      │
│                              ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                   Слой алгоритмов                           │    │
│  │  ┌─────────────────────┐    ┌─────────────────────────┐     │    │
│  │  │     H3AStar         │    │   H3MazeGenerator       │     │    │
│  │  │(Двунаправленный A*) │    │  (Алгоритм Прима)       │     │    │
│  │  └─────────────────────┘    └─────────────────────────┘     │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Установка

### Требования

| Компонент | Версия | Примечание |
|-----------|--------|------------|
| CMake | >= 3.19 | Система сборки |
| C++ компилятор | C++20 | GCC 10+, Clang 12+, MSVC 2022+ |
| Qt | 6.5+ | QuickControls2, Sql, Positioning |
| vcpkg | Последняя | Менеджер пакетов |
| MapLibre Native Qt | Последняя | Рендеринг карт |

### Шаги сборки

#### 1. Установка Qt 6

```bash
# Linux (Ubuntu/Debian)
sudo apt install qt6-base-dev qt6-declarative-dev qt6-positioning-dev

# macOS
brew install qt@6

# Windows — используйте Qt Online Installer
```

#### 2. Установка vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
```

#### 3. Сборка MapLibre Native Qt

```bash
git clone https://github.com/maplibre/maplibre-native-qt.git
cd maplibre-native-qt

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/maplibre-native-qt" \
  -DCMAKE_PREFIX_PATH="/путь/к/Qt/6.x.x/<platform>" \
  -DCMAKE_TOOLCHAIN_FILE="/путь/к/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake --build build -j
cmake --install build
```

#### 4. Сборка qHexWalker

```bash
git clone https://github.com/your-username/qHexWalker.git
cd qHexWalker

export VCPKG_ROOT="/путь/к/vcpkg"
export QT_PREFIX="/путь/к/Qt/6.x.x/<platform>"
export MAPLIBRE_PREFIX="$HOME/.local/maplibre-native-qt"

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX};${MAPLIBRE_PREFIX}"

cmake --build build -j
```

---

## Производительность

| Подход | Временная сложность | Улучшение |
|--------|---------------------|-----------|
| Стандартный A* | O(b^d) | Базовый |
| Двунаправленный A* | O(b^(d/2)) | ~50% снижение |
| + Иерархия разрешений | O(b^(d/4)) | ~75% снижение |

Где `b` = коэффициент ветвления (6 для гексагонов), `d` = глубина пути

---

## Решение проблем

| Ошибка | Решение |
|--------|---------|
| `Could not find QMapLibre` | Добавьте путь MapLibre в `CMAKE_PREFIX_PATH` |
| `Could not find Qt6::Positioning` | Установите модуль Qt Positioning |
| `vcpkg toolchain not found` | Проверьте путь к `vcpkg.cmake` |

---

<div align="center">

**Made with C++20, Qt 6, and H3**

</div>
