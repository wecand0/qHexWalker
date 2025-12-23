#pragma once

#include <QObject>
#include <h3/h3api.h>
#include <random>
#include <unordered_set>
#include <vector>

class H3MazeGenerator : public QObject {
    Q_OBJECT

public:
    explicit H3MazeGenerator(QObject *parent = nullptr);
    ~H3MazeGenerator() override = default;

    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };

    // Главный метод генерации лабиринта
    std::unordered_set<H3Index> generateMaze(const H3Index centerCell, int radius);

signals:
    void generationProgress(int percent);
    void mazeGenerated(const std::unordered_set<H3Index> &walls);

private:
    std::mt19937 rng_;

    // Получает всех соседей соты на расстоянии 1
    std::vector<H3Index> getNeighbors(const H3Index cell);

    // Получает все соты в радиусе
    std::unordered_set<H3Index, H3IndexHash> getCellsInRadius(const H3Index center, int radius);
};