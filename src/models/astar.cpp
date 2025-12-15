#include "astar.h"

#include <queue>

H3AStar::H3AStar(QObject *parent) : QObject(parent) {}

H3AStar::~H3AStar() = default;

std::vector<H3Index> H3AStar::findShortestPath(const H3Index start, const H3Index end) {
    // Проверка валидности индексов
    if (!isValidCell(start) || !isValidCell(end)) {
        throw std::domain_error("Невалидные H3 индексы");
    }

    if (start == end) {
        throw std::runtime_error("Стартовая и конечная точка равны");
    }

    // Сохраняем оригинальные индексы и их разрешения
    const H3Index originalStart = start;
    const H3Index originalEnd = end;
    const int startRes = getResolution(start);
    const int endRes = getResolution(end);

    // Поднимаемся к разрешению 2 если нужно
    const H3Index startRes2 = startRes != 2 ? cellToParentRes2(start) : start;
    const H3Index endRes2 = endRes != 2 ? cellToParentRes2(end) : end;

    if (startRes2 == H3_NULL || endRes2 == H3_NULL) {
        throw std::domain_error("Ошибка преобразования к разрешению 2");
    }

    // Получаем координаты целевой ячейки для эвристики
    LatLng endCoord;
    if (cellToLatLng(endRes2, &endCoord) != E_SUCCESS) {
        throw std::runtime_error("Ошибка получения координат целевой ячейки");
    }

    // Ищем путь на разрешении 2
    const std::vector<H3Index> pathRes2 = findPathAtResolution2(startRes2, endRes2, endCoord);
    if (pathRes2.empty()) {
        return {};
    }

    // Детализируем путь с учетом исходных разрешений
    return refinePath(pathRes2, originalStart, originalEnd, startRes, endRes);
}

std::vector<H3Index> H3AStar::findPathAtResolution2(const H3Index start, const H3Index end, const LatLng &endCoord) {
    // Приоритетная очередь для A* (мин-куча по fScore)
    std::priority_queue<Node, std::vector<Node>, std::greater<>> openSet;

    // g-оценки (реальное расстояние от старта)
    std::unordered_map<H3Index, double, H3IndexHash> gScores;
    gScores.reserve(300);

    // Карта предшественников для восстановления пути
    std::unordered_map<H3Index, H3Index, H3IndexHash> previous;
    previous.reserve(300);

    // Закрытое множество (посещенные узлы)
    std::unordered_set<H3Index, H3IndexHash> closedSet;
    closedSet.reserve(300);

    // Инициализация стартового узла
    gScores[start] = 0.0;
    double startHeuristic = heuristic(start, endCoord);
    openSet.emplace(start, 0, startHeuristic);
    int nodesExplored = 0;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        nodesExplored++;

        // Если достигли конечной точки
        if (current.cell == end) {
            // spdlog::info("Узлов исследовано: {}", nodesExplored);
            return reconstructPath(previous, start, end);
        }

        // Если уже посещали этот узел, пропускаем
        if (closedSet.contains(current.cell)) {
            continue;
        }

        closedSet.insert(current.cell);
        // emit newCell(current.cell);

        // Получаем соседей текущей ячейки
        for (const auto neighbors = getNeighbors(current.cell); const H3Index &neighbor : neighbors) {
            if (closedSet.contains(neighbor)) {
                continue;
            }

            // Вычисляем новую g-оценку (реальное расстояние)
            double edgeDistance = getDistanceBetweenCells(current.cell, neighbor);
            // Если нашли более короткий путь к соседу
            if (double tentativeGScore = gScores[current.cell] + edgeDistance;
                !gScores.contains(neighbor) || tentativeGScore < gScores[neighbor]) {

                // Обновляем путь к соседу
                previous[neighbor] = current.cell;
                gScores[neighbor] = tentativeGScore;

                // Вычисляем f-оценку (g + эвристика)
                double h = heuristic(neighbor, endCoord);
                double fScore = tentativeGScore + h;

                openSet.emplace(neighbor, tentativeGScore, fScore);
            }
        }
    }

    // Путь не найден
    spdlog::warn("Путь не найден между индексами, исследовано {} узлов", nodesExplored);
    return {};
}
std::vector<H3Index> H3AStar::refineEndSegmentGradual(const H3Index prevInPath, const H3Index parentEnd,
                                                      const H3Index originalEnd, const int endRes) {
    std::vector<H3Index> segment;
    // res from 2 to 15
    segment.reserve(15);

    // Строим путь с постепенным увеличением разрешения от 2 до endRes
    H3Index currentCell = parentEnd;

    for (int res = 3; res <= endRes; ++res) {
        // Получаем дочерние ячейки текущей ячейки на разрешении res
        std::vector<H3Index> children = getChildrenAtResolution(currentCell, res);
        if (children.empty()) {
            break;
        }

        // Определяем целевую ячейку на этом разрешении
        H3Index targetAtRes = H3_NULL;
        if (res == endRes) {
            targetAtRes = originalEnd;
        } else {
            // Находим дочернюю ячейку, содержащую originalEnd
            for (const H3Index &child : children) {
                H3Index childParent = H3_NULL;
                cellToParent(originalEnd, res, &childParent);
                if (child == childParent) {
                    targetAtRes = child;
                    break;
                }
            }
        }

        if (targetAtRes == H3_NULL) {
            break;
        }

        // Находим точку входа - ячейку, ближайшую к предыдущему направлению
        if (const H3Index entryCell = findBoundaryCellInDirection(children, targetAtRes, prevInPath);
            entryCell != H3_NULL) {
            // Ищем путь от точки входа к цели на текущем разрешении
            std::vector<H3Index> subPath;
            subPath.reserve(children.size());
            subPath = findLocalPathAtResolution(entryCell, targetAtRes, currentCell);

            segment.insert(segment.end(), subPath.begin(), subPath.end());
            currentCell = targetAtRes;
        }
    }

    return segment;
}
std::vector<H3Index> H3AStar::refineStartSegmentGradual(const H3Index originalStart, const H3Index nextInPath,
                                                        const int startRes) {
    std::vector<H3Index> segment;
    segment.emplace_back(originalStart);

    // Строим путь с постепенным уменьшением разрешения от startRes до 2
    H3Index currentCell = originalStart;

    for (int res = startRes - 1; res >= 2; --res) {
        // Получаем родителя текущей ячейки на разрешении res
        H3Index parent = H3_NULL;
        if (cellToParent(currentCell, res, &parent) != E_SUCCESS) {
            break;
        }

        // Получаем дочерние ячейки родителя на разрешении res+1
        std::vector<H3Index> siblings = getChildrenAtResolution(parent, res + 1);

        // Находим границу - ячейку, ближайшую к направлению движения

        if (const H3Index boundaryCell = findBoundaryCellInDirection(siblings, currentCell, nextInPath);
            boundaryCell != H3_NULL && boundaryCell != currentCell) {
            // Ищем путь к границе на текущем разрешении

            // Добавляем путь (без первого элемента, т.к. Он уже добавлен)
            if (std::vector<H3Index> subPath = findLocalPathAtResolution(currentCell, boundaryCell, parent);
                subPath.size() > 1) {
                segment.insert(segment.end(), subPath.begin() + 1, subPath.end());
                // FIXME do I need this?
                currentCell = boundaryCell;
            }
        }

        // Переходим на уровень выше (меньше разрешение)
        currentCell = parent;
        segment.emplace_back(currentCell);
    }

    return segment;
}
std::vector<H3Index> H3AStar::refinePath(const std::vector<H3Index> &pathRes2, const H3Index originalStart,
                                         const H3Index originalEnd, const int startRes, const int endRes) {
    if (pathRes2.size() < 2) {
        return pathRes2;
    }

    std::vector<H3Index> refinedPath;
    refinedPath.reserve(pathRes2.size() - 1);

    // 1. Детализируем начало пути с плавным переходом разрешений
    if (startRes > 2) {
        std::vector<H3Index> startSegment = refineStartSegmentGradual(originalStart, pathRes2.at(1), startRes);
        refinedPath.insert(refinedPath.end(), startSegment.begin(), startSegment.end());
    } else {
        refinedPath.emplace_back(pathRes2.front());
    }

    // 2. Добавляем средние элементы пути (если есть)
    for (size_t i = 1; i < pathRes2.size() - 1; ++i) {
        refinedPath.emplace_back(pathRes2[i]);
    }

    // 3. Детализируем конец пути с плавным переходом разрешений
    if (endRes > 2 && pathRes2.size() >= 2) {
        std::vector<H3Index> endSegment;
        endSegment.reserve(pathRes2.size() - 1);
        endSegment =
            refineEndSegmentGradual(pathRes2[pathRes2.size() - 2], pathRes2[pathRes2.size() - 1], originalEnd, endRes);
        refinedPath.insert(refinedPath.end(), endSegment.begin(), endSegment.end());
    } else {
        refinedPath.emplace_back(pathRes2.back());
    }

    return refinedPath;
}
H3Index H3AStar::findBoundaryCellInDirection(const std::vector<H3Index> &cells, const H3Index from,
                                             const H3Index direction) {
    if (cells.empty()) {
        return H3_NULL;
    }

    LatLng fromCoord, directionCoord;
    if (cellToLatLng(from, &fromCoord) != E_SUCCESS || cellToLatLng(direction, &directionCoord) != E_SUCCESS) {
        return cells.front();
    }

    // Вычисляем вектор направления
    const double dirLat = directionCoord.lat - fromCoord.lat;
    const double dirLon = directionCoord.lng - fromCoord.lng;

    H3Index bestCell = H3_NULL;
    double maxProjection = -std::numeric_limits<double>::max();

    for (const H3Index &cell : cells) {
        LatLng cellCoord;
        if (cellToLatLng(cell, &cellCoord) != E_SUCCESS) {
            continue;
        }

        // Вычисляем вектор к ячейке
        const double toLat = cellCoord.lat - fromCoord.lat;
        const double toLon = cellCoord.lng - fromCoord.lng;

        // Скалярное произведение (проекция на направление)
        if (const double projection = toLat * dirLat + toLon * dirLon; projection > maxProjection) {
            maxProjection = projection;
            bestCell = cell;
        }
    }

    return bestCell != H3_NULL ? bestCell : cells.front();
}
std::vector<H3Index> H3AStar::findLocalPathAtResolution(H3Index start, H3Index end, H3Index limitParent) {
    if (start == end) {
        return {start};
    }

    LatLng endCoord;
    if (cellToLatLng(end, &endCoord) != E_SUCCESS) {
        return {start, end};
    }

    std::priority_queue<Node, std::vector<Node>, std::greater<>> openSet;
    std::unordered_map<H3Index, double, H3IndexHash> gScores;
    std::unordered_map<H3Index, H3Index, H3IndexHash> previous;
    std::unordered_set<H3Index, H3IndexHash> closedSet;

    gScores[start] = 0.0;
    double startHeuristic = heuristic(start, endCoord);
    openSet.push({start, 0.0, startHeuristic});

    int maxIterations = MAX_CELLS_RES_2;  // Ограничение для предотвращения зацикливания
    int iterations = 0;

    while (!openSet.empty() && iterations++ < maxIterations) {
        Node current = openSet.top();
        openSet.pop();

        if (current.cell == end) {
            return reconstructPath(previous, start, end);
        }

        if (closedSet.contains(current.cell)) {
            continue;
        }

        closedSet.emplace(current.cell);
        for (const auto neighbors = getNeighbors(current.cell); const H3Index &neighbor : neighbors) {
            if (closedSet.contains(neighbor)) {
                continue;
            }

            // Проверяем, что сосед внутри родительской ячейки
            int parentRes = getResolution(limitParent);
            H3Index neighborParent = H3_NULL;
            cellToParent(neighbor, parentRes, &neighborParent);
            if (neighborParent != limitParent) {
                continue;
            }

            const double edgeDistance = getDistanceBetweenCells(current.cell, neighbor);

            if (const double tentativeGScore = gScores[current.cell] + edgeDistance;
                !gScores.contains(neighbor) || tentativeGScore < gScores[neighbor]) {

                previous[neighbor] = current.cell;
                gScores[neighbor] = tentativeGScore;

                double h = heuristic(neighbor, endCoord);
                double fScore = tentativeGScore + h;

                openSet.push({neighbor, tentativeGScore, fScore});
            }
        }
    }

    // Если не нашли путь, возвращаем прямое соединение
    return {start, end};
}
std::vector<H3Index> H3AStar::getChildrenAtResolution(const H3Index parent, const int resolution) {
    std::vector<H3Index> children;

    int64_t childrenSize = 0;
    if (cellToChildrenSize(parent, resolution, &childrenSize) != E_SUCCESS) {
        return children;
    }

    children.resize(childrenSize);
    if (cellToChildren(parent, resolution, children.data()) != E_SUCCESS) {
        return {};
    }

    // Фильтруем нулевые индексы
    std::erase(children, H3_NULL);

    return children;
}

std::array<H3Index, H3AStar::MAX_NEIGHBORS> H3AStar::getNeighbors(const H3Index cell) {
    std::array<H3Index, 7> ring = {};
    if (const H3Error err = gridDisk(cell, 1, ring.data()); err != E_SUCCESS) {
        return {};
    }

    int64_t maxSize = 0;
    if (maxGridDiskSize(1, &maxSize) != E_SUCCESS) {
        return {};
    }

    std::array<H3Index, MAX_NEIGHBORS> neighbors_ = {};
    int count = 0;
    for (auto i = 0; i < maxSize; i++) {
        if (ring[i] != 0 && ring[i] != cell) {
            neighbors_[count++] = ring[i];
        }
    }

    return neighbors_;
}
double H3AStar::getDistanceBetweenCells(const H3Index cell1, const H3Index cell2) {
    LatLng coord1, coord2;

    // Получаем координаты центров ячеек
    // Возвращаем единичное расстояние по умолчанию
    H3Error err = cellToLatLng(cell1, &coord1);
    if (err != E_SUCCESS) {
        return 1.0;
    }
    err = cellToLatLng(cell2, &coord2);
    if (err != E_SUCCESS) {
        return 1.0;
    }

    // Вычисляем расстояние
    return greatCircleDistanceM(&coord1, &coord2);
}
double H3AStar::heuristic(const H3Index cell, const LatLng &targetCoord) {
    LatLng cellCoord;
    if (cellToLatLng(cell, &cellCoord) != E_SUCCESS) {
        return 0.0;
    }

    return greatCircleDistanceM(&cellCoord, &targetCoord);
}

std::vector<H3Index> H3AStar::reconstructPath(const std::unordered_map<H3Index, H3Index, H3IndexHash> &previous,
                                              const H3Index start, const H3Index end) {

    std::vector<H3Index> path;
    path.reserve(previous.size());
    H3Index current = end;

    while (current != start) {
        path.emplace_back(current);
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

H3Index H3AStar::cellToParentRes2(const H3Index index) {
    H3Index indexRes2 = H3_NULL;
    if (cellToParent(index, 2, &indexRes2) != E_SUCCESS) {
        return H3_NULL;
    }
    return indexRes2;
}