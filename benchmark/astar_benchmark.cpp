#include <benchmark/benchmark.h>

// clang-format off
#include <pch.h>
#include "astar.h"
// clang-format on

static void BM_StarPath(benchmark::State &state) {
    H3AStar astar;
    const H3Index start = 0x822a87fffffffffL;
    const H3Index end = 0x8eb8a6b13046757L;
    std::vector<H3Index> path;
    for (const auto &_ : state) {
        path = astar.findShortestPath(start, end);
        for (const auto &__ : path) {
        }
        path.clear();
    }
}
BENCHMARK(BM_StarPath);

BENCHMARK_MAIN();