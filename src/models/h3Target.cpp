#include "h3Target.h"

H3Target::H3Target(QObject *parent) : IH3Data(parent) {}

H3Target::~H3Target() = default;

quint8 H3Target::zoom() const noexcept { return zoom_; }
quint16 H3Target::order() const noexcept { return order_; }
QGeoCoordinate H3Target::coordinate() const noexcept { return coordinate_; }

void H3Target::setZoom(const quint8 zoom) {
    if (zoom_ != zoom) {
        zoom_ = zoom;
        emit zoomChanged();
    }
}
void H3Target::setOrder(const quint16 order) {
    if (order != order_) {
        order_ = order;
        emit orderChanged();
    }
}

void H3Target::setCoordinate(const QGeoCoordinate &coordinate) {
    if (!coordinate.isValid()) {
        spdlog::warn("Coordinate is not valid! {}", coordinate.toString(QGeoCoordinate::Degrees).toStdString());

        return;
    }
    if (coordinate != coordinate_) {
        coordinate_ = coordinate;
        emit coordinateChanged();
    }
}