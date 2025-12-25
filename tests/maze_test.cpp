#include <QEventLoop>
#include <QTimer>
#include <gtest/gtest.h>

// clang-format off
#include <pch.h>
#include "h3MazeGenerator.h"
#include "h3MazeAdapter.h"
// clang-format on

// ========================================
// H3MazeGenerator Tests
// ========================================

class MazeGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override { generator = new H3MazeGenerator(nullptr); }

    void TearDown() override { delete generator; }

    H3MazeGenerator *generator{};
};

TEST_F(MazeGeneratorTest, GeneratesNonEmptyMaze) {
    // Генерируем лабиринт с центром в нулевой точке
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);
    ASSERT_NE(centerCell, H3_NULL);

    const int radius = 10;
    auto walls = generator->generateMaze(centerCell, radius);

    // Проверяем, что стены были сгенерированы
    EXPECT_GT(walls.size(), 0) << "Maze should contain walls";
}

TEST_F(MazeGeneratorTest, SmallMazeRadius) {
    // Тест с маленьким радиусом
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 3;
    auto walls = generator->generateMaze(centerCell, radius);

    // Даже для маленького радиуса должны быть стены
    EXPECT_GT(walls.size(), 0);

    // Количество стен должно быть разумным для такого радиуса
    EXPECT_LT(walls.size(), 1000);
}

TEST_F(MazeGeneratorTest, MediumMazeRadius) {
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 20;
    auto walls = generator->generateMaze(centerCell, radius);

    EXPECT_GT(walls.size(), 10);
}

TEST_F(MazeGeneratorTest, LargeMazeRadius) {
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 50;
    auto walls = generator->generateMaze(centerCell, radius);

    EXPECT_GT(walls.size(), 100);
}

TEST_F(MazeGeneratorTest, WallsAreValidH3Indexes) {
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 10;
    auto walls = generator->generateMaze(centerCell, radius);

    // Проверяем, что все стены являются валидными H3 индексами
    for (const auto &wall : walls) {
        EXPECT_NE(wall, H3_NULL) << "Wall should not be H3_NULL";
        EXPECT_TRUE(isValidCell(wall)) << "Wall should be a valid H3 cell";
    }
}

TEST_F(MazeGeneratorTest, WallsHaveSameResolution) {
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    const int resolution = 3;
    ASSERT_EQ(latLngToCell(&ll, resolution, &centerCell), E_SUCCESS);

    const int radius = 10;
    auto walls = generator->generateMaze(centerCell, radius);

    // Все стены должны иметь одинаковое разрешение
    for (const auto &wall : walls) {
        EXPECT_EQ(getResolution(wall), resolution) << "All walls should have the same resolution as center cell";
    }
}

TEST_F(MazeGeneratorTest, DifferentCenterLocations) {
    // Тестируем генерацию лабиринта в разных точках мира
    std::vector<std::pair<double, double>> locations = {
        {0.0, 0.0},           // Экватор, нулевой меридиан
        {55.7558, 37.6173},   // Москва
        {40.7128, -74.0060},  // Нью-Йорк
        {-33.8688, 151.2093}  // Сидней
    };

    for (const auto &[lat, lng] : locations) {
        const LatLng ll{.lat = lat, .lng = lng};
        H3Index centerCell = H3_NULL;
        ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

        const int radius = 5;
        auto walls = generator->generateMaze(centerCell, radius);

        EXPECT_GT(walls.size(), 0) << "Maze should be generated at location (" << lat << ", " << lng << ")";
    }
}

TEST_F(MazeGeneratorTest, GenerateMazeWithEntrances) {
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 10;
    auto result = generator->generateMazeWithEntrances(centerCell, radius);

    // Проверяем наличие стен
    EXPECT_GT(result.walls.size(), 0);

    // Проверяем валидность входа и выхода
    EXPECT_NE(result.entrance, H3_NULL);
    EXPECT_NE(result.exit, H3_NULL);
    EXPECT_TRUE(isValidCell(result.entrance));
    EXPECT_TRUE(isValidCell(result.exit));

    // Вход и выход не должны совпадать
    EXPECT_NE(result.entrance, result.exit);
}

TEST_F(MazeGeneratorTest, NoPentagonsInWalls) {
    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 10;
    auto walls = generator->generateMaze(centerCell, radius);

    // Проверяем, что в стенах нет пентагонов (они могут вызвать проблемы)
    for (const auto &wall : walls) {
        EXPECT_FALSE(isPentagon(wall)) << "Walls should not contain pentagons";
    }
}

// ========================================
// H3MazeAdapter Tests
// ========================================

class MazeAdapterTest : public ::testing::Test {
protected:
    void SetUp() override { adapter = new H3MazeAdapter(nullptr); }

    void TearDown() override { delete adapter; }

    H3MazeAdapter *adapter{};
};

// ========================================
// Integration Tests
// ========================================

TEST(MazePerformanceTest, LargeMazeGeneration) {
    // Тест производительности для большого лабиринта
    H3MazeGenerator generator(nullptr);

    const LatLng ll{.lat = 0.0, .lng = 0.0};
    H3Index centerCell = H3_NULL;
    ASSERT_EQ(latLngToCell(&ll, 3, &centerCell), E_SUCCESS);

    const int radius = 100;

    auto start = std::chrono::high_resolution_clock::now();
    auto walls = generator.generateMaze(centerCell, radius);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    // Генерация не должна занимать слишком много времени
    EXPECT_LT(duration.count(), 3) << "Large maze should generate in under 3 seconds";
    EXPECT_GT(walls.size(), 100) << "Large maze should have many walls";
}