//
// Created by Vadim on 15.12.2025.
//

#include "h3TargetsModel.h"
#include "h3Data.h"
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

QVariant H3TargetsModel::data(const QModelIndex &index, int role) const {
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
        case CoordinatesRole:
            return QVariant::fromValue(data->coordinate());
        default:;
    }
    return {};
}

QHash<int, QByteArray> H3TargetsModel::roleNames() const {
    return {{ResRole, "res"}, {IndexRole, "index"}, {CellColor, "color"}, {PathRole, "path"}, {CoordinatesRole, "coordinate"}};
}

void H3TargetsModel::compute() {
    emit onCompute();
}

void H3TargetsModel::remove(int index) {
    beginResetModel();
    cells_.takeAt(index);
    endResetModel();
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

    auto polygon = Helper::indexToPolygon(h3Index);
    if (!polygon.has_value()) {
        return;
    }

    auto cell = new H3Data(this);
    cell->setRes(res);
    cell->setIndex(h3Index);
    cell->setCoordinate(coordinate);
    cell->setPath(polygon.value());
    cell->setColor("red");

    beginInsertRows(QModelIndex(), static_cast<int>(cells_.size()), static_cast<int>(cells_.size()));
    cells_.emplace_back(cell);
    endInsertRows();

    SPDLOG_INFO("ADDED {} {} {}", res, coordinate.latitude(), coordinate.longitude());
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
