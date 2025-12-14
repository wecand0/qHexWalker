#include <benchmark/benchmark.h>

#include <astar.h>
#include <pch.h>

static void BM_StarPath(benchmark::State &state) {
    H3AStar astar;
    const H3Index start = 0x822a87fffffffffL;
    const H3Index end = 0x8eb8a6b13046757L;
    for (const auto &_ : state) {
        auto path = astar.findShortestPath(start, end);
    }
}
BENCHMARK(BM_StarPath);

BENCHMARK_MAIN();