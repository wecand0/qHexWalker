#ifndef QHEXWALKER_HELPER_H
#define QHEXWALKER_HELPER_H

namespace H3_VIEWER {
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
        for (auto i = 1; i < childBoundary.numVerts; ++i) {
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

    // Находит общее ребро (2 вершины) между двумя соседними ячейками H3
    static std::optional<QVariantList> getSharedEdge(const H3Index cell1, const H3Index cell2) {
        CellBoundary boundary1{}, boundary2{};

        if (cellToBoundary(cell1, &boundary1) != E_SUCCESS || cellToBoundary(cell2, &boundary2) != E_SUCCESS) {
            return std::nullopt;
        }

        if (boundary1.numVerts == 0 || boundary2.numVerts == 0) {
            return std::nullopt;
        }

        // Находим общие вершины (должно быть ровно 2 для соседних шестиугольников)
        std::vector<LatLng> sharedVertices;

        for (int i = 0; i < boundary1.numVerts; ++i) {
            for (int j = 0; j < boundary2.numVerts; ++j) {
                const double latDiff = std::abs(boundary1.verts[i].lat - boundary2.verts[j].lat);
                const double lngDiff = std::abs(boundary1.verts[i].lng - boundary2.verts[j].lng);

                if (constexpr double EPSILON = 1e-9; latDiff < EPSILON && lngDiff < EPSILON) {
                    sharedVertices.emplace_back(boundary1.verts[i]);
                    break;
                }
            }
        }

        if (sharedVertices.size() != 2) {
            // Не соседи или ошибка
            return std::nullopt;
        }

        // Создаем линию из двух точек
        QVariantList line;
        line.reserve(2);

        const double lat1 = radsToDegs(sharedVertices[0].lat);
        const double lng1 = radsToDegs(sharedVertices[0].lng);
        const double lat2 = radsToDegs(sharedVertices[1].lat);
        double lng2 = radsToDegs(sharedVertices[1].lng);

        // Обрабатываем антимеридиан
        if (const double delta = lng2 - lng1; delta > 180.0) {
            lng2 -= 360.0;
        } else if (delta < -180.0) {
            lng2 += 360.0;
        }

        line.emplace_back(QVariant::fromValue(QGeoCoordinate{lat1, lng1, 0}));
        line.emplace_back(QVariant::fromValue(QGeoCoordinate{lat2, lng2, 0}));

        return line;
    }
};
}  // namespace H3_VIEWER

#endif  // QHEXWALKER_HELPER_H
