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

std::unordered_set<H3Index> H3MazeGenerator::generateMaze(const H3Index centerCell, int radius, H3Index &outStart,
                                                          H3Index &outEnd) {
    spdlog::info("Generating H3 maze with center cell, radius={}", radius);

    // Получаем все соты в радиусе
    auto allCells = getCellsInRadius(centerCell, radius);
    if (allCells.empty()) {
        spdlog::error("No cells in radius");
        return {};
    }

    spdlog::info("Total cells in area: {}", allCells.size());

    // Шаг 1: Создаём сетку узлов (каждая 3-я сота)
    std::vector<H3Index> allCellsVec(allCells.begin(), allCells.end());
    std::unordered_set<H3Index, H3IndexHash> nodes;

    for (size_t i = 0; i < allCellsVec.size(); i += 3) {
        nodes.insert(allCellsVec[i]);
    }

    if (nodes.empty()) {
        nodes.insert(centerCell);
    }

    spdlog::info("Created {} nodes for maze", nodes.size());

    // Шаг 2: Выбираем случайный стартовый узел
    std::vector<H3Index> nodesVec(nodes.begin(), nodes.end());
    std::uniform_int_distribution<size_t> startDist(0, nodesVec.size() - 1);
    outStart = nodesVec[startDist(rng_)];

    // Шаг 3: Генерация лабиринта методом Recursive Backtracker
    std::stack<H3Index> stack;
    std::unordered_set<H3Index, H3IndexHash> visited;
    std::unordered_set<H3Index, H3IndexHash> passages;

    stack.push(outStart);
    visited.insert(outStart);
    passages.insert(outStart);

    while (!stack.empty()) {
        H3Index current = stack.top();

        // Находим непосещённых соседей среди узлов
        auto currentNeighbors = getNeighbors(current);
        std::vector<H3Index> unvisitedNeighbors;

        // Проверяем соседей первого уровня
        for (const auto &n1 : currentNeighbors) {
            if (nodes.count(n1) && !visited.count(n1)) {
                unvisitedNeighbors.push_back(n1);
            }

            // Проверяем соседей второго уровня
            auto n1Neighbors = getNeighbors(n1);
            for (const auto &n2 : n1Neighbors) {
                if (n2 != current && nodes.count(n2) && !visited.count(n2)) {
                    // Проверяем что n2 ещё не в списке
                    if (std::find(unvisitedNeighbors.begin(), unvisitedNeighbors.end(), n2) ==
                        unvisitedNeighbors.end()) {
                        unvisitedNeighbors.push_back(n2);
                    }
                }
            }
        }

        if (!unvisitedNeighbors.empty()) {
            // Случайно выбираем соседа
            std::shuffle(unvisitedNeighbors.begin(), unvisitedNeighbors.end(), rng_);
            H3Index next = unvisitedNeighbors[0];

            // Отмечаем как посещённый
            visited.insert(next);
            passages.insert(next);

            // Прокладываем коридор между current и next
            // Ищем общего соседа или путь через промежуточную соту
            auto nextNeighbors = getNeighbors(next);

            // Проверяем общих соседей
            for (const auto &cn : currentNeighbors) {
                if (allCells.count(cn)) {
                    for (const auto &nn : nextNeighbors) {
                        if (cn == nn && !nodes.count(cn)) {
                            // Нашли общего соседа - делаем его проходом
                            passages.insert(cn);
                            goto corridor_found;
                        }
                    }
                }
            }

            // Если нет общего соседа, ищем путь через две соты
            for (const auto &cn : currentNeighbors) {
                if (!allCells.count(cn) || nodes.count(cn))
                    continue;

                auto cnNeighbors = getNeighbors(cn);
                for (const auto &cnn : cnNeighbors) {
                    if (cnn == next) {
                        // Путь через одну промежуточную соту
                        passages.insert(cn);
                        goto corridor_found;
                    }

                    if (allCells.count(cnn) && !nodes.count(cnn)) {
                        auto cnnNeighbors = getNeighbors(cnn);
                        for (const auto &cnnn : cnnNeighbors) {
                            if (cnnn == next) {
                                // Путь через две промежуточные соты
                                passages.insert(cn);
                                passages.insert(cnn);
                                goto corridor_found;
                            }
                        }
                    }
                }
            }

        corridor_found:
            stack.push(next);
        } else {
            // Backtrack
            stack.pop();
        }
    }

    // Шаг 4: Находим наиболее удалённый узел для выхода
    double maxDist = 0.0;
    outEnd = outStart;
    LatLng startCoord;
    cellToLatLng(outStart, &startCoord);

    for (const auto &node : visited) {
        LatLng nodeCoord;
        if (cellToLatLng(node, &nodeCoord) == E_SUCCESS) {
            double dist = greatCircleDistanceM(&startCoord, &nodeCoord);
            if (dist > maxDist) {
                maxDist = dist;
                outEnd = node;
            }
        }
    }

    // Шаг 5: Создаём стены (все соты минус проходы)
    std::unordered_set<H3Index> walls;
    for (const auto &cell : allCells) {
        if (!passages.count(cell)) {
            walls.insert(cell);
        }
    }

    double wallPercent = (walls.size() * 100.0) / allCells.size();
    spdlog::info("Maze generated: {} walls ({:.1f}%), {} passages ({:.1f}%), {} nodes visited", walls.size(),
                 wallPercent, passages.size(), 100.0 - wallPercent, visited.size());

    emit mazeGenerated(walls);
    return walls;
}