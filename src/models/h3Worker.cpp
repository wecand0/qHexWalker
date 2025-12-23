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
            H3Index start = 0x8235affffffffff, end = 0x827c6ffffffffff;
            walls = mazeGenerator_.generateMaze(centerCell, radius, start, end);

            spdlog::info("Cell maze generated: {} wall cells, start={}, end={}", walls.size(), start, end);
            isMazeComputed = true;
            spdlog::info("Maze generation complete");
        }
            // Получаем все клетки в области лабиринта
            // auto allCells = mazeGenerator_.getCellsInRadius(centerCell, radius);
            // spdlog::info("Total maze cells: {}", allCells.size());
            //
            // // Клетки-стены используются для блокировки в A*
            // walls.clear();
            // walls.insert(wallCells.begin(), wallCells.end());

            // Визуализация: отрисовываем ВСЕ клетки лабиринта
            // Стены - темный цвет, проходы - светлый цвет
            for (const auto &cell : walls) {
                auto cellPolygon = Helper::indexToPolygon(cell);
                if (!cellPolygon.has_value()) {
                    continue;
                }

                //std::this_thread::sleep_for(1ms);

                emit cellComputed(getResolution(cell), cell, cellPolygon.value(), true);
            }


        // std::vector<H3Index> pentagons;
        // auto pSize = pentagonCount();
        // pentagons.resize(pSize);
        // getPentagons(2, pentagons.data());
        // for (const auto &pentagon : pentagons) {
        //     auto pentagonPolygon = Helper::indexToPolygon(pentagon);
        //     if (!pentagonPolygon.has_value()) {
        //         continue;
        //     }
        //     emit cellComputed(getResolution(pentagon), pentagon, pentagonPolygon.value(), false);
        // }

        // для построение лабиринта в entry point и далее создавать единый полигон LinkedGeoPolygon
        //         auto _ = QtConcurrent::run([this, coordinate] {
        //     try {
        //         // Marshal all QObject interactions back to the GUI thread
        //         QMetaObject::invokeMethod(
        //             this,
        //             [this, coordinate] {
        //
        //             },
        //             Qt::BlockingQueuedConnection);
        //     } catch (std::exception &e) {
        //         spdlog::critical(e.what());
        //     }
        // });

        // Устанавливаем стены в A*
       // astar_->setBlockedCells(walls);

        H3Index prevIndex = req.indexes.front();
        std::vector<H3Index> path;
        //  std::vector<H3Index> tempV;
        for (size_t indexId = 1; indexId < req.indexes.size(); indexId++) {
            try {
                path = astar_->findShortestPath(prevIndex, req.indexes.at(indexId));
                //                std::ranges::copy(tempV, std::back_inserter(path));
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