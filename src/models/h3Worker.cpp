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
            const QGeoCoordinate center{55, 55, 0};
            int radius = 50;

            // Конвертируем координату в H3
            LatLng ll{.lat = degsToRads(center.latitude()), .lng = degsToRads(center.longitude())};

            H3Index centerCell = H3_NULL;
            if (latLngToCell(&ll, 2, &centerCell) != E_SUCCESS) {
                return;
            }

            SPDLOG_CRITICAL("walls");

            // Генерируем лабиринт
            H3Index start = 0x82185ffffffffff, end = 0x821557fffffffff;
            walls = mazeGenerator_.generateMaze(centerCell, 15, start, end);
            mazeGenerator_.mazeGenerated(walls);
            SPDLOG_CRITICAL("walls: {}", walls.size());
            for (const auto &wall : walls) {
                auto childPolygon = Helper::indexToPolygon(wall);
                if (!childPolygon.has_value()) {
                    break;
                }
                // std::this_thread::sleep_for(17ms);
                emit cellComputed(getResolution(wall), wall, childPolygon.value(), true);
            }
            isMazeComputed = true;
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
                    std::this_thread::sleep_for(17ms);
                    emit cellComputed(getResolution(index), index, childPolygon.value(), false);
                }
            } catch (const std::exception &e) {
                spdlog::warn("{}", e.what());
            }
        }
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