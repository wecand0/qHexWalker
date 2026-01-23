#ifndef QHEXWALKER_H3MAZEADAPTER_H
#define QHEXWALKER_H3MAZEADAPTER_H

#include <QFuture>
#include <unordered_set>

class H3MazeGenerator;
class H3MazeAdapter final : public QObject {
    Q_OBJECT
public:
    explicit H3MazeAdapter(QObject *parent = nullptr);
    ~H3MazeAdapter() override;

public slots:
    // Асинхронный запуск генерации лабиринта
    void generateMazeAsync(double lat, double lon, int kRingRadius);

signals:
    // Сигнал для визуализации стен (полигоны для QML)
    void mazePolygonsComputed(const std::vector<QVariantList> &polygons);

    // Сигнал для передачи стен в A* алгоритм
    void mazeWallsGenerated(const std::unordered_set<H3Index> &walls);

    // Сигнал для передачи центра и радиуса лабиринта
    void mazeRadiusComputed(const QGeoCoordinate &center, double radiusMeters);

private:
    // Основная функция генерации (вызывается в отдельном потоке)
    void generateMaze(double lat, double lon, int kRingRadius);

    static std::vector<QVariantList> cellsToMergedPolygons(const std::unordered_set<H3Index> &cells);
    static void deleteStartEndEntities(H3Index start, H3Index end, std::unordered_set<H3Index> &walls);
    static H3Index getMiddleOfRing(const std::vector<H3Index> &distances, H3Index zeroCell);

    H3MazeGenerator *mazeGenerator_{};
    QList<QFuture<void>> pendingFutures_;  // Track async tasks to prevent dangling pointers
};

#endif  // QHEXWALKER_H3MAZEADAPTER_H
