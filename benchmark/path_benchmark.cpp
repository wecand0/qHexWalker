#include <benchmark/benchmark.h>

// clang-format off
#include <pch.h>
#include "astar.h"
#include "dijkstra.h"
// clang-format on

static void BM_StarPath(benchmark::State &state) {
    H3AStar astar;
    const H3Index start = 0x822a87fffffffffL;
    const H3Index end = 0x8eb8a6b13046757L;
    std::vector<H3Index> path;
    for (const auto &_ : state) {
        path = astar.findShortestPath(start, end);
    }
}
BENCHMARK(BM_StarPath);

static void BM_DijkstraPath(benchmark::State &state) {
    Dijkstra dijkstra;
    const H3Index start = 0x822a87fffffffffL;
    const H3Index end = 0x82b8a7fffffffffL;
    std::vector<H3Index> path;
    for (const auto &_ : state) {
        path = dijkstra.findShortestPath(start, end);
    }
}
BENCHMARK(BM_DijkstraPath);

BENCHMARK_MAIN();