#include "h3Worker.h"

#include <ranges>

using namespace H3_VIEWER;
using namespace std::chrono_literals;

namespace {

// Converts a set of H3 cells to a list of merged polygons
std::vector<QVariantList> cellsToMergedPolygons(const std::unordered_set<H3Index> &cells) {
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
    H3Error err = cellsToLinkedMultiPolygon(cellsVec.data(), static_cast<int>(cellsVec.size()), &polygon);
    if (err != E_SUCCESS) {
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

}  // namespace

H3Worker::H3Worker(QObject *parent) : QObject(parent) { astar_ = new H3AStar(); }

H3Worker::~H3Worker() { astar_->deleteLater(); }

void H3Worker::doWork() {
    while (!QThread::currentThread()->isInterruptionRequested()) {
        {
            std::unique_lock lk(mutex_);
            cv_.wait(lk, [this] { return isRequested.load(); });
        }

        // Скопировать запрос и сбросить флаг
        PendingRequest req;
        {
            std::lock_guard lk(mutex_);
            req = pending_;
            pending_.has = false;
        }

        if (!req.has) {
            continue;
        }

        if (!isMazeComputed) {
            // Конвертируем координату в H3
            LatLng ll{.lat = 0, .lng = 0};

            H3Index centerCell = H3_NULL;
            H3Error err = E_SUCCESS;
            err = latLngToCell(&ll, 3, &centerCell);
            if (err != E_SUCCESS) {
                spdlog::warn("{} {}", "Failed to convert center coordinates to H3", describeH3Error(err));
            }
            int radius = 50;
            spdlog::info("Generating maze at center cell with radius {}", radius);

            // Генерируем клеточный лабиринт (возвращает клетки-стены)
            try {
                walls = mazeGenerator_.generateMaze(centerCell, radius);
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

            deleteStartEndEntities(zeroCell, middleCell);

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
            // Визуализация: объединяем все стены в полигоны и отправляем
            if (auto mergedPolygons = cellsToMergedPolygons(walls); !mergedPolygons.empty()) {
                emit mazePolygonsComputed(mergedPolygons);
            }
            isMazeComputed = true;
            spdlog::info("Maze generation complete");
        }

        // Устанавливаем стены в A*
        astar_->setBlockedCells(walls);

        // for (const auto index : walls) {
        //     auto childPolygon = Helper::indexToPolygon(index);
        //     if (!childPolygon.has_value()) {
        //         break;
        //     }
        //     std::this_thread::sleep_for(1ms);
        //     emit cellComputed(getResolution(index), index, childPolygon.value(), false);
        // }

        H3Index prevIndex = req.indexes.front();
        std::vector<H3Index> path;
        for (size_t indexId = 1; indexId < req.indexes.size(); indexId++) {
            try {
                path = astar_->findShortestPath(prevIndex, req.indexes.at(indexId));
                prevIndex = req.indexes.at(indexId);
                for (const auto index : path) {
                    auto childPolygon = Helper::indexToPolygon(index);
                    if (!childPolygon.has_value()) {
                        break;
                    }
                    emit cellComputed(getResolution(index), index, childPolygon.value(), true);
                }
            } catch (const std::exception &e) {
                spdlog::warn("{}", e.what());
            }
        }

        // QVariantList ppCoordinates_;
        //
        // LinkedGeoPolygon polygon;
        // cellsToLinkedMultiPolygon(path.data(), static_cast<int>(path.size()), &polygon);
        //
        // std::vector<LatLng> temp;
        // temp.resize(path.size());
        //
        // auto linkedLatLng = polygon.first->first;
        // while (linkedLatLng) {
        //     temp.emplace_back(linkedLatLng->vertex);
        //     linkedLatLng = linkedLatLng->next;
        // }
        // // the last one == the first to make loop
        // temp.emplace_back(polygon.first->first->vertex);
        // destroyLinkedMultiPolygon(&polygon);
        //
        // ppCoordinates_.reserve(static_cast<qsizetype>(temp.size()));
        // for (auto &&[lat, lng] : temp) {
        //     ppCoordinates_.emplace_back(
        //         QVariant::fromValue(QGeoCoordinate{radsToDegs(lat), radsToDegs(lng), 0}));
        // }
        // emit cellsComputed(ppCoordinates_);

        {
            std::lock_guard lk(mutex_);
            isRequested.store(false);
        }
    }
    emit finished();
}

void H3Worker::requestCell(const std::vector<H3Index> &index) {
    {
        std::lock_guard lk(mutex_);
        if (isRequested.load()) {
            SPDLOG_WARN("cancel <requestCell>");
            return;
        }
        pending_.indexes = index;
        pending_.has = true;
        isRequested.store(true);
    }
    cv_.notify_one();
}
void H3Worker::deleteStartEndEntities(const H3Index start, const H3Index end) {
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
H3Index H3Worker::getMiddleOfRing(const std::vector<H3Index> &distances, const H3Index zeroCell) {
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