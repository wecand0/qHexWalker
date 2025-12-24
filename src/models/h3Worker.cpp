#include "h3Worker.h"

#include <ranges>

using namespace H3_VIEWER;
using namespace std::chrono_literals;

namespace {

// Converts a set of H3 cells to a list of merged polygons
std::vector<QVariantList> cellsToMergedPolygons(const std::unordered_set<H3Index> &cells) {
    std::vector<QVariantList> result;

    if (cells.empty()) {
        return result;
    }

    // Convert set to vector for H3 API
    std::vector<H3Index> cellsVec(cells.begin(), cells.end());
    cellsVec.shrink_to_fit();
    //cellsVec.erase(cellsVec.begin(), cellsVec.begin() + 1);

    LinkedGeoPolygon polygon{};
    H3Error err;
    err = cellsToLinkedMultiPolygon(cellsVec.data(), static_cast<int>(cellsVec.size()), &polygon);

    if (err != E_SUCCESS) {
        spdlog::warn("cellsToLinkedMultiPolygon failed: {}", describeH3Error(err));
        return result;
    }

    // Process all polygons in the linked list
    const LinkedGeoPolygon *currentPoly = &polygon;
    while (currentPoly != nullptr) {
        // Process outer loop (first loop is the outer boundary)
        if (currentPoly->first != nullptr) {
            QVariantList polyPath;

            const LinkedLatLng *currentVertex = currentPoly->first->first;
            while (currentVertex != nullptr) {
                const double lat = radsToDegs(currentVertex->vertex.lat);
                const double lng = radsToDegs(currentVertex->vertex.lng);
                polyPath.emplace_back(QVariant::fromValue(QGeoCoordinate{lat, lng, 0}));
                currentVertex = currentVertex->next;
            }

            // Close the polygon by adding the first point at the end
            if (!polyPath.isEmpty() && currentPoly->first->first != nullptr) {
                const double lat = radsToDegs(currentPoly->first->first->vertex.lat);
                const double lng = radsToDegs(currentPoly->first->first->vertex.lng);
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

H3Worker::H3Worker(QObject *parent) : QObject(parent) {
    astar_ = new H3AStar();
}

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
            const QGeoCoordinate center{0, 0, 0};
            int radius = 20;

            // Конвертируем координату в H3
            LatLng ll{.lat = degsToRads(center.latitude()), .lng = degsToRads(center.longitude())};

            H3Index centerCell = H3_NULL;
            H3Error err = E_SUCCESS;
            err = latLngToCell(&ll, 2, &centerCell);
            if (err != E_SUCCESS) {
                spdlog::warn("{} {}", "Failed to convert center coordinates to H3", describeH3Error(err));
            }

            spdlog::info("Generating maze at center cell with radius {}", radius);

            // Генерируем клеточный лабиринт (возвращает клетки-стены)
            try {
                walls = mazeGenerator_.generateMaze(centerCell, radius);
            }catch (const std::exception &e) {
                spdlog::error("{}", e.what());
            }

            spdlog::info("Cell maze generated: {} wall cells", walls.size());
            isMazeComputed = true;
            spdlog::info("Maze generation complete");

            int64_t ringSize = 0;
            err = maxGridDiskSize(radius, &ringSize);
            if (err != E_SUCCESS) {
                spdlog::warn(describeH3Error(err));
            }
            std::vector<H3Index> distances(ringSize);
            err = gridRing(centerCell, radius, distances.data());
            if (err != E_SUCCESS) {
                spdlog::warn(describeH3Error(err));
            }
            distances.shrink_to_fit();

            const H3Index zeroCell = distances.front();
            const H3Index middleCell = getMiddleOfRing(distances, zeroCell);

            deleteStartEndEntities(zeroCell, middleCell);

            //The first cell is the entrance, skip it.
            for(auto const& cellId : distances | std::views::drop(1)) {
                if (cellId == middleCell) {
                    continue;
                }
                if (!isValidCell(cellId)) {
                    continue;
                }
                walls.insert(cellId);
            }
        }

        // Визуализация: объединяем все стены в полигоны и отправляем
        auto mergedPolygons = cellsToMergedPolygons(walls);
        if (!mergedPolygons.empty()) {
            emit mazePolygonsComputed(mergedPolygons);
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
                    std::this_thread::sleep_for(1ms);
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
void H3Worker::deleteStartEndEntities(H3Index start, H3Index end) {
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
    for(auto const& cellId : distances | std::views::drop(1)) {
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