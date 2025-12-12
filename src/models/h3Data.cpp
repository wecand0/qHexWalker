// Precompiled header must go first
#include "pch.h"

#include "h3Data.h"

H3Data::H3Data(QObject *parent) : QObject(parent) {}

H3Data::~H3Data() = default;

quint8 H3Data::res() const noexcept { return res_; }
quint64 H3Data::index() const noexcept { return index_; }
QColor H3Data::color() const noexcept { return color_; }
QGeoCoordinate H3Data::coordinate() const noexcept { return coordinate_; }
QVariantList H3Data::path() const { return path_; }

void H3Data::setRes(const quint8 res) {
    if (res != res_) {
        res_ = res;
        emit resChanged();
    }
}

void H3Data::setIndex(const quint64 index) {
    if (index != index_) {
        index_ = index;
        emit indexChanged();
    }
}
void H3Data::setColor(const QColor &color) {
    if (color != color_) {
        color_ = color;
        emit colorChanged();
    }
}
void H3Data::setCoordinate(const QGeoCoordinate &coordinate) {
    if (!coordinate.isValid()) {
        spdlog::warn("Coordinate is not valid! {}", coordinate.toString(QGeoCoordinate::Degrees).toStdString());

        return;
    }
    if (coordinate != coordinate_) {
        coordinate_ = coordinate;
        emit coordinateChanged();
    }
}

void H3Data::setPath(const QVariantList &path) {
    if (path_ != path) {
        path_ = path;
        emit pathChanged();
    }
}