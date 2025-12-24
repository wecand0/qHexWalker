//
// Created by user on 02/12/2025.
//

#ifndef Q_HEX_WALKER_H3MODEL_H
#define Q_HEX_WALKER_H3MODEL_H

#include <QAbstractListModel>

class H3Cell;
namespace H3_VIEWER {
class H3Worker;
}
class H3Model final : public QAbstractListModel {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Model)
    Q_PROPERTY(QVariantList coordinates READ coordinates NOTIFY coordinatesChanged)
    Q_PROPERTY(QList<QVariantList> mazePolygons READ mazePolygons NOTIFY mazePolygonsChanged)
public:
    enum Roles { ResRole = Qt::UserRole + 1, IndexRole, CellColor, PathRole };

    explicit H3Model(QObject *parent = nullptr);
    ~H3Model() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]]
    QVariantList coordinates() const noexcept {
        return coordinates_;
    }

    [[nodiscard]] QList<QVariantList> mazePolygons() const noexcept { return mazePolygons_; }

private slots:
    void onCellsComputed(const QVariantList &list);
    void onCellComputed(quint8 res, H3Index index, const QVariantList &polygon, bool isSearching);
    void onMazePolygonsComputed(const std::vector<QVariantList> &polygons);

public:
    void Init();

public slots:
    Q_INVOKABLE void requestCells(const std::vector<H3Index> &indexes);
    Q_INVOKABLE void requestCell(quint8 mapZoom, const QGeoCoordinate &coordinate);
    Q_INVOKABLE void clearAllCells();

signals:
    void clearingStarted();
    void clearingFinished();
    void coordinatesChanged();
    void mazePolygonsChanged();

private:
    [[nodiscard]] std::optional<H3Cell *> findCellByID(quint64 id) const;
    [[nodiscard]] std::optional<H3Cell *> findCellByRes(quint8 res) const;
    [[nodiscard]] bool isCoordinateTargetValid(quint8 zoom, const QGeoCoordinate &coordinate) const;
    [[nodiscard]] QString getColorForResolution(quint8 resolution) const;

    void addCell(quint8 res, H3Index index, const QVariantList &polygon, const QColor &color);
    void addPentagons();

    H3_VIEWER::H3Worker *worker_{};
    QThread *thread_{};

    QList<H3Cell *> pathCells_;
    QVariantList coordinates_;
    QList<QVariantList> mazePolygons_;  // Список объединённых полигонов стен

    const uint8_t minZoom_c{3};
    const uint8_t maxZoom_c{15};
    std::unordered_map<uint8_t, uint8_t> zoomToRes_;
    const QHash<int, QString> resolutionColors_c = {
        {2, "crimson"},      {3, "orangered"},   {4, "darkorange"},  {5, "orange"},          {6, "gold"},
        {7, "yellow"},       {8, "greenyellow"}, {9, "limegreen"},   {10, "mediumseagreen"}, {11, "turquoise"},
        {12, "deepskyblue"}, {13, "dodgerblue"}, {14, "mediumblue"}, {15, "darkviolet"}};

    std::atomic_bool isClearing_{false};
};

#endif  // Q_HEX_WALKER_H3MODEL_H