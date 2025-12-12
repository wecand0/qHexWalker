#include "h3Worker.h"

using namespace H3_VIEWER;
using namespace std::chrono_literals;

H3Worker::H3Worker(QObject *parent) : QObject(parent) {
    astar_ = new H3AStar();
    // connect(astar_, &H3AStar::newCell, this, [this](H3Index index) {
    //     // searchingCells_.emplace_back(index);
    //     // const auto childPolygon = indexToPolygon(index);
    //     // std::this_thread::sleep_for(5ms);
    //     // emit cellComputed(getResolution(index), index, childPolygon, true);
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
        searchingCells_.clear();

        const H3Index start = req.index;  // 0x8b194ad14da3fffL;
        constexpr H3Index end = 0x8eb8a6b13046757L;

        for (const auto path = astar_->findShortestPath(start, end); const auto index : path) {
            auto childPolygon = Helper::indexToPolygon(index);
            if (!childPolygon.has_value()) {
                break;
            }
            std::this_thread::sleep_for(7ms);
            emit cellComputed(getResolution(index), index, childPolygon.value(), false);
        }

        {
            std::lock_guard lk(mutex_);
            isRequested.store(false);
        }
        SPDLOG_INFO("End wait");
    }
    emit finished();
}

void H3Worker::requestCell(const H3Index index) {
    {
        std::lock_guard lk(mutex_);
        if (isRequested.load()) {
            SPDLOG_WARN("cancel <requestCell>");
            return;
        }
        pending_.index = index;
        pending_.has = true;
        isRequested.store(true);
    }
    cv_.notify_one();
}