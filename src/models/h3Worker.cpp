#include "h3Worker.h"

#include <ranges>

using namespace H3_VIEWER;
using namespace std::chrono_literals;

H3Worker::H3Worker(QObject *parent) : QObject(parent) {
    astar_ = new H3AStar(this);

    // Подключаем сигнал A* для визуализации процесса поиска
    connect(astar_, &H3AStar::newCell, this, &H3Worker::onAStarNewCell, Qt::DirectConnection);
}

H3Worker::~H3Worker() = default;

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

        // Сброс счётчика и засекаем время
        exploredCellsCount_.store(0);
        auto startTime = std::chrono::high_resolution_clock::now();

        H3Index prevIndex = req.indexes.front();
        std::vector<H3Index> path;
        int totalPathLength = 0;

        for (size_t indexId = 1; indexId < req.indexes.size(); indexId++) {
            try {
                path = astar_->findShortestPath(prevIndex, req.indexes.at(indexId));
                prevIndex = req.indexes.at(indexId);
                totalPathLength += static_cast<int>(path.size());

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

        // Вычисляем время поиска
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        const double timeMs = duration.count() / 1000.0;

        // Эмитим статистику
        emit searchStats(exploredCellsCount_.load(), timeMs, totalPathLength);

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

void H3Worker::setWalls(const std::unordered_set<H3Index> &mazeWalls) {
    std::lock_guard lk(mutex_);
    walls = mazeWalls;

    if (astar_) {
        astar_->setBlockedCells(walls);
    }

    spdlog::info("H3Worker: Walls updated for A* algorithm, {} wall cells", walls.size());
}

void H3Worker::onAStarNewCell(const H3Index cell) {
    // Увеличиваем счётчик исследованных ячеек для статистики
    exploredCellsCount_.fetch_add(1, std::memory_order_relaxed);

    // Throttling: показываем каждую N-ую ячейку для плавной анимации
    static int counter = 0;
    static constexpr int THROTTLE_FACTOR = 25;  // Показываем каждую 25-ю ячейку

    if (++counter % THROTTLE_FACTOR != 0) {
        return;
    }

    // Конвертируем H3Index в полигон
    const auto polygon = Helper::indexToPolygon(cell);
    if (!polygon.has_value()) {
        return;
    }

    // Эмитим сигнал с флагом isSearching=false (исследуемая ячейка, не финальный путь)
    emit cellComputed(getResolution(cell), cell, polygon.value(), false);
}