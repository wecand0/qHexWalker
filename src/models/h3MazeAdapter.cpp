#include "h3MazeAdapter.h"
#include "h3MazeGenerator.h"

#include <QtConcurrent/qtconcurrentrun.h>

H3MazeAdapter::H3MazeAdapter(QObject *parent) : QObject(parent) { mazeGenerator_ = new H3MazeGenerator(this); }

H3MazeAdapter::~H3MazeAdapter() = default;

void H3MazeAdapter::generateMazeAsync(const double lat, const double lon, const int kRingRadius) {
    auto _ = QtConcurrent::run([=, this] {
        try {
            generateMaze(lat, lon, kRingRadius);
        } catch (const std::exception &e) {
            spdlog::critical("Maze generation failed: {}", e.what());
        }
    });
}

void H3MazeAdapter::generateMaze(const double lat, const double lon, const int kRingRadius) {
    std::unordered_set<H3Index> walls;
    // Конвертируем координату в H3
    const LatLng ll{.lat = lat, .lng = lon};

    H3Index centerCell = H3_NULL;
    H3Error err = E_SUCCESS;
    err = latLngToCell(&ll, 3, &centerCell);
    if (err != E_SUCCESS) {
        spdlog::warn("{} {}", "Failed to convert center coordinates to H3", describeH3Error(err));
    }
    int radius = kRingRadius;
    spdlog::info("Generating maze at center cell with radius {}", radius);

    // Генерируем клеточный лабиринт (возвращает клетки-стены)
    try {
        walls = mazeGenerator_->generateMaze(centerCell, radius);
    } catch (const std::exception &e) {
        spdlog::error("{}", e.what());
    }

    spdlog::info("Cell maze generated: {} wall cells", walls.size());

    // кольцо вокруг лабиринта с радиусом на 1 больше
    int64_t ringSize = 0;
    radius++;
    err = maxGridDiskSize(radius, &ringSize);
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }
    std::vector<H3Index> ring1st(ringSize);
    err = gridRing(centerCell, radius, ring1st.data());
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }
    ring1st.shrink_to_fit();

    const H3Index zeroCell = ring1st.front();
    const H3Index middleCell = getMiddleOfRing(ring1st, zeroCell);

    deleteStartEndEntities(zeroCell, middleCell, walls);

    // The first cell is the entrance, skip it.
    for (auto const &cellId : ring1st | std::views::drop(1)) {
        if (cellId == middleCell) {
            continue;
        }
        if (!isValidCell(cellId)) {
            continue;
        }
        if (walls.contains(cellId)) {
            continue;
        }
        walls.insert(cellId);
    }

    // Marshal results back to GUI thread
    QMetaObject::invokeMethod(
        this,
        [this, walls] {
            // Визуализация: объединяем все стены в полигоны и отправляем
            if (const auto mergedPolygons = cellsToMergedPolygons(walls); !mergedPolygons.empty()) {
                emit mazePolygonsComputed(mergedPolygons);
                spdlog::info("Maze polygons computed and emitted");
            }

            // Передаем стены для A* алгоритма
            emit mazeWallsGenerated(walls);
            spdlog::info("Maze generation complete: {} wall cells", walls.size());
        },
        Qt::QueuedConnection);
}

std::vector<QVariantList> H3MazeAdapter::cellsToMergedPolygons(const std::unordered_set<H3Index> &cells) {
    if (cells.empty()) {
        return {};
    }

    // Filter valid cells - remove H3_NULL, pentagons, and ensure same resolution
    std::vector<H3Index> cellsVec;
    cellsVec.reserve(cells.size());

    int targetRes = -1;
    for (const auto &cell : cells) {
        if (cell == H3_NULL || !isValidCell(cell)) {
            continue;
        }
        if (isPentagon(cell)) {
            continue;  // Skip pentagons - they cause issues with cellsToLinkedMultiPolygon
        }

        const int res = getResolution(cell);
        if (targetRes == -1) {
            targetRes = res;
        } else if (res != targetRes) {
            continue;  // Skip cells with different resolution
        }

        cellsVec.emplace_back(cell);
    }

    if (cellsVec.empty()) {
        spdlog::warn("No valid cells to convert to polygons");
        return {};
    }

    spdlog::info("Converting {} valid cells (res={}) to polygons", cellsVec.size(), targetRes);

    LinkedGeoPolygon polygon{};
    if (const H3Error err = cellsToLinkedMultiPolygon(cellsVec.data(), static_cast<int>(cellsVec.size()), &polygon);
        err != E_SUCCESS) {
        spdlog::warn("cellsToLinkedMultiPolygon failed: {} (cells count: {})", describeH3Error(err), cellsVec.size());
        return {};
    }

    // Process all polygons in the linked list
    std::vector<QVariantList> result;
    const LinkedGeoPolygon *currentPoly = &polygon;
    while (currentPoly != nullptr) {
        // Process outer loop (first loop is the outer boundary)
        if (currentPoly->first != nullptr) {
            QVariantList polyPath;

            const LinkedLatLng *currentVertex = currentPoly->first->first;
            double prevLng = 0.0;
            bool isFirst = true;

            while (currentVertex != nullptr) {
                const double lat = radsToDegs(currentVertex->vertex.lat);
                double lng = radsToDegs(currentVertex->vertex.lng);

                // Обработка антимеридиана - корректируем долготу если скачок > 180°
                if (!isFirst) {
                    if (const double delta = lng - prevLng; delta > 180.0) {
                        lng -= 360.0;
                    } else if (delta < -180.0) {
                        lng += 360.0;
                    }
                }

                polyPath.emplace_back(QVariant::fromValue(QGeoCoordinate{lat, lng, 0}));
                prevLng = lng;
                isFirst = false;
                currentVertex = currentVertex->next;
            }

            // Close the polygon by adding the first point at the end
            if (!polyPath.isEmpty() && currentPoly->first->first != nullptr) {
                const double lat = radsToDegs(currentPoly->first->first->vertex.lat);
                double lng = radsToDegs(currentPoly->first->first->vertex.lng);

                // Корректируем замыкающую точку относительно последней
                if (const double delta = lng - prevLng; delta > 180.0) {
                    lng -= 360.0;
                } else if (delta < -180.0) {
                    lng += 360.0;
                }

                polyPath.emplace_back(QVariant::fromValue(QGeoCoordinate{lat, lng, 0}));
            }

            if (!polyPath.isEmpty()) {
                result.emplace_back(std::move(polyPath));
            }
        }

        currentPoly = currentPoly->next;
    }

    destroyLinkedMultiPolygon(&polygon);

    spdlog::info("Converted {} cells to {} polygons", cells.size(), result.size());
    return result;
}
void H3MazeAdapter::deleteStartEndEntities(const H3Index start, const H3Index end, std::unordered_set<H3Index> &walls) {
    // start
    int64_t maxSize = 0;
    constexpr int kRingSize = 3;
    H3Error err = maxGridDiskSize(kRingSize, &maxSize);
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }

    std::vector<H3Index> disk(maxSize);
    err = gridDisk(start, kRingSize, disk.data());
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }
    for (auto d : disk) {
        if (walls.contains(d)) {
            walls.erase(d);
        }
    }

    // end
    std::vector<H3Index> disk2(maxSize);
    err = gridDisk(end, kRingSize, disk2.data());
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }
    for (auto d : disk2) {
        if (walls.contains(d)) {
            walls.erase(d);
        }
    }
}
H3Index H3MazeAdapter::getMiddleOfRing(const std::vector<H3Index> &distances, const H3Index zeroCell) {
    LatLng zeroLatLng;
    cellToLatLng(zeroCell, &zeroLatLng);
    double dist = 0;
    LatLng ll;
    H3Index middleCell = H3_NULL;
    H3Error err = E_SUCCESS;
    for (auto const &cellId : distances | std::views::drop(1)) {
        if (!isValidCell(cellId)) {
            continue;
        }
        err = cellToLatLng(cellId, &ll);
        if (err != E_SUCCESS) {
            spdlog::warn(describeH3Error(err));
        }
        if (const auto distTemp = greatCircleDistanceM(&zeroLatLng, &ll); distTemp > dist) {
            dist = distTemp;
            middleCell = cellId;
        }
    }
    return middleCell;
}