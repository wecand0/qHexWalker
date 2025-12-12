//
// Created by Vadim on 06.12.2025.
//

#ifndef Q_HEX_WALKER_DIJKSTRA_H
#define Q_HEX_WALKER_DIJKSTRA_H

class Dijkstra {
public:
    // Найти кратчайший путь между двумя H3-индексами
    std::vector<H3Index> findShortestPathDijkstra(H3Index start, H3Index end);
    // Функция для нормализации долготы в диапазон [-180, 180)
    static double normalizeLongitude(double lon);

private:
    // Хеш-функция для H3Index
    struct H3IndexHash {
        std::size_t operator()(const H3Index &index) const { return std::hash<uint64_t>()(index); }
    };
    // Структура для узла в алгоритме Дейкстры
    struct Node {
        H3Index cell;
        double distance;

        bool operator>(const Node &other) const { return distance > other.distance; }
    };

    // Получить соседей H3-ячейки
    std::vector<H3Index> getNeighbors(H3Index cell);
    // Вычислить расстояние между двумя H3-ячейками
    double getDistanceBetweenCells(H3Index cell1, H3Index cell2);
    // Восстановить путь из карты предшественников
    std::vector<H3Index> reconstructPath(const std::unordered_map<H3Index, H3Index, H3IndexHash> &previous,
                                         H3Index start, H3Index end);
};

#endif  // Q_HEX_WALKER_DIJKSTRA_H
