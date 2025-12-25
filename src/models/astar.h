#ifndef Q_HEX_WALKER_ASTAR_H
#define Q_HEX_WALKER_ASTAR_H

class H3AStar final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3AStar)
public:
    explicit H3AStar(QObject *parent = nullptr);
    ~H3AStar() override;

    std::vector<H3Index> findShortestPath(H3Index start, H3Index end);

    // Set blocked (impassable) cells. Call before findShortestPath.
    void setBlockedCells(const std::unordered_set<H3Index> &blocked);

signals:
    void newCell(H3Index h3Index);

private:
    static constexpr uint16_t MAX_CELLS_RES_2{41'162};
    static constexpr int MAX_NEIGHBORS{6};

    struct Node {
        H3Index cell{};
        double gScore{};
        double fScore{};
        bool operator>(const Node &other) const { return fScore > other.fScore; }
    };

    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };

    // Blocked cells (obstacles)
    std::unordered_set<H3Index> blockedCells{};

    std::vector<H3Index> findPathAtResolution3(H3Index start, H3Index end, const LatLng &endCoord);
    std::vector<H3Index> refineEndSegmentGradual(H3Index prevInPath, H3Index parentEnd, H3Index originalEnd,
                                                 int endRes);
    std::vector<H3Index> refineStartSegmentGradual(H3Index originalStart, H3Index nextInPath, int startRes);
    std::vector<H3Index> refinePath(const std::vector<H3Index> &pathRes3, H3Index originalStart, H3Index originalEnd,
                                    int startRes, int endRes);
    static H3Index findBoundaryCellInDirection(const std::vector<H3Index> &cells, H3Index from, H3Index direction);
    std::vector<H3Index> findLocalPathAtResolution(H3Index start, H3Index end, H3Index limitParent);
    static std::vector<H3Index> getChildrenAtResolution(H3Index parent, int resolution);
    std::array<H3Index, MAX_NEIGHBORS> getNeighbors(H3Index cell);
    double getDistanceBetweenCells(H3Index cell1, H3Index cell2);
    double heuristic(H3Index cell, const LatLng &targetCoord);
    std::vector<H3Index> reconstructPath(const std::unordered_map<H3Index, H3Index, H3IndexHash> &previous,
                                         H3Index start, H3Index end);
    static H3Index cellToParentRes3(H3Index index);
    static H3Index cellToChildRes3(H3Index index);
};

#endif  // Q_HEX_WALKER_ASTAR_H