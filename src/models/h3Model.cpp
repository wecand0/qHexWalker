#include "h3Model.h"
#include "h3Cell.h"
#include "h3MazeAdapter.h"
#include "h3Worker.h"

#include <QtConcurrent/qtconcurrentrun.h>
#include <algorithm>

H3Model::H3Model(QObject *parent) : QAbstractListModel(parent) {
    auto zoomToResolution = [&](const double zoom) {
        return std::max(std::min(zoom / 1.5, static_cast<double>(maxZoom_c)), 0.0);
    };
    for (auto zoom = minZoom_c; zoom < maxZoom_c; zoom++) {
        zoomToRes_.emplace(zoom, std::floor(zoomToResolution(zoom)));
    }
}

H3Model::~H3Model() {
    // Останавливаем worker перед очисткой
    if (worker_ && thread_) {
        thread_->quit();
        thread_->wait(250);
    }

    qDeleteAll(pathCells_);
    pathCells_.clear();
}

int H3Model::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(pathCells_.size());
}

QVariant H3Model::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() >= pathCells_.size()) {
        return {};
    }

    const auto data = pathCells_.at(index.row());
    switch (role) {
    case ResRole:
        return QVariant::fromValue(data->res());
    case IndexRole:
        return QVariant::fromValue(data->index());
    case CellColor:
        return QVariant::fromValue(data->color());
    case PathRole:
        return QVariant::fromValue(data->path());
    default:;
    }
    return {};
}

QHash<int, QByteArray> H3Model::roleNames() const {
    // clang-format off
    return {{ResRole, "res"}, {IndexRole, "h3Index"}, {CellColor, "color"}, {PathRole, "path"}};
    // clang-format on
}

//////////////

void H3Model::Init() {
    // Создаем и настраиваем worker
    worker_ = new H3_VIEWER::H3Worker();
    thread_ = new QThread();
    worker_->moveToThread(thread_);

    connect(worker_, &H3_VIEWER::H3Worker::finished, worker_, &H3_VIEWER::H3Worker::deleteLater, Qt::QueuedConnection);
    connect(thread_, &QThread::finished, thread_, &QThread::deleteLater, Qt::QueuedConnection);

    // Запуск рабочего цикла в потоке
    connect(thread_, &QThread::started, worker_, &H3_VIEWER::H3Worker::doWork, Qt::QueuedConnection);
    // Получение результатов пересчета
    connect(worker_, &H3_VIEWER::H3Worker::cellComputed, this, &H3Model::onCellComputed, Qt::QueuedConnection);
    connect(worker_, &H3_VIEWER::H3Worker::cellsComputed, this, &H3Model::onCellsComputed, Qt::QueuedConnection);
    connect(worker_, &H3_VIEWER::H3Worker::mazePolygonsComputed, this, &H3Model::onMazePolygonsComputed,
            Qt::QueuedConnection);
    connect(worker_, &H3_VIEWER::H3Worker::searchStats, this, &H3Model::onSearchStats, Qt::QueuedConnection);
    thread_->start();

    // Создаем и настраиваем maze adapter
    mazeAdapter_ = new H3MazeAdapter();

    // Подключаем сигналы maze adapter
    // Сигнал для визуализации полигонов стен
    connect(mazeAdapter_, &H3MazeAdapter::mazePolygonsComputed, this, &H3Model::onMazePolygonsComputed,
            Qt::QueuedConnection);

    // Сигнал для передачи стен в worker для A* алгоритма
    connect(
        mazeAdapter_, &H3MazeAdapter::mazeWallsGenerated, this,
        [this](const std::unordered_set<H3Index> &walls) { worker_->setWalls(walls); }, Qt::QueuedConnection);

    // Пробрасываем сигнал mazeWallsGenerated наружу для targetsModel
    connect(mazeAdapter_, &H3MazeAdapter::mazeWallsGenerated, this, &H3Model::mazeWallsGenerated, Qt::QueuedConnection);

    // Получаем вычисленный радиус лабиринта и передаем дальше
    connect(
        mazeAdapter_, &H3MazeAdapter::mazeRadiusComputed, this,
        [this](const QGeoCoordinate &center, double radiusMeters) {
            mazeCenter_ = center;
            mazeRadius_ = radiusMeters;

            emit mazeCenterChanged();
            emit mazeRadiusChanged();
            emit mazeBoundsGenerated(mazeCenter_, mazeRadius_);

            spdlog::info("H3Model: Maze bounds received - center ({}, {}), radius {} meters", center.latitude(),
                         center.longitude(), radiusMeters);
        },
        Qt::QueuedConnection);

    // Запускаем генерацию лабиринта асинхронно
    try {
        constexpr double mazeLat = 0.0;
        constexpr double mazeLon = 0.0;
        constexpr int kRingRadius = 50;

        mazeAdapter_->generateMazeAsync(mazeLat, mazeLon, kRingRadius);
    } catch (const std::exception &e) {
        spdlog::critical("{}", e.what());
    }
}

bool H3Model::isCoordinateTargetValid(const quint8 zoom, const QGeoCoordinate &coordinate) const {
    if (!coordinate.isValid()) {
        return false;
    }
    if (zoom >= maxZoom_c) {
        return false;
    }

    return true;
}

std::optional<H3Cell *> H3Model::findCellByRes(const quint8 res) const {
    const auto it = std::ranges::find_if(pathCells_, [res](const auto &cell) { return cell->res() == res; });
    if (it == pathCells_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<H3Cell *> H3Model::findCellByID(const quint64 id) const {
    const auto it = std::ranges::find_if(pathCells_, [id](const auto &cell) { return cell->index() == id; });
    if (it == pathCells_.end()) {
        return std::nullopt;
    }
    return *it;
}

QString H3Model::getColorForResolution(const quint8 resolution) const {
    // Цветовая схема: от крупных ячеек (теплые цвета) к мелким (холодные цвета)
    return resolutionColors_c.value(resolution, "gray");
}
void H3Model::addCell(const quint8 res, const H3Index index, const QVariantList &polygon, const QColor &color) {
    // Не добавляем новые ячейки во время очистки
    if (isClearing_) {
        return;
    }

    if (findCellByID(index).has_value()) {
        return;
    }

    auto cell = new H3Cell(this);
    cell->setRes(res);
    cell->setIndex(index);
    cell->setPath(polygon);
    cell->setColor(color);

    beginInsertRows(QModelIndex(), static_cast<int>(pathCells_.size()), static_cast<int>(pathCells_.size()));
    pathCells_.emplace_back(cell);
    endInsertRows();
}
void H3Model::addPentagons() {
    std::vector<H3Index> pentagons;
    pentagons.resize(pentagonCount());
    for (auto res = 2; res < 15; res++) {
        if (const auto err = getPentagons(res, pentagons.data()); err != E_SUCCESS) {
            spdlog::warn(describeH3Error(err));
        }
        for (const auto &pentagon : pentagons) {
            auto pentagonPolygon = H3_VIEWER::Helper::indexToPolygon(pentagon);
            if (!pentagonPolygon.has_value()) {
                continue;
            }
            onCellComputed(getResolution(pentagon), pentagon, pentagonPolygon.value(), false);
        }
    }
}

void H3Model::onCellsComputed(const QVariantList &list) {
    coordinates_ = list;
    emit coordinatesChanged();
}

void H3Model::onCellComputed(const quint8 res, const H3Index index, const QVariantList &polygon,
                             const bool isSearching) {
    // Не добавляем новые ячейки во время очистки
    if (isClearing_) {
        return;
    }

    // isSearching=false: A* исследует ячейку (светло-голубой)
    // isSearching=true: финальный путь (цвет по разрешению)
    const QColor cellColor =
        isSearching ? QColor(getColorForResolution(res)) : QColor(100, 200, 255, 120);  // Cyan для поиска

    addCell(res, index, polygon, cellColor);
}

void H3Model::requestCells(const std::vector<H3Index> &indexes) {
    if (indexes.empty()) {
        return;
    }
    // Если есть старые ячейки, очищаем их перед добавлением новой
    if (!pathCells_.empty()) {
        clearAllCells();

        if (!isClearing_) {
            worker_->requestCell(indexes);
        }
    } else {
        // Если модель пустая, запрашиваем сразу
        worker_->requestCell(indexes);
    }
}
void H3Model::clearAllCells() {
    // Проверяем, есть ли что очищать
    if (pathCells_.isEmpty()) {
        return;
    }

    // Предотвращаем повторный вызов во время очистки
    if (isClearing_) {
        spdlog::info("Already clearing, skipping...");
        return;
    }

    spdlog::info("Starting clearAllCells, count: {}", pathCells_.size());

    isClearing_ = true;
    emit clearingStarted();

    beginResetModel();
    qDeleteAll(pathCells_);
    pathCells_.clear();
    endResetModel();

    // Очищаем полигоны лабиринта
    // if (!mazePolygons_.isEmpty()) {
    //     mazePolygons_.clear();
    //     emit mazePolygonsChanged();
    // }

    isClearing_ = false;
    emit clearingFinished();
}

void H3Model::onMazePolygonsComputed(const std::vector<QVariantList> &polygons) {
    if (isClearing_) {
        return;
    }

    mazePolygons_.clear();
    mazePolygons_.reserve(static_cast<qsizetype>(polygons.size()));

    for (const auto &polygon : polygons) {
        mazePolygons_.append(polygon);
    }

    spdlog::info("Maze polygons updated: {} polygons", mazePolygons_.size());
    emit mazePolygonsChanged();
}

void H3Model::onSearchStats(int exploredCells, double timeMs, int pathLength) {
    searchStatsText_ = QString("Explored: %1 cells | Time: %2 ms | Path: %3 cells")
                           .arg(exploredCells)
                           .arg(timeMs, 0, 'f', 2)
                           .arg(pathLength);

    spdlog::info("Search stats: {} explored, {:.2f} ms, {} path length", exploredCells, timeMs, pathLength);
    emit searchStatsChanged();
}