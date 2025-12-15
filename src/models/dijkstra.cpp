#include "dijkstra.h"

#include <queue>

std::vector<H3Index> Dijkstra::findShortestPathDijkstra(H3Index start, H3Index end) {
    // Проверка валидности индексов
    if (!isValidCell(start) || !isValidCell(end)) {
        spdlog::warn("Невалидные H3 индексы");
        return {};
    }

    // Приоритетная очередь для алгоритма Дейкстры (мин-куча)
    std::priority_queue<Node, std::vector<Node>, std::greater<>> pq;

    // Карта расстояний
    std::unordered_map<H3Index, double, H3IndexHash> distances;

    // Карта предшественников для восстановления пути
    std::unordered_map<H3Index, H3Index, H3IndexHash> previous;

    // Посещенные узлы
    std::unordered_set<H3Index, H3IndexHash> visited;

    // Инициализация
    distances[start] = 0.0;
    pq.push({start, 0.0});

    while (!pq.empty()) {
        auto [cell, distance] = pq.top();
        pq.pop();

        // Если достигли конечной точки
        if (cell == end) {
            return reconstructPath(previous, start, end);
        }

        // Если уже посещали этот узел, пропускаем
        if (visited.contains(cell)) {
            continue;
        }

        visited.insert(cell);
        // Получаем соседей текущей ячейки
        std::vector<H3Index> neighbors = getNeighbors(cell);

        for (const H3Index &neighbor : neighbors) {
            if (visited.count(neighbor)) {
                continue;
            }

            // Вычисляем расстояние до соседа
            // Используем расстояние между центрами ячеек
            double edgeDistance = getDistanceBetweenCells(cell, neighbor);
            // Если нашли более короткий путь
            if (double newDistance = distances[cell] + edgeDistance;
                !distances.contains(neighbor) || newDistance < distances[neighbor]) {
                distances[neighbor] = newDistance;
                previous[neighbor] = cell;
                pq.push({neighbor, newDistance});
            }
        }
    }

    // Путь не найден
    spdlog::warn("Путь не найден между индексами");
    return {};
}
double Dijkstra::normalizeLongitude(double lon) {
    while (lon >= 180.0)
        lon -= 360.0;
    while (lon < -180.0)
        lon += 360.0;
    return lon;
}
std::vector<H3Index> Dijkstra::getNeighbors(H3Index cell) {
    std::vector<H3Index> neighbors;

    // H3 v4 API: получаем соседей через gridDisk с k=1
    int64_t maxSize = 0;
    H3Error err = maxGridDiskSize(1, &maxSize);
    if (err != E_SUCCESS) {
        return neighbors;
    }

    std::vector<H3Index> ring(maxSize);
    err = gridDisk(cell, 1, ring.data());
    if (err != E_SUCCESS) {
        return neighbors;
    }

    // Фильтруем результаты (исключаем саму ячейку и нулевые индексы)
    for (const H3Index &idx : ring) {
        if (idx != 0 && idx != cell) {
            neighbors.push_back(idx);
        }
    }

    return neighbors;
}
double Dijkstra::getDistanceBetweenCells(H3Index cell1, H3Index cell2) {
    LatLng coord1, coord2;

    // Получаем координаты центров ячеек
    const H3Error err1 = cellToLatLng(cell1, &coord1);
    const H3Error err2 = cellToLatLng(cell2, &coord2);

    if (err1 != E_SUCCESS || err2 != E_SUCCESS) {
        return 1.0;  // Возвращаем единичное расстояние по умолчанию
    }

    // Вычисляем расстояние по формуле гаверсинуса
    return greatCircleDistanceM(&coord1, &coord2);
}
std::vector<H3Index> Dijkstra::reconstructPath(const std::unordered_map<H3Index, H3Index, H3IndexHash> &previous,
                                               H3Index start, H3Index end) {
    std::vector<H3Index> path;
    H3Index current = end;

    while (current != start) {
        path.push_back(current);
        auto it = previous.find(current);
        if (it == previous.end()) {
            return {};  // Путь не найден
        }
        current = it->second;
    }

    path.emplace_back(start);
    std::ranges::reverse(path);

    return path;
}