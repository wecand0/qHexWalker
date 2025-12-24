#include "h3TargetsModel.h"
#include "h3Target.h"

#include <helper.h>

H3TargetsModel::H3TargetsModel(QObject *parent) : QAbstractListModel(parent) {
    auto zoomToResolution = [&](const double zoom) {
        return std::max(std::min(zoom / 1.5, static_cast<double>(maxZoom_c)), 0.0);
    };
    for (auto zoom = minZoom_c; zoom < maxZoom_c; zoom++) {
        zoomToRes_.emplace(zoom, std::floor(zoomToResolution(zoom)));
    }
}

H3TargetsModel::~H3TargetsModel() {
    qDeleteAll(cells_);
    cells_.clear();
}

int H3TargetsModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(cells_.size());
}

QVariant H3TargetsModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() >= cells_.size()) {
        return {};
    }

    const auto data = cells_.at(index.row());
    switch (role) {
    case ResRole:
        return QVariant::fromValue(data->res());
    case ZoomRole:
        return QVariant::fromValue(data->zoom());
    case OrderRole:
        return QVariant::fromValue(data->order());
    case IndexRole:
        return QVariant::fromValue(data->index());
    case CellColor:
        return QVariant::fromValue(data->color());
    case PathRole:
        return QVariant::fromValue(data->path());
    case CoordinatesRole:
        return QVariant::fromValue(data->coordinate());
    default:;
    }
    return {};
}

QHash<int, QByteArray> H3TargetsModel::roleNames() const {
    // clang-format off
    return { {ResRole, "res"},
                {ZoomRole, "zoom"},
                {OrderRole, "order"},
                {IndexRole, "h3Index"},
                {CellColor, "color"},
                {PathRole, "path"},
                {CoordinatesRole, "coordinate"}};
    // clang-format on
}

void H3TargetsModel::compute() {
    std::vector<H3Index> indexes;
    indexes.reserve(cells_.size());
    for (const auto &cell : cells_) {
        indexes.emplace_back(cell->index());
    }
    emit onCompute(indexes);
}
void H3TargetsModel::move(int from, int to) {
    if (from < 0 || from >= cells_.size() || to < 0 || to >= cells_.size() || from == to)
        return;

    // Правильно рассчитываем destinationRow для beginMoveRows
    const int destinationRow = (to > from) ? (to + 1) : to;

    isClearing_ = true;
    emit clearingStarted();

    beginMoveRows(QModelIndex(), from, from, QModelIndex(), destinationRow);

    // Ручное перемещение — 100% надёжно
    H3Target *item = cells_.takeAt(from);
    cells_.insert(to, item);

    endMoveRows();

    // Обновляем order
    const int start = std::min(from, to);
    const int end = std::max(from, to);
    for (int i = start; i <= end; ++i) {
        cells_[i]->setOrder(static_cast<quint16>(i + 1));
    }

    emit dataChanged(index(start), index(end), {OrderRole});

    isClearing_ = false;
    emit clearingFinished();
}

qsizetype H3TargetsModel::remove(const int row) {
    if (row < 0 || row >= cells_.size())
        return {};

    isClearing_ = true;
    emit clearingStarted();

    // Правильно: удаляем одну строку
    beginRemoveRows(QModelIndex(), row, row);
    const H3Target *cell = cells_.takeAt(row);
    delete cell;
    endRemoveRows();

    // Обновляем order у оставшихся элементов (начиная с row и до конца)
    for (int i = row; i < cells_.size(); ++i) {
        cells_[i]->setOrder(static_cast<quint16>(i + 1));
    }

    // Уведомляем QML, что order изменился у оставшихся элементов
    if (row < cells_.size()) {
        emit dataChanged(index(row), index(cells_.size() - 1), {OrderRole});
    } else if (cells_.isEmpty()) {
        // Если список стал пустым — уведомляем хотя бы одну (фиктивную) строку
        emit dataChanged(index(0), index(0), {OrderRole});
    }

    isClearing_ = false;
    emit clearingFinished();

    std::vector<H3Index> indexes;
    indexes.reserve(cells_.size());
    for (const auto &c : cells_) {
        indexes.emplace_back(c->index());
    }
    emit onRemoveCell(indexes);

    return cells_.size();
}

void H3TargetsModel::requestCell(const quint8 mapZoom, const QGeoCoordinate &coordinate) {
    if (!isCoordinateTargetValid(mapZoom, coordinate)) {
        return;
    }
    if (isClearing_) {
        return;
    }
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

    auto comp = [h3Index](const H3Target *cell) { return cell->index() == h3Index; };
    const auto isUnique = std::ranges::find_if(cells_, comp);
    if (isUnique != cells_.end()) {
        return;
    }

    const auto polygon = H3_VIEWER::Helper::indexToPolygon(h3Index);
    if (!polygon.has_value()) {
        return;
    }

    auto cell = new H3Target(this);
    cell->setRes(res);
    cell->setZoom(mapZoom);
    cell->setIndex(h3Index);
    cell->setCoordinate(coordinate);
    cell->setPath(polygon.value());
    cell->setColor("red");

    beginInsertRows(QModelIndex(), static_cast<int>(cells_.size()), static_cast<int>(cells_.size()));
    cells_.emplace_back(cell);
    endInsertRows();

    cell->setOrder(static_cast<quint16>(cells_.size()));  // после endInsertRows()
}

void H3TargetsModel::clearAllCells() {
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

bool H3TargetsModel::isCoordinateTargetValid(quint8 zoom, const QGeoCoordinate &coordinate) const {
    if (!coordinate.isValid()) {
        return false;
    }
    if (zoom >= maxZoom_c) {
        return false;
    }

    return true;
}
