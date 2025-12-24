#pragma once

#include <random>

class H3MazeGenerator final : public QObject {
    Q_OBJECT

public:
    explicit H3MazeGenerator(QObject *parent = nullptr);
    ~H3MazeGenerator() override = default;

    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };

    struct MazeResult {
        std::unordered_set<H3Index> walls;
        H3Index entrance;
        H3Index exit;
    };

    // Главный метод генерации лабиринта
    std::unordered_set<H3Index> generateMaze(H3Index centerCell, int radius);

    // Новый метод с информацией о входе и выходе
    MazeResult generateMazeWithEntrances(H3Index centerCell, int radius);

signals:
    void generationProgress(int percent);
    void mazeGenerated(const std::unordered_set<H3Index> &walls);

private:
    std::mt19937 rng_;

    // Получает всех соседей соты на расстоянии 1
    static std::vector<H3Index> getNeighbors(H3Index cell);

    // Получает все соты в радиусе
    static std::unordered_set<H3Index, H3IndexHash> getCellsInRadius(H3Index center, int radius);

    // Создает сетку комнат с минимальным интервалом 2
    static std::unordered_set<H3Index, H3IndexHash> createRoomGrid(
        const std::unordered_set<H3Index, H3IndexHash> &allCells);

    // Находит стену между двумя комнатами (на расстоянии 2)
    static std::optional<H3Index> findWallBetween(H3Index room1, H3Index room2);

    // Генерирует лабиринт методом Randomized Prim's
    std::unordered_set<H3Index, H3IndexHash> generateMazePrim(const std::unordered_set<H3Index, H3IndexHash> &rooms);

    // Находит комнаты-соседи на расстоянии 2
    static std::vector<H3Index> getRoomNeighbors(
        H3Index room,
        const std::unordered_set<H3Index, H3IndexHash> &rooms);

    // Находит самую удаленную комнату от стартовой через BFS
    static H3Index findFarthestRoom(
        H3Index start,
        const std::unordered_set<H3Index, H3IndexHash> &passages);

    // Проверяет, находится ли ячейка на границе области
    static bool isOnBorder(
        H3Index cell,
        H3Index center,
        int radius);
};