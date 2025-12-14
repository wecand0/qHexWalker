#include <gtest/gtest.h>

// clang-format off
#include <pch.h>
#include "astar.h"
// clang-format on

TEST(astar_test, zero_path) {
    H3AStar astar;
    const H3Index start = 0x822a87fffffffffL;
    const H3Index end = 0x822a87fffffffffL;
    EXPECT_THROW(astar.findShortestPath(start, end), std::runtime_error);
}

TEST(astar_test, path_from_res2_to_n) {
    H3AStar astar;
    const H3Index start = 0x822a87fffffffffL;
    const H3Index end = 0x8eb8a6b13046757L;
    auto path = astar.findShortestPath(start, end);
    EXPECT_EQ(path.size(), 78);
}

TEST(astar_test, path_from_res15_to_n) {
    H3AStar astar;
    const H3Index start = 0x8f2a80cd1aa63a6L;
    const H3Index end = 0x8eb8a6b13046757L;
    auto path = astar.findShortestPath(start, end);
    EXPECT_EQ(path.size(), 107);
}