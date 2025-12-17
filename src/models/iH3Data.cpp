#include "iH3Data.h"

IH3Data::IH3Data(QObject *parent) : QObject(parent) {}

IH3Data::~IH3Data() = default;

quint8 IH3Data::res() const noexcept { return res_; }
quint64 IH3Data::index() const noexcept { return index_; }
QColor IH3Data::color() const noexcept { return color_; }
QVariantList IH3Data::path() const { return path_; }

void IH3Data::setRes(const quint8 res) {
    if (res != res_) {
        res_ = res;
        emit resChanged();
    }
}

void IH3Data::setIndex(const quint64 index) {
    if (index != index_) {
        index_ = index;
        emit indexChanged();
    }
}
void IH3Data::setColor(const QColor &color) {
    if (color != color_) {
        color_ = color;
        emit colorChanged();
    }
}

void IH3Data::setPath(const QVariantList &path) {
    if (path_ != path) {
        path_ = path;
        emit pathChanged();
    }
}