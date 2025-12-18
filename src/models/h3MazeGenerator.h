#ifndef QHEXWALKER_H3MAZEGENERATOR_H
#define QHEXWALKER_H3MAZEGENERATOR_H

#include <random>

#include <QObject>
#include <h3/h3api.h>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QObject>
#include <h3/h3api.h>
#include <random>
#include <unordered_set>
#include <vector>

class H3MazeGenerator : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3MazeGenerator)

public:
    explicit H3MazeGenerator(QObject *parent = nullptr);
    ~H3MazeGenerator() override = default;

    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };

    std::unordered_set<H3Index> generateMaze(const H3Index centerCell, int radius, H3Index &outStart, H3Index &outEnd);

    std::unordered_set<H3Index> generateMazeWithDensity(const H3Index centerCell, int radius, double wallDensity,
                                                        H3Index &outStart, H3Index &outEnd);

    std::unordered_set<H3Index> generateMazeWithFixedPoints(const H3Index start, const H3Index end,
                                                            int expandRadius = 5);

signals:
    void generationProgress(int percent);
    void mazeGenerated(const std::unordered_set<H3Index> &walls);

private:
    std::vector<H3Index> getNeighbors(const H3Index cell);

    std::unordered_set<H3Index, H3IndexHash> getCellsInRadius(const H3Index center, int radius);

    std::unordered_set<H3Index> generateMazeClassic(const H3Index centerCell, int radius, H3Index &outStart,
                                                    H3Index &outEnd);

    std::vector<H3Index> findCorridorBetween(const H3Index from, const H3Index to,
                                             const std::unordered_set<H3Index, H3IndexHash> &allCells);

    bool isValidCell(const H3Index cell) const;

    std::mt19937 rng_;
};

#endif  // QHEXWALKER_H3MAZEGENERATOR_H
