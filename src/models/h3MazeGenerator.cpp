#include "h3MazeGenerator.h"
#include <algorithm>
#include <queue>
#include <spdlog/spdlog.h>
#include <stack>
#include <unordered_map>

H3MazeGenerator::H3MazeGenerator(QObject *parent) : QObject(parent), rng_(std::random_device{}()) {
}

std::vector<H3Index> H3MazeGenerator::getNeighbors(const H3Index cell) {
    std::array<H3Index, 7> ring = {};
    if (gridDisk(cell, 1, ring.data()) != E_SUCCESS) {
        return {};
    }

    std::vector<H3Index> neighbors;
    neighbors.reserve(6);

    for (const auto &neighbor : ring) {
        if (neighbor != H3_NULL && neighbor != cell) {
            neighbors.emplace_back(neighbor);
        }
    }

    return neighbors;
}

std::unordered_set<H3Index, H3MazeGenerator::H3IndexHash> H3MazeGenerator::getCellsInRadius(const H3Index center,
                                                                                            const int radius) {
    std::unordered_set<H3Index, H3IndexHash> cells;

    int64_t maxSize = 0;
    H3Error err = maxGridDiskSize(radius, &maxSize);
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
        return cells;
    }

    std::vector<H3Index> disk(maxSize);

    err = gridDisk(center, radius, disk.data());
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
        return cells;
    }

    // skip pentagons
    cells.reserve(maxSize);
    for (const auto &cell : disk) {
        if (cell != H3_NULL && !isPentagon(cell) && isValidIndex(cell)) {
            cells.insert(cell);
        }
    }

    return cells;
}

// Создает сетку комнат с минимальным интервалом 2
std::unordered_set<H3Index, H3MazeGenerator::H3IndexHash> H3MazeGenerator::createRoomGrid(
    const std::unordered_set<H3Index, H3IndexHash> &allCells) {

    std::unordered_set<H3Index, H3IndexHash> rooms;
    std::unordered_set<H3Index, H3IndexHash> occupied;

    for (const auto &cell : allCells) {
        if (occupied.contains(cell)) {
            continue;
        }

        // Добавляем комнату
        rooms.insert(cell);

        // Резервируем соседей на расстоянии 1 (будущие потенциальные стены)
        auto neighbors = getNeighbors(cell);
        for (const auto &neighbor : neighbors) {
            occupied.insert(neighbor);
        }
        occupied.insert(cell);
    }

    return rooms;
}

// Находит комнаты-соседи на расстоянии 2
std::vector<H3Index> H3MazeGenerator::getRoomNeighbors(
    const H3Index room,
    const std::unordered_set<H3Index, H3IndexHash> &rooms) {

    std::vector<H3Index> roomNeighbors;

    // Получаем соседей на расстоянии 2
    std::array<H3Index, 19> ring = {};
    if (gridDisk(room, 2, ring.data()) != E_SUCCESS) {
        return roomNeighbors;
    }

    for (const auto &candidate : ring) {
        if (candidate == H3_NULL || candidate == room) {
            continue;
        }

        // Проверяем, что это комната и находится на расстоянии ровно 2
        if (rooms.contains(candidate)) {
            int64_t distance = 0;
            if (gridDistance(room, candidate, &distance) == E_SUCCESS && distance == 2) {
                roomNeighbors.push_back(candidate);
            }
        }
    }

    return roomNeighbors;
}

// Находит стену между двумя комнатами на расстоянии 2
std::optional<H3Index> H3MazeGenerator::findWallBetween(const H3Index room1, const H3Index room2) {
    auto neighbors1 = getNeighbors(room1);
    auto neighbors2 = getNeighbors(room2);

    // Ищем общего соседа - это и будет стена между комнатами
    for (const auto &n1 : neighbors1) {
        for (const auto &n2 : neighbors2) {
            if (n1 == n2) {
                return n1;
            }
        }
    }

    return std::nullopt;
}

// Генерирует лабиринт методом Randomized Prim's
std::unordered_set<H3Index, H3MazeGenerator::H3IndexHash> H3MazeGenerator::generateMazePrim(
    const std::unordered_set<H3Index, H3IndexHash> &rooms,
    const std::unordered_set<H3Index, H3IndexHash> &allCells) {

    if (rooms.empty()) {
        return {};
    }

    // Проходы - изначально пустое множество, будем добавлять комнаты и проходы между ними
    std::unordered_set<H3Index, H3IndexHash> passages;
    std::unordered_set<H3Index, H3IndexHash> visitedRooms;

    // Список стен: пара (стена, непосещенная комната за ней)
    std::vector<std::pair<H3Index, H3Index>> wallList;

    // Выбираем случайную стартовую комнату
    auto roomsVec = std::vector<H3Index>(rooms.begin(), rooms.end());
    std::uniform_int_distribution<size_t> startDist(0, roomsVec.size() - 1);
    H3Index startRoom = roomsVec[startDist(rng_)];

    // Отмечаем стартовую комнату как посещенную и добавляем в проходы
    visitedRooms.insert(startRoom);
    passages.insert(startRoom);

    // Добавляем все стены стартовой комнаты
    auto neighborRooms = getRoomNeighbors(startRoom, rooms);
    for (const auto &neighborRoom : neighborRooms) {
        if (!visitedRooms.contains(neighborRoom)) {
            auto wall = findWallBetween(startRoom, neighborRoom);
            if (wall.has_value()) {
                wallList.emplace_back(wall.value(), neighborRoom);
            }
        }
    }

    // Основной цикл алгоритма Prim's
    while (!wallList.empty()) {
        // Выбираем случайную стену из списка
        std::uniform_int_distribution<size_t> wallDist(0, wallList.size() - 1);
        size_t wallIdx = wallDist(rng_);
        auto [wall, nextRoom] = wallList[wallIdx];

        // Удаляем стену из списка
        wallList.erase(wallList.begin() + static_cast<long>(wallIdx));

        // Если комната за стеной уже посещена, пропускаем
        if (visitedRooms.contains(nextRoom)) {
            continue;
        }

        // Превращаем стену в проход
        passages.insert(wall);
        passages.insert(nextRoom);
        visitedRooms.insert(nextRoom);

        // Добавляем новые стены из новой комнаты
        auto newNeighborRooms = getRoomNeighbors(nextRoom, rooms);
        for (const auto &newNeighborRoom : newNeighborRooms) {
            if (!visitedRooms.contains(newNeighborRoom)) {
                auto newWall = findWallBetween(nextRoom, newNeighborRoom);
                if (newWall.has_value()) {
                    wallList.emplace_back(newWall.value(), newNeighborRoom);
                }
            }
        }
    }

    spdlog::info("Prim's algorithm: visited {} rooms out of {}", visitedRooms.size(), rooms.size());

    return passages;
}

// Находит самую удаленную комнату от стартовой через BFS
H3Index H3MazeGenerator::findFarthestRoom(
    const H3Index start,
    const std::unordered_set<H3Index, H3IndexHash> &passages) {

    std::unordered_map<H3Index, int> distances;
    std::queue<H3Index> queue;

    queue.push(start);
    distances[start] = 0;

    H3Index farthest = start;
    int maxDist = 0;

    while (!queue.empty()) {
        H3Index current = queue.front();
        queue.pop();

        auto neighbors = getNeighbors(current);
        for (const auto &neighbor : neighbors) {
            if (passages.contains(neighbor) && !distances.contains(neighbor)) {
                distances[neighbor] = distances[current] + 1;
                queue.push(neighbor);

                if (distances[neighbor] > maxDist) {
                    maxDist = distances[neighbor];
                    farthest = neighbor;
                }
            }
        }
    }

    spdlog::info("Farthest room is at distance {}", maxDist);
    return farthest;
}

// Проверяет, находится ли ячейка на границе области
bool H3MazeGenerator::isOnBorder(const H3Index cell, const H3Index center, int radius) {
    int64_t distance = 0;
    if (gridDistance(center, cell, &distance) != E_SUCCESS) {
        return false;
    }

    // Ячейка на границе, если она находится на максимальном расстоянии от центра
    return distance >= radius - 1;
}

// Новый метод с информацией о входе и выходе
H3MazeGenerator::MazeResult H3MazeGenerator::generateMazeWithEntrances(const H3Index centerCell, int radius) {
    spdlog::info("Generating H3 maze with entrances, center cell, radius={}", radius);

    // Получаем все соты в радиусе
    auto allCells = getCellsInRadius(centerCell, radius);
    if (allCells.empty()) {
        spdlog::error("No cells in radius");
        return {std::unordered_set<H3Index>(), H3_NULL, H3_NULL};
    }

    spdlog::info("Total cells in area: {}", allCells.size());

    // Шаг 1: Создаем сетку комнат с интервалом 2
    auto rooms = createRoomGrid(allCells);
    spdlog::info("Created {} rooms for maze", rooms.size());

    if (rooms.empty()) {
        return {std::unordered_set<H3Index>(), H3_NULL, H3_NULL};
    }

    // Шаг 2: Генерируем лабиринт методом Prim's
    auto passages = generateMazePrim(rooms, allCells);
    spdlog::info("Generated {} passages", passages.size());

    // Шаг 3: Находим вход (случайная комната на границе)
    std::vector<H3Index> borderRooms;
    for (const auto &room : rooms) {
        if (passages.contains(room) && isOnBorder(room, centerCell, radius)) {
            borderRooms.push_back(room);
        }
    }

    H3Index entrance = H3_NULL;
    if (!borderRooms.empty()) {
        std::uniform_int_distribution<size_t> borderDist(0, borderRooms.size() - 1);
        entrance = borderRooms[borderDist(rng_)];
    } else if (!passages.empty()) {
        entrance = *passages.begin();
    }

    // Шаг 4: Находим выход (самая удаленная комната от входа)
    H3Index exit = findFarthestRoom(entrance, passages);

    // Шаг 5: Создаем стены (все соты минус проходы)
    std::unordered_set<H3Index> walls;
    for (const auto &cell : allCells) {
        if (!passages.contains(cell)) {
            walls.insert(cell);
        }
    }

    double wallPercent = (walls.size() * 100.0) / allCells.size();
    spdlog::info("Maze generated: {} walls ({:.1f}%), {} passages ({:.1f}%)",
                 walls.size(), wallPercent, passages.size(), 100.0 - wallPercent);
    spdlog::info("Entrance: {}, Exit: {}", entrance, exit);

    emit mazeGenerated(walls);
    return {walls, entrance, exit};
}

// Обратная совместимость - старый метод без входа/выхода
std::unordered_set<H3Index> H3MazeGenerator::generateMaze(const H3Index centerCell, int radius) {
    auto result = generateMazeWithEntrances(centerCell, radius);
    return result.walls;
}