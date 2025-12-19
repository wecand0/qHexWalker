//
// Created by user on 02/12/2025.
//

#ifndef Q_HEX_WALKER_H3WORKER_H
#define Q_HEX_WALKER_H3WORKER_H

#include "astar.h"
#include "helper.h"

#include "h3MazeGenerator.h"

struct h3_deleter {
    void operator()(LinkedGeoPolygon *poly) const {
        if (poly == nullptr) {
            return;
        }
        destroyLinkedMultiPolygon(poly);
    }
};
using LinkedGeoPolygonPtr = std::unique_ptr<LinkedGeoPolygon, h3_deleter>;

namespace H3_VIEWER {
class H3Worker final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Worker)

public:
    explicit H3Worker(QObject *parent = nullptr);
    ~H3Worker() override;
    struct PendingRequest {
        std::vector<H3Index> indexes;
        bool has{};
    };

public slots:
    void doWork();
    // Запросить пересчет ячейки по координатам (в градусах) и разрешению
    void requestCell(const std::vector<H3Index> &index);

signals:
    void finished();
    // Результат пересчета: разрешение, идентификатор H3 и полигон ячейки
    void cellComputed(quint8 res, H3Index id, const QVariantList &polygon, bool isSearching);

    void cellsComputed(const QVariantList &polygon);

private:
    std::unordered_set<H3Index> walls;
    bool isMazeComputed = false;
    H3AStar *astar_{};
    H3MazeGenerator mazeGenerator_{};
    // static QVariantList indexToPolygon(H3Index index);
    std::atomic_bool isRequested{false};
    std::mutex mutex_;
    std::condition_variable cv_;
    PendingRequest pending_;
};
}  // namespace H3_VIEWER

#endif  // Q_HEX_WALKER_H3WORKER_H