#include "astar.h"

#include <queue>

H3AStar::H3AStar(QObject *parent) : QObject(parent) {}

H3AStar::~H3AStar() = default;

void H3AStar::setBlockedCells(const std::unordered_set<H3Index> &blocked) { blockedCells = blocked; }

std::vector<H3Index> H3AStar::findShortestPath(const H3Index start, const H3Index end) {
    if (!isValidCell(start) || !isValidCell(end)) {
        throw std::domain_error("Invalid H3 indexes");
    }
    if (start == end) {
        throw std::runtime_error("Start and end points are the same");
    }

    if (!blockedCells.empty() && (blockedCells.contains(start) || blockedCells.contains(end))) {
        throw std::runtime_error("Start or end cell is blocked");
    }

    const H3Index originalStart = start;
    const H3Index originalEnd = end;
    const int startRes = getResolution(start);
    const int endRes = getResolution(end);

    const H3Index startRes3 = startRes != 3 ? cellToParentRes3(start) : start;
    const H3Index endRes3 = endRes != 3 ? cellToParentRes3(end) : end;

    if (startRes3 == H3_NULL || endRes3 == H3_NULL) {
        throw std::domain_error("Error converting to resolution 3");
    }
    if (!blockedCells.empty() && (blockedCells.contains(startRes3) || blockedCells.contains(endRes3))) {
        return {};  // Coarse start/end blocked
    }

    LatLng endCoord;
    if (cellToLatLng(endRes3, &endCoord) != E_SUCCESS) {
        throw std::runtime_error("Error getting target coordinates");
    }

    const std::vector<H3Index> pathRes3 = findPathAtResolution3(startRes3, endRes3, endCoord);
    if (pathRes3.empty()) {
        return {};
    }

    return refinePath(pathRes3, originalStart, originalEnd, startRes, endRes);
}

std::vector<H3Index> H3AStar::findPathAtResolution3(const H3Index start, const H3Index end, const LatLng &endCoord) {
    // Bidirectional A*: поиск одновременно с двух сторон

    // Forward search (от start к end)
    std::priority_queue<Node, std::vector<Node>, std::greater<>> forwardOpen;
    std::unordered_map<H3Index, double, H3IndexHash> forwardG;
    std::unordered_map<H3Index, H3Index, H3IndexHash> forwardPrev;
    std::unordered_set<H3Index, H3IndexHash> forwardClosed;

    // Backward search (от end к start)
    std::priority_queue<Node, std::vector<Node>, std::greater<>> backwardOpen;
    std::unordered_map<H3Index, double, H3IndexHash> backwardG;
    std::unordered_map<H3Index, H3Index, H3IndexHash> backwardPrev;
    std::unordered_set<H3Index, H3IndexHash> backwardClosed;

    // Резервирование памяти (по ~150 на каждое направление)
    forwardG.reserve(150);
    forwardPrev.reserve(150);
    forwardClosed.reserve(150);
    backwardG.reserve(150);
    backwardPrev.reserve(150);
    backwardClosed.reserve(150);

    // Инициализация
    LatLng startCoord;
    if (cellToLatLng(start, &startCoord) != E_SUCCESS) {
        return {};
    }

    forwardG[start] = 0.0;
    forwardOpen.emplace(Node{start, 0.0, heuristic(start, endCoord)});

    backwardG[end] = 0.0;
    backwardOpen.emplace(Node{end, 0.0, heuristic(end, startCoord)});

    // Переменные для отслеживания встречи
    H3Index meetingPoint = H3_NULL;
    double bestPathCost = std::numeric_limits<double>::infinity();
    int nodesExplored = 0;

    // Попеременный поиск с двух сторон
    while (!forwardOpen.empty() && !backwardOpen.empty()) {
        // Проверка терминации: если лучший путь уже найден
        const double forwardMin = forwardOpen.top().fScore;
        const double backwardMin = backwardOpen.top().fScore;

        if (forwardMin + backwardMin >= bestPathCost) {
            break;  // Оптимальный путь найден
        }

        // === FORWARD STEP ===
        if (!forwardOpen.empty()) {
            Node current = forwardOpen.top();
            forwardOpen.pop();
            nodesExplored++;

            if (!forwardClosed.contains(current.cell)) {
                forwardClosed.insert(current.cell);
                emit newCell(current.cell);

                // Проверка встречи: нашли ли мы эту ячейку с обратной стороны?
                if (backwardClosed.contains(current.cell)) {
                    double pathCost = forwardG[current.cell] + backwardG[current.cell];
                    if (pathCost < bestPathCost) {
                        bestPathCost = pathCost;
                        meetingPoint = current.cell;
                    }
                }

                // Расширение узла
                for (const auto neighbors = getNeighbors(current.cell); const H3Index &neighbor : neighbors) {
                    if (neighbor == H3_NULL || forwardClosed.contains(neighbor) || blockedCells.contains(neighbor)) {
                        continue;
                    }

                    double edgeDistance = getDistanceBetweenCells(current.cell, neighbor);
                    double tentativeG = forwardG[current.cell] + edgeDistance;

                    if (!forwardG.contains(neighbor) || tentativeG < forwardG[neighbor]) {
                        forwardPrev[neighbor] = current.cell;
                        forwardG[neighbor] = tentativeG;
                        double h = heuristic(neighbor, endCoord);
                        forwardOpen.emplace(Node{neighbor, tentativeG, tentativeG + h});
                    }
                }
            }
        }

        // === BACKWARD STEP ===
        if (!backwardOpen.empty()) {
            Node current = backwardOpen.top();
            backwardOpen.pop();
            nodesExplored++;

            if (!backwardClosed.contains(current.cell)) {
                backwardClosed.insert(current.cell);
                emit newCell(current.cell);

                // Проверка встречи
                if (forwardClosed.contains(current.cell)) {
                    double pathCost = forwardG[current.cell] + backwardG[current.cell];
                    if (pathCost < bestPathCost) {
                        bestPathCost = pathCost;
                        meetingPoint = current.cell;
                    }
                }

                // Расширение узла
                for (const auto neighbors = getNeighbors(current.cell); const H3Index &neighbor : neighbors) {
                    if (neighbor == H3_NULL || backwardClosed.contains(neighbor) || blockedCells.contains(neighbor)) {
                        continue;
                    }

                    double edgeDistance = getDistanceBetweenCells(current.cell, neighbor);
                    double tentativeG = backwardG[current.cell] + edgeDistance;

                    if (!backwardG.contains(neighbor) || tentativeG < backwardG[neighbor]) {
                        backwardPrev[neighbor] = current.cell;
                        backwardG[neighbor] = tentativeG;
                        double h = heuristic(neighbor, startCoord);
                        backwardOpen.emplace(Node{neighbor, tentativeG, tentativeG + h});
                    }
                }
            }
        }
    }

    // Реконструкция пути через точку встречи
    if (meetingPoint != H3_NULL) {
        // Путь от start до meetingPoint
        std::vector<H3Index> forwardPath;
        H3Index current = meetingPoint;
        while (current != start) {
            forwardPath.push_back(current);
            auto it = forwardPrev.find(current);
            if (it == forwardPrev.end()) {
                break;
            }
            current = it->second;
        }
        forwardPath.push_back(start);
        std::ranges::reverse(forwardPath);

        // Путь от meetingPoint до end
        std::vector<H3Index> backwardPath;
        current = meetingPoint;
        while (current != end) {
            auto it = backwardPrev.find(current);
            if (it == backwardPrev.end()) {
                break;
            }
            current = it->second;
            backwardPath.push_back(current);
        }
        backwardPath.push_back(end);

        // Объединение путей (без дублирования meetingPoint)
        forwardPath.insert(forwardPath.end(), backwardPath.begin(), backwardPath.end());

        spdlog::info("Bidirectional A* found path, explored {} nodes (meeting at 0x{:x})", nodesExplored, meetingPoint);
        return forwardPath;
    }

    spdlog::warn("No path found, explored {} nodes", nodesExplored);
    return {};
}

std::vector<H3Index> H3AStar::findLocalPathAtResolution(H3Index start, H3Index end, H3Index limitParent) const {
    if (start == end) {
        return {start};
    }
    if (blockedCells.contains(start) || blockedCells.contains(end)) {
        return {};  // Blocked endpoint in local search
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
    openSet.push({start, 0.0, heuristic(start, endCoord)});

    int maxIterations = MAX_CELLS_RES_2;
    int iterations = 0;

    while (!openSet.empty() && iterations++ < maxIterations) {
        Node current = openSet.top();
        openSet.pop();

        if (current.cell == end) {
            return reconstructPath(previous, start, end);
        }

        if (closedSet.contains(current.cell))
            continue;
        closedSet.insert(current.cell);

        for (const auto neighbors = getNeighbors(current.cell); const H3Index &neighbor : neighbors) {
            if (neighbor == H3_NULL)
                continue;

            if (closedSet.contains(neighbor) || blockedCells.contains(neighbor)) {
                continue;
            }

            int parentRes = getResolution(limitParent);
            H3Index neighborParent = H3_NULL;
            cellToParent(neighbor, parentRes, &neighborParent);
            if (neighborParent != limitParent) {
                continue;
            }

            const double edgeDistance = getDistanceBetweenCells(current.cell, neighbor);
            const double tentativeGScore = gScores[current.cell] + edgeDistance;

            if (!gScores.contains(neighbor) || tentativeGScore < gScores[neighbor]) {
                previous[neighbor] = current.cell;
                gScores[neighbor] = tentativeGScore;
                double h = heuristic(neighbor, endCoord);
                openSet.push({neighbor, tentativeGScore, tentativeGScore + h});
            }
        }
    }

    // Fallback if no path (could be due to blocks)
    return {start, end};
}
std::vector<H3Index> H3AStar::refineEndSegmentGradual(const H3Index prevInPath, const H3Index parentEnd,
                                                      const H3Index originalEnd, const int endRes) {
    std::vector<H3Index> segment;
    // res from 2 to 15
    segment.reserve(15);

    // Строим путь с постепенным увеличением разрешения от 2 до endRes
    H3Index currentCell = parentEnd;

    for (int res = 4; res <= endRes; ++res) {
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
                                                        const int startRes) const {
    std::vector<H3Index> segment;
    segment.emplace_back(originalStart);

    // Строим путь с постепенным уменьшением разрешения от startRes до 2
    H3Index currentCell = originalStart;

    for (int res = startRes - 1; res >= 3; --res) {
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
std::vector<H3Index> H3AStar::refinePath(const std::vector<H3Index> &pathRes3, const H3Index originalStart,
                                         const H3Index originalEnd, const int startRes, const int endRes) {
    if (pathRes3.size() < 2) {
        return {};
    }

    std::vector<H3Index> refinedPath;
    refinedPath.reserve(pathRes3.size() - 1);

    // 1. Детализируем начало пути с плавным переходом разрешений
    if (startRes > 3) {
        std::vector<H3Index> startSegment = refineStartSegmentGradual(originalStart, pathRes3.at(1), startRes);
        refinedPath.insert(refinedPath.end(), startSegment.begin(), startSegment.end());
    } else {
        refinedPath.emplace_back(pathRes3.front());
    }

    // 2. Добавляем средние элементы пути (если есть)
    for (size_t i = 1; i < pathRes3.size() - 1; ++i) {
        refinedPath.emplace_back(pathRes3[i]);
    }

    // 3. Детализируем конец пути с плавным переходом разрешений
    if (endRes > 3 && pathRes3.size() >= 2) {
        std::vector<H3Index> endSegment;
        endSegment.reserve(pathRes3.size() - 1);
        endSegment =
            refineEndSegmentGradual(pathRes3[pathRes3.size() - 2], pathRes3[pathRes3.size() - 1], originalEnd, endRes);
        refinedPath.insert(refinedPath.end(), endSegment.begin(), endSegment.end());
    } else {
        refinedPath.emplace_back(pathRes3.back());
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

H3Index H3AStar::cellToParentRes3(const H3Index index) {
    H3Index indexRes3 = H3_NULL;
    if (cellToParent(index, 3, &indexRes3) != E_SUCCESS) {
        return H3_NULL;
    }
    return indexRes3;
}
H3Index H3AStar::cellToChildRes3(const H3Index index) {
    H3Index indexRes3 = H3_NULL;
    if (cellToCenterChild(index, 3, &indexRes3) != E_SUCCESS) {
        return H3_NULL;
    }
    return indexRes3;
}