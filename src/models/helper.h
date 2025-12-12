#ifndef QHEXWALKER_HELPER_H
#define QHEXWALKER_HELPER_H

#include <QGeoCoordinate>
#include <QVariantList>
#include <h3/h3api.h>

struct Helper {
    static std::optional<QVariantList> indexToPolygon(const H3Index index) {
        CellBoundary childBoundary{};
        if (const auto errB = cellToBoundary(index, &childBoundary); errB != E_SUCCESS) {
            return std::nullopt;
        }

        if (childBoundary.numVerts == 0) {
            return std::nullopt;
        }

        QVariantList polygon;
        //+1 для завершения полигона
        polygon.reserve(childBoundary.numVerts + 1);

        // Конвертируем первую точку
        const double firstLat = radsToDegs(childBoundary.verts[0].lat);
        const double firstLng = radsToDegs(childBoundary.verts[0].lng);

        polygon.emplace_back(QVariant::fromValue(QGeoCoordinate{firstLat, firstLng, 0}));

        double prevLng = firstLng;

        // Обрабатываем остальные точки
        for (int i = 1; i < childBoundary.numVerts; ++i) {
            const double lat = radsToDegs(childBoundary.verts[i].lat);
            double lng = radsToDegs(childBoundary.verts[i].lng);

            // Проверяем скачок через антимеридиан
            // Если разница больше 180°, значит пересекли антимеридиан
            if (const double delta = lng - prevLng; delta > 180.0) {
                // Скачок с востока на запад: уменьшаем текущую долготу
                lng -= 360.0;
            } else if (delta < -180.0) {
                // Скачок с запада на восток: увеличиваем текущую долготу
                lng += 360.0;
            }

            polygon.emplace_back(QVariant::fromValue(QGeoCoordinate{lat, lng, 0}));
            prevLng = lng;
        }

        // Замыкаем полигон - используем первую точку
        polygon.emplace_back(QVariant::fromValue(QGeoCoordinate{firstLat, firstLng, 0}));

        return polygon;
    }
};

#endif  // QHEXWALKER_HELPER_H
