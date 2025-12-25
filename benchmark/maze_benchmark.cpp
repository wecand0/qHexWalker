#include <benchmark/benchmark.h>

// clang-format off
#include <pch.h>
#include "h3MazeGenerator.h"
#include "astar.h"
// clang-format on

// ========================================
// Maze Generation Benchmarks
// ========================================

// Генерация небольшого лабиринта (радиус 5)
static void BM_MazeGeneration_Small(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = 5;

    for (const auto &_ : state) {
        auto walls = generator.generateMaze(centerCell, radius);
        benchmark::DoNotOptimize(walls);
    }

    state.SetLabel("radius=5");
}
BENCHMARK(BM_MazeGeneration_Small);

// Генерация среднего лабиринта (радиус 20)
static void BM_MazeGeneration_Medium(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = 20;

    for (const auto &_ : state) {
        auto walls = generator.generateMaze(centerCell, radius);
        benchmark::DoNotOptimize(walls);
    }

    state.SetLabel("radius=20");
}
BENCHMARK(BM_MazeGeneration_Medium);

// Генерация большого лабиринта (радиус 50)
static void BM_MazeGeneration_Large(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = 50;

    for (const auto &_ : state) {
        auto walls = generator.generateMaze(centerCell, radius);
        benchmark::DoNotOptimize(walls);
    }

    state.SetLabel("radius=50");
}
BENCHMARK(BM_MazeGeneration_Large);

// Генерация очень большого лабиринта (радиус 100)
static void BM_MazeGeneration_VeryLarge(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = 100;

    for (const auto &_ : state) {
        auto walls = generator.generateMaze(centerCell, radius);
        benchmark::DoNotOptimize(walls);
    }

    state.SetLabel("radius=100");
}
BENCHMARK(BM_MazeGeneration_VeryLarge);

// Параметрический бенчмарк: разные радиусы
static void BM_MazeGeneration_Parametric(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = state.range(0);

    for (const auto &_ : state) {
        auto walls = generator.generateMaze(centerCell, radius);
        benchmark::DoNotOptimize(walls);
    }

    state.SetComplexityN(radius);
}
BENCHMARK(BM_MazeGeneration_Parametric)->RangeMultiplier(2)->Range(5, 100)->Complexity();

// Генерация с входом и выходом
static void BM_MazeGeneration_WithEntrances(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = state.range(0);

    for (const auto &_ : state) {
        try {
            auto result = generator.generateMazeWithEntrances(centerCell, radius);
            benchmark::DoNotOptimize(result);
        } catch (const std::exception &e) {
            state.SkipWithError(e.what());
        }
    }
}
BENCHMARK(BM_MazeGeneration_WithEntrances)->Arg(10)->Arg(30)->Arg(50);

// ========================================
// A* with Obstacles Benchmarks
// ========================================

// A* без препятствий (базовый случай)
static void BM_AStar_NoObstacles(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3AStar astar;

    // Generate valid H3 indices at resolution 3
    const LatLng startLL{.lat = 0.0, .lng = 0.0};
    const LatLng endLL{.lat = 0.5, .lng = 0.5};
    H3Index start = H3_NULL;
    H3Index end = H3_NULL;
    latLngToCell(&startLL, 3, &start);
    latLngToCell(&endLL, 3, &end);

    for (const auto &_ : state) {
        auto path = astar.findShortestPath(start, end);
        benchmark::DoNotOptimize(path);
    }

    state.SetLabel("no_obstacles");
}
BENCHMARK(BM_AStar_NoObstacles);

// A* с небольшим лабиринтом
static void BM_AStar_SmallMaze(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    // Генерируем лабиринт один раз
    H3MazeGenerator generator(nullptr);
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    auto walls = generator.generateMaze(centerCell, 10);

    // Создаем A* и устанавливаем препятствия
    H3AStar astar;
    astar.setBlockedCells(walls);

    // Генерируем точки начала и конца внутри лабиринта
    int64_t diskSize = 0;
    maxGridDiskSize(10, &diskSize);
    std::vector<H3Index> disk(diskSize);
    gridDisk(centerCell, 10, disk.data());

    // Находим клетки, которые не являются стенами
    std::vector<H3Index> freeCells;
    for (const auto &cell : disk) {
        if (isValidCell(cell) && !walls.contains(cell)) {
            freeCells.push_back(cell);
        }
    }

    if (freeCells.size() < 2) {
        state.SkipWithError("Not enough free cells for pathfinding");
        return;
    }

    const H3Index start = freeCells.front();
    const H3Index end = freeCells.back();

    for (const auto &_ : state) {
        try {
            auto path = astar.findShortestPath(start, end);
            benchmark::DoNotOptimize(path);
        } catch (...) {
            // Путь может не существовать
        }
    }

    state.SetLabel("maze_r10");
}
BENCHMARK(BM_AStar_SmallMaze);

// A* со средним лабиринтом
static void BM_AStar_MediumMaze(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    auto walls = generator.generateMaze(centerCell, 30);

    H3AStar astar;
    astar.setBlockedCells(walls);

    int64_t diskSize = 0;
    maxGridDiskSize(30, &diskSize);
    std::vector<H3Index> disk(diskSize);
    gridDisk(centerCell, 30, disk.data());

    std::vector<H3Index> freeCells;
    for (const auto &cell : disk) {
        if (isValidCell(cell) && !walls.contains(cell)) {
            freeCells.push_back(cell);
        }
    }

    if (freeCells.size() < 2) {
        state.SkipWithError("Not enough free cells for pathfinding");
        return;
    }

    const H3Index start = freeCells.front();
    const H3Index end = freeCells.back();

    for (const auto &_ : state) {
        try {
            auto path = astar.findShortestPath(start, end);
            benchmark::DoNotOptimize(path);
        } catch (...) {
            // Путь может не существовать
        }
    }

    state.SetLabel("maze_r30");
}
BENCHMARK(BM_AStar_MediumMaze);

// A* с большим лабиринтом
static void BM_AStar_LargeMaze(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    H3MazeGenerator generator(nullptr);
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    auto walls = generator.generateMaze(centerCell, 50);

    H3AStar astar;
    astar.setBlockedCells(walls);

    int64_t diskSize = 0;
    maxGridDiskSize(50, &diskSize);
    std::vector<H3Index> disk(diskSize);
    gridDisk(centerCell, 50, disk.data());

    std::vector<H3Index> freeCells;
    for (const auto &cell : disk) {
        if (isValidCell(cell) && !walls.contains(cell)) {
            freeCells.push_back(cell);
        }
    }

    if (freeCells.size() < 2) {
        state.SkipWithError("Not enough free cells for pathfinding");
        return;
    }

    const H3Index start = freeCells.front();
    const H3Index end = freeCells.back();

    for (const auto &_ : state) {
        try {
            auto path = astar.findShortestPath(start, end);
            benchmark::DoNotOptimize(path);
        } catch (...) {
            // Путь может не существовать
        }
    }

    state.SetLabel("maze_r50");
}
BENCHMARK(BM_AStar_LargeMaze);

// Параметрический A* с разным количеством препятствий
static void BM_AStar_VariableObstacles(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = 20;
    const int obstaclePercent = state.range(0);  // процент препятствий (0-90)

    // Генерируем лабиринт
    H3MazeGenerator generator(nullptr);
    auto allWalls = generator.generateMaze(centerCell, radius);

    // Создаем подмножество препятствий
    std::unordered_set<H3Index> walls;
    int count = 0;
    int targetCount = (allWalls.size() * obstaclePercent) / 100;
    for (const auto &wall : allWalls) {
        if (count >= targetCount)
            break;
        walls.insert(wall);
        count++;
    }

    H3AStar astar;
    astar.setBlockedCells(walls);

    // Получаем свободные клетки
    int64_t diskSize = 0;
    maxGridDiskSize(radius, &diskSize);
    std::vector<H3Index> disk(diskSize);
    gridDisk(centerCell, radius, disk.data());

    std::vector<H3Index> freeCells;
    for (const auto &cell : disk) {
        if (isValidCell(cell) && !walls.contains(cell)) {
            freeCells.push_back(cell);
        }
    }

    if (freeCells.size() < 2) {
        state.SkipWithError("Not enough free cells");
        return;
    }

    const H3Index start = freeCells.front();
    const H3Index end = freeCells.back();

    for (const auto &_ : state) {
        try {
            auto path = astar.findShortestPath(start, end);
            benchmark::DoNotOptimize(path);
        } catch (...) {
            // Путь может не существовать
        }
    }

    state.SetLabel("obstacles_" + std::to_string(obstaclePercent) + "%");
}
BENCHMARK(BM_AStar_VariableObstacles)
    ->Arg(0)    // Без препятствий
    ->Arg(25)   // 25% препятствий
    ->Arg(50)   // 50% препятствий
    ->Arg(75);  // 75% препятствий

// ========================================
// Combined Benchmarks
// ========================================

// Полный цикл: генерация лабиринта + поиск пути
static void BM_MazeGenerationAndPathfinding(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = state.range(0);

    for (const auto &_ : state) {
        // Генерируем лабиринт
        H3MazeGenerator generator(nullptr);
        auto walls = generator.generateMaze(centerCell, radius);

        // Настраиваем A*
        H3AStar astar;
        astar.setBlockedCells(walls);

        // Находим свободные клетки
        int64_t diskSize = 0;
        maxGridDiskSize(radius, &diskSize);
        std::vector<H3Index> disk(diskSize);
        gridDisk(centerCell, radius, disk.data());

        std::vector<H3Index> freeCells;
        for (const auto &cell : disk) {
            if (isValidCell(cell) && !walls.contains(cell)) {
                freeCells.push_back(cell);
            }
        }

        if (freeCells.size() >= 2) {
            try {
                auto path = astar.findShortestPath(freeCells.front(), freeCells.back());
                benchmark::DoNotOptimize(path);
            } catch (...) {
                // Путь может не существовать
            }
        }
    }
}
BENCHMARK(BM_MazeGenerationAndPathfinding)->Arg(10)->Arg(30)->Arg(50);

// Множественные запросы поиска пути в одном лабиринте
static void BM_AStar_MultipleQueries(benchmark::State &state) {
    spdlog::set_level(spdlog::level::err);
    // Генерируем лабиринт один раз
    H3MazeGenerator generator(nullptr);
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    latLngToCell(&ll, 3, &centerCell);

    const int radius = 30;
    auto walls = generator.generateMaze(centerCell, radius);

    H3AStar astar;
    astar.setBlockedCells(walls);

    // Получаем все свободные клетки
    int64_t diskSize = 0;
    maxGridDiskSize(radius, &diskSize);
    std::vector<H3Index> disk(diskSize);
    gridDisk(centerCell, radius, disk.data());

    std::vector<H3Index> freeCells;
    for (const auto &cell : disk) {
        if (isValidCell(cell) && !walls.contains(cell)) {
            freeCells.push_back(cell);
        }
    }

    if (freeCells.size() < 10) {
        state.SkipWithError("Not enough free cells");
        return;
    }

    const int numQueries = state.range(0);

    for (const auto &_ : state) {
        // Выполняем несколько запросов
        for (int i = 0; i < numQueries && i + 1 < freeCells.size(); i++) {
            try {
                auto path = astar.findShortestPath(freeCells[i], freeCells[i + 1]);
                benchmark::DoNotOptimize(path);
            } catch (...) {
                // Путь может не существовать
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * numQueries);
}
BENCHMARK(BM_AStar_MultipleQueries)->Arg(1)->Arg(5)->Arg(10)->Arg(20);

BENCHMARK_MAIN();