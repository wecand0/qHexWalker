#include "h3Worker.h"

using namespace H3_VIEWER;
using namespace std::chrono_literals;

H3Worker::H3Worker(QObject *parent) : QObject(parent) {
    astar_ = new H3AStar();
    // connect(astar_, &H3AStar::newCell, this, [this](H3Index index) {
    //     const auto childPolygon = Helper::indexToPolygon(index);
    //     std::this_thread::sleep_for(30ms);
    //     emit cellComputed(getResolution(index), index, childPolygon.value(), true);
    // });
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
            if (latLngToCell(&ll, 2, &centerCell) != E_SUCCESS) {
                spdlog::error("Failed to convert center coordinates to H3");
                return;
            }

            spdlog::info("Generating maze at center cell with radius {}", radius);

            // Генерируем клеточный лабиринт (возвращает клетки-стены)
            walls = mazeGenerator_.generateMaze(centerCell, radius);

            spdlog::info("Cell maze generated: {} wall cells", walls.size());
            isMazeComputed = true;
            spdlog::info("Maze generation complete");

            int64_t ringSize = 0;
            maxGridDiskSize(radius, &ringSize);
            std::vector<H3Index> distances(ringSize);

            gridRing(centerCell, radius, distances.data());
            distances.shrink_to_fit();

            const H3Index zeroCell = distances.front();
            const H3Index middleCell = getMiddleOfRing(distances, zeroCell);

            deleteStartEndEntities(zeroCell, middleCell);

            for (size_t cellId = 0; cellId < distances.size(); cellId++) {
                if (cellId == 0) {
                    continue;
                }
                if (distances.at(cellId) == middleCell) {
                    continue;
                }
                walls.insert(distances.at(cellId));
            }
        }

        // Визуализация: обрисовываем ВСЕ клетки лабиринта
        // Стены - темный цвет, проходы - светлый цвет
        for (const auto cell : walls) {
            auto cellPolygon = Helper::indexToPolygon(cell);
            if (!cellPolygon.has_value()) {
                continue;
            }
            emit cellComputed(getResolution(cell), cell, cellPolygon.value(), true);
        }

        // Устанавливаем стены в A*
        astar_->setBlockedCells(walls);

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
                    emit cellComputed(getResolution(index), index, childPolygon.value(), false);
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
    H3Error err = maxGridDiskSize(3, &maxSize);
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }

    std::vector<H3Index> disk(maxSize);
    err = gridDisk(start, 3, disk.data());
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
    err = gridDisk(end, 3, disk2.data());
    if (err != E_SUCCESS) {
        spdlog::warn(describeH3Error(err));
    }
    for (auto d : disk2) {
        if (walls.contains(d)) {
            walls.erase(d);
        }
    }
}
H3Index H3Worker::getMiddleOfRing(const std::vector<H3Index> &distances, H3Index zeroCell) {
    LatLng zeroLatLng;
    cellToLatLng(zeroCell, &zeroLatLng);
    double dist = 0;
    LatLng ll;
    H3Index middleCell = H3_NULL;
    for (size_t cellId = 0; cellId < distances.size(); cellId++) {
        if (cellId == 0) {
            continue;
        }
        cellToLatLng(distances.at(cellId), &ll);

        auto distTemp = greatCircleDistanceM(&zeroLatLng, &ll);
        if (distTemp > dist) {
            dist = distTemp;
            middleCell = distances.at(cellId);
        }
    }
    return middleCell;
}