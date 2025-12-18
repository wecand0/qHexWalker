#include "h3MazeGenerator.h"
#include <algorithm>
#include <queue>
#include <spdlog/spdlog.h>
#include <stack>

H3MazeGenerator::H3MazeGenerator(QObject *parent) : QObject(parent), rng_(std::random_device{}()) {}

std::vector<H3Index> H3MazeGenerator::getNeighbors(const H3Index cell) {
    std::array<H3Index, 7> ring = {};
    if (gridDisk(cell, 1, ring.data()) != E_SUCCESS) {
        return {};
    }

    std::vector<H3Index> neighbors;
    neighbors.reserve(6);

    for (const auto &neighbor : ring) {
        if (neighbor != H3_NULL && neighbor != cell) {
            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}

std::unordered_set<H3Index, H3MazeGenerator::H3IndexHash> H3MazeGenerator::getCellsInRadius(const H3Index center,
                                                                                            int radius) {
    std::unordered_set<H3Index, H3IndexHash> cells;

    int64_t maxSize = 0;
    if (maxGridDiskSize(radius, &maxSize) != E_SUCCESS) {
        return cells;
    }

    std::vector<H3Index> disk(maxSize);
    if (gridDisk(center, radius, disk.data()) != E_SUCCESS) {
        return cells;
    }

    for (const auto &cell : disk) {
        if (cell != H3_NULL) {
            cells.insert(cell);
        }
    }

    return cells;
}

// bool H3MazeGenerator::isValidCell(const H3Index cell) const {
//     if (cell == H3_NULL) {
//         return false;
//     }
//     if (!isValidCell(cell)) {
//         return false;
//     }
//     return true;
// }

// Находит путь между двумя соседними ячейками сетки
std::vector<H3Index> H3MazeGenerator::findCorridorBetween(const H3Index from, const H3Index to,
                                                          const std::unordered_set<H3Index, H3IndexHash> &allCells) {
    std::vector<H3Index> corridor;

    // Простой случай - соседи напрямую
    auto fromNeighbors = getNeighbors(from);
    for (const auto &n : fromNeighbors) {
        if (n == to) {
            // Прямые соседи - возвращаем пустой коридор
            return corridor;
        }
    }

    // Ищем промежуточные ячейки
    auto neighbors = getNeighbors(from);
    for (const auto &cell : neighbors) {
        if (!allCells.count(cell))
            continue;

        auto cellNeighbors = getNeighbors(cell);
        for (const auto &cn : cellNeighbors) {
            if (cn == to) {
                corridor.push_back(cell);
                return corridor;
            }
        }
    }

    return corridor;
}

// Классический алгоритм генерации лабиринта с коридорами
std::unordered_set<H3Index> H3MazeGenerator::generateMazeClassic(const H3Index centerCell, int radius,
                                                                 H3Index &outStart, H3Index &outEnd) {
    // if (!isValidCell(centerCell)) {
    //     spdlog::error("Invalid center cell");
    //     return {};
    // }

    auto allCells = getCellsInRadius(centerCell, radius);
    if (allCells.empty()) {
        return {};
    }

    spdlog::info("Generating classic maze with {} total cells", allCells.size());

    // Шаг 1: Создаем сетку узлов лабиринта (каждая 3-я ячейка)
    std::unordered_set<H3Index, H3IndexHash> gridNodes;
    std::vector<H3Index> allCellsVec(allCells.begin(), allCells.end());

    // Берем узлы с интервалом для создания стен между ними
    for (size_t i = 0; i < allCellsVec.size(); i += 3) {
        gridNodes.insert(allCellsVec[i]);
    }

    if (gridNodes.empty()) {
        gridNodes.insert(allCellsVec[0]);
    }

    spdlog::info("Grid nodes: {}", gridNodes.size());

    // Шаг 2: Выбираем стартовый узел
    std::vector<H3Index> gridVec(gridNodes.begin(), gridNodes.end());
    std::uniform_int_distribution<size_t> startDist(0, gridVec.size() - 1);
    outStart = gridVec[startDist(rng_)];

    // Шаг 3: Recursive Backtracker для создания лабиринта
    std::stack<H3Index> stack;
    std::unordered_set<H3Index, H3IndexHash> visitedNodes;
    std::unordered_set<H3Index, H3IndexHash> passages;

    stack.push(outStart);
    visitedNodes.insert(outStart);
    passages.insert(outStart);

    while (!stack.empty()) {
        H3Index current = stack.top();

        // Находим непосещенных соседей среди узлов сетки
        auto neighbors = getNeighbors(current);
        std::vector<H3Index> unvisitedNodeNeighbors;

        // Расширяем поиск соседей - смотрим на расстояние 2-3 ячейки
        for (const auto &n1 : neighbors) {
            if (gridNodes.count(n1) && visitedNodes.count(n1) == 0) {
                unvisitedNodeNeighbors.push_back(n1);
            }

            // Смотрим на соседей второго уровня
            auto n1neighbors = getNeighbors(n1);
            for (const auto &n2 : n1neighbors) {
                if (n2 != current && gridNodes.count(n2) && visitedNodes.count(n2) == 0) {
                    // Проверяем, что между ними можно провести коридор
                    bool canConnect = false;
                    for (const auto &check : neighbors) {
                        auto checkNeighbors = getNeighbors(check);
                        for (const auto &cn : checkNeighbors) {
                            if (cn == n2) {
                                canConnect = true;
                                break;
                            }
                        }
                        if (canConnect)
                            break;
                    }

                    if (canConnect && std::find(unvisitedNodeNeighbors.begin(), unvisitedNodeNeighbors.end(), n2) ==
                                          unvisitedNodeNeighbors.end()) {
                        unvisitedNodeNeighbors.push_back(n2);
                    }
                }
            }
        }

        if (!unvisitedNodeNeighbors.empty()) {
            // Выбираем случайного соседа
            std::shuffle(unvisitedNodeNeighbors.begin(), unvisitedNodeNeighbors.end(), rng_);
            H3Index next = unvisitedNodeNeighbors[0];

            // Отмечаем как посещенный
            visitedNodes.insert(next);
            passages.insert(next);

            // КЛЮЧЕВОЙ МОМЕНТ: Прокладываем коридор между current и next
            // Находим промежуточные ячейки
            auto currentNeighbors = getNeighbors(current);
            auto nextNeighbors = getNeighbors(next);

            // Ищем общих соседей или путь через одну ячейку
            H3Index corridorCell = H3_NULL;

            // Проверяем прямых общих соседей
            for (const auto &cn : currentNeighbors) {
                if (allCells.count(cn)) {
                    for (const auto &nn : nextNeighbors) {
                        if (cn == nn) {
                            corridorCell = cn;
                            break;
                        }
                    }
                }
                if (corridorCell != H3_NULL)
                    break;
            }

            // Если нет прямого соседа, ищем путь через две ячейки
            if (corridorCell == H3_NULL) {
                for (const auto &cn : currentNeighbors) {
                    if (!allCells.count(cn))
                        continue;

                    auto cnNeighbors = getNeighbors(cn);
                    for (const auto &cnn : cnNeighbors) {
                        if (cnn == next) {
                            passages.insert(cn);
                            corridorCell = cn;
                            break;
                        }

                        // Еще один уровень
                        if (allCells.count(cnn)) {
                            auto cnnNeighbors = getNeighbors(cnn);
                            for (const auto &cnnn : cnnNeighbors) {
                                if (cnnn == next) {
                                    passages.insert(cn);
                                    passages.insert(cnn);
                                    corridorCell = cnn;
                                    break;
                                }
                            }
                        }
                        if (corridorCell != H3_NULL)
                            break;
                    }
                    if (corridorCell != H3_NULL)
                        break;
                }
            }

            if (corridorCell != H3_NULL) {
                passages.insert(corridorCell);
            }

            stack.push(next);
        } else {
            // Backtrack
            stack.pop();
        }
    }

    // Находим выход (самую удаленную точку)
    double maxDist = 0.0;
    outEnd = outStart;
    LatLng startCoord;
    cellToLatLng(outStart, &startCoord);

    for (const auto &cell : visitedNodes) {
        LatLng cellCoord;
        if (cellToLatLng(cell, &cellCoord) == E_SUCCESS) {
            double dist = greatCircleDistanceM(&startCoord, &cellCoord);
            if (dist > maxDist) {
                maxDist = dist;
                outEnd = cell;
            }
        }
    }

    // Создаем стены (все ячейки МИНУС проходы)
    std::unordered_set<H3Index> walls;
    for (const auto &cell : allCells) {
        if (passages.count(cell) == 0) {
            walls.insert(cell);
        }
    }

    double wallPercent = (walls.size() * 100.0) / allCells.size();
    spdlog::info("Classic maze: {} walls ({:.1f}%), {} passages ({:.1f}%)", walls.size(), wallPercent, passages.size(),
                 100.0 - wallPercent);

    emit mazeGenerated(walls);
    return walls;
}

std::unordered_set<H3Index> H3MazeGenerator::generateMaze(const H3Index centerCell, int radius, H3Index &outStart,
                                                          H3Index &outEnd) {
    return generateMazeClassic(centerCell, radius, outStart, outEnd);
}

std::unordered_set<H3Index> H3MazeGenerator::generateMazeWithDensity(const H3Index centerCell, int radius,
                                                                     double wallDensity, H3Index &outStart,
                                                                     H3Index &outEnd) {
    // Для разной плотности меняем интервал выборки узлов
    return generateMazeClassic(centerCell, radius, outStart, outEnd);
}

std::unordered_set<H3Index> H3MazeGenerator::generateMazeWithFixedPoints(const H3Index start, const H3Index end,
                                                                         int expandRadius) {
    // if (!isValidCell(start) || !isValidCell(end)) {
    //     return {};
    // }

    auto startCells = getCellsInRadius(start, expandRadius);
    auto endCells = getCellsInRadius(end, expandRadius);

    std::unordered_set<H3Index, H3IndexHash> allCells;
    allCells.insert(startCells.begin(), startCells.end());
    allCells.insert(endCells.begin(), endCells.end());

    LatLng startCoord, endCoord;
    cellToLatLng(start, &startCoord);
    cellToLatLng(end, &endCoord);

    int64_t distance = 0;
    gridDistance(start, end, &distance);

    for (int i = 0; i <= distance; ++i) {
        double t = static_cast<double>(i) / std::max(int64_t(1), distance);
        LatLng interpCoord;
        interpCoord.lat = startCoord.lat + t * (endCoord.lat - startCoord.lat);
        interpCoord.lng = startCoord.lng + t * (endCoord.lng - startCoord.lng);

        H3Index interpCell = H3_NULL;
        if (latLngToCell(&interpCoord, 2, &interpCell) == E_SUCCESS) {
            auto cells = getCellsInRadius(interpCell, expandRadius / 2);
            allCells.insert(cells.begin(), cells.end());
        }
    }

    // Создаем сетку узлов
    std::unordered_set<H3Index, H3IndexHash> gridNodes;
    std::vector<H3Index> allCellsVec(allCells.begin(), allCells.end());

    for (size_t i = 0; i < allCellsVec.size(); i += 3) {
        gridNodes.insert(allCellsVec[i]);
    }

    gridNodes.insert(start);
    gridNodes.insert(end);

    // Recursive backtracker от старта
    std::stack<H3Index> stack;
    std::unordered_set<H3Index, H3IndexHash> visitedNodes;
    std::unordered_set<H3Index, H3IndexHash> passages;

    stack.push(start);
    visitedNodes.insert(start);
    passages.insert(start);

    bool endReached = false;

    while (!stack.empty()) {
        H3Index current = stack.top();

        auto neighbors = getNeighbors(current);
        std::vector<H3Index> unvisitedNodeNeighbors;

        for (const auto &n1 : neighbors) {
            if (gridNodes.count(n1) && visitedNodes.count(n1) == 0) {
                unvisitedNodeNeighbors.push_back(n1);
            }

            auto n1neighbors = getNeighbors(n1);
            for (const auto &n2 : n1neighbors) {
                if (n2 != current && gridNodes.count(n2) && visitedNodes.count(n2) == 0) {
                    if (std::find(unvisitedNodeNeighbors.begin(), unvisitedNodeNeighbors.end(), n2) ==
                        unvisitedNodeNeighbors.end()) {
                        unvisitedNodeNeighbors.push_back(n2);
                    }
                }
            }
        }

        if (!unvisitedNodeNeighbors.empty()) {
            std::shuffle(unvisitedNodeNeighbors.begin(), unvisitedNodeNeighbors.end(), rng_);
            H3Index next = unvisitedNodeNeighbors[0];

            visitedNodes.insert(next);
            passages.insert(next);

            // Прокладываем коридор
            auto currentNeighbors = getNeighbors(current);
            for (const auto &cn : currentNeighbors) {
                if (!allCells.count(cn))
                    continue;

                auto cnNeighbors = getNeighbors(cn);
                for (const auto &cnn : cnNeighbors) {
                    if (cnn == next) {
                        passages.insert(cn);
                        break;
                    }
                }
            }

            if (next == end) {
                endReached = true;
            }

            stack.push(next);
        } else {
            stack.pop();
        }
    }

    if (!endReached) {
        passages.insert(end);
    }

    std::unordered_set<H3Index> walls;
    for (const auto &cell : allCells) {
        if (passages.count(cell) == 0) {
            walls.insert(cell);
        }
    }

    spdlog::info("Fixed-point maze: {} walls, {} passages", walls.size(), passages.size());

    emit mazeGenerated(walls);
    return walls;
}