#include "h3Model.h"
#include "h3Data.h"
#include "h3Worker.h"

H3Model::H3Model(QObject *parent) : QAbstractListModel(parent) {
    auto zoomToResolution = [&](const double zoom) { return std::max(std::min(zoom / 1.5, maxZoom_c), 0.0); };
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

    qDeleteAll(cells_);
    cells_.clear();
}

int H3Model::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(cells_.size());
}

QVariant H3Model::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() >= cells_.size()) {
        return {};
    }

    const auto data = cells_.at(index.row());
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
    return {{ResRole, "res"}, {IndexRole, "index"}, {CellColor, "color"}, {PathRole, "path"}};
}

//////////////

void H3Model::Init() {
    worker_ = new H3_VIEWER::H3Worker();
    thread_ = new QThread();
    worker_->moveToThread(thread_);

    connect(worker_, &H3_VIEWER::H3Worker::finished, worker_, &H3_VIEWER::H3Worker::deleteLater, Qt::QueuedConnection);
    connect(thread_, &QThread::finished, thread_, &QThread::deleteLater, Qt::QueuedConnection);

    // Запуск рабочего цикла в потоке
    connect(thread_, &QThread::started, worker_, &H3_VIEWER::H3Worker::doWork, Qt::QueuedConnection);
    // Получение результатов пересчета
    connect(worker_, &H3_VIEWER::H3Worker::cellComputed, this, &H3Model::onCellComputed, Qt::QueuedConnection);
    thread_->start();
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

std::optional<H3Data *> H3Model::findCellByRes(const quint8 res) const {
    const auto it = std::ranges::find_if(cells_, [res](const auto &cell) { return cell->res() == res; });
    if (it == cells_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::optional<H3Data *> H3Model::findCellByID(const quint64 id) const {
    const auto it = std::ranges::find_if(cells_, [id](const auto &cell) { return cell->index() == id; });
    if (it == cells_.end()) {
        return std::nullopt;
    }
    return *it;
}

QString H3Model::getColorForResolution(const quint8 resolution) const {
    // Цветовая схема: от крупных ячеек (теплые цвета) к мелким (холодные цвета)
    return resolutionColors_c.value(resolution, "gray");
}

void H3Model::onCellComputed(const quint8 res, const H3Index index, const QVariantList &polygon,
                             const bool isSearching) {
    // Не добавляем новые ячейки во время очистки
    if (isClearing_) {
        return;
    }

    auto cell = new H3Data(this);
    cell->setRes(res);
    cell->setIndex(index);
    cell->setPath(polygon);

    if (isSearching) {
        cell->setColor("lavender");
    } else {
        cell->setColor(getColorForResolution(res));
    }

    beginInsertRows(QModelIndex(), static_cast<int>(cells_.size()), static_cast<int>(cells_.size()));
    cells_.emplace_back(cell);
    endInsertRows();
}

void H3Model::requestCell(const quint8 mapZoom, const QGeoCoordinate &coordinate) {
    if (!worker_) {
        return;
    }
    if (!isCoordinateTargetValid(mapZoom, coordinate)) {
        return;
    }
    if (isClearing_) {
        return;
    }

    SPDLOG_INFO("requestCell map zoom {}", mapZoom);

    uint8_t res = 0;
    try {
        res = zoomToRes_.at(mapZoom);
    } catch (const std::out_of_range &err) {
        spdlog::error("Выбран недопустимый зум под разрешение {}", err.what());
        return;
    }

    H3Index h3Index = H3_NULL;
    const LatLng ll{.lat = degsToRads(coordinate.latitude()), .lng = degsToRads(coordinate.longitude())};
    if (const auto errIdx = latLngToCell(&ll, res, &h3Index); errIdx != E_SUCCESS || h3Index == H3_NULL) {
        spdlog::warn("Impossible to convert this lat:{} lng:{} coordinate to H3Index {}", coordinate.latitude(),
                     coordinate.longitude(), errIdx);
        return;
    }
    if (findCellByID(h3Index).has_value()) {
        return;
    }

    // Если есть старые ячейки, очищаем их перед добавлением новой
    if (!cells_.empty()) {
        clearAllCells();

        if (!isClearing_) {
            worker_->requestCell(h3Index);
        }
    } else {
        // Если модель пустая, запрашиваем сразу
        worker_->requestCell(h3Index);
    }
}

void H3Model::clearAllCells() {
    // Проверяем, есть ли что очищать
    if (cells_.isEmpty()) {
        return;
    }

    // Предотвращаем повторный вызов во время очистки
    if (isClearing_) {
        spdlog::info("Already clearing, skipping...");
        return;
    }

    spdlog::info("Starting clearAllCells, count: {}", cells_.size());

    isClearing_ = true;
    emit clearingStarted();

    beginResetModel();
    qDeleteAll(cells_);
    cells_.clear();
    endResetModel();

    isClearing_ = false;
    emit clearingFinished();
}