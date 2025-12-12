#ifndef Q_HEX_WALKER_ASTAR_H
#define Q_HEX_WALKER_ASTAR_H

// Класс для алгоритма A* на H3
class H3AStar final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3AStar)
public:
    explicit H3AStar(QObject *parent = nullptr);
    ~H3AStar() override;
    std::vector<H3Index> findShortestPath(H3Index start, H3Index end);

signals:
    void newCell(H3Index h3Index);

private:
    static constexpr uint16_t MAX_CELLS_RES_2{5'882};
    static constexpr int MAX_NEIGHBORS{6};
    // Структура для узла в алгоритме A*
    struct Node {
        H3Index cell{};
        double gScore{};  // Реальная стоимость пути от старта
        double fScore{};  // gScore + heuristic (оценка полной стоимости)
        bool operator>(const Node &other) const { return fScore > other.fScore; }
    };

    // Хеш-функция для H3Index
    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };
    std::vector<H3Index> findPathAtResolution2(H3Index start, H3Index end, const LatLng &endCoord);

    // Плавная детализация конечного сегмента: постепенное увеличение разрешения
    // Например: разрешение 2 -> 3 -> 4 -> ... -> endRes
    std::vector<H3Index> refineEndSegmentGradual(H3Index prevInPath, H3Index parentEnd, H3Index originalEnd,
                                                 int endRes);

    // Плавная детализация начального сегмента: постепенное увеличение разрешения
    // Например: разрешение 10 -> 9 -> 8 -> 7 -> ... -> 3 -> 2
    std::vector<H3Index> refineStartSegmentGradual(H3Index originalStart, H3Index nextInPath, int startRes);

    // Детализация пути с плавным переходом между разрешениями
    std::vector<H3Index> refinePath(const std::vector<H3Index> &pathRes2, H3Index originalStart, H3Index originalEnd,
                                    int startRes, int endRes);

    // Найти граничную ячейку в направлении движения
    H3Index findBoundaryCellInDirection(const std::vector<H3Index> &cells, H3Index from, H3Index direction);

    // Локальный поиск на определенном разрешении внутри родительской ячейки
    std::vector<H3Index> findLocalPathAtResolution(H3Index start, H3Index end, H3Index limitParent);

    // Получить все дочерние ячейки на заданном разрешении
    std::vector<H3Index> getChildrenAtResolution(H3Index parent, int resolution);

    // Получить соседей H3-ячейки
    std::array<H3Index, MAX_NEIGHBORS> getNeighbors(H3Index cell);

    double getDistanceBetweenCells(H3Index cell1, H3Index cell2);

    // Эвристическая функция - расстояние по прямой до цели
    double heuristic(H3Index cell, const LatLng &targetCoord);

    // Восстановить путь из карты предшественников
    std::vector<H3Index> reconstructPath(const std::unordered_map<H3Index, H3Index, H3IndexHash> &previous,
                                         H3Index start, H3Index end);

    // Преобразовать индекс к разрешению 2
    H3Index cellToParentRes2(H3Index index);
};

#endif  // Q_HEX_WALKER_ASTAR_H
