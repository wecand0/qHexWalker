#ifndef QHEXWALKER_H3TARGETSMODEL_H
#define QHEXWALKER_H3TARGETSMODEL_H

#include <QAbstractListModel>

class H3Target;
namespace H3_VIEWER {
}
class H3TargetsModel final : public QAbstractListModel {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3TargetsModel)
public:
    enum Roles { ResRole = Qt::UserRole + 1, ZoomRole, OrderRole, IndexRole, CellColor, PathRole, CoordinatesRole };

    explicit H3TargetsModel(QObject *parent = nullptr);
    ~H3TargetsModel() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private slots:
    // void onCellAdded(quint8 res, H3Index index, const QVariantList &polygon, bool isSearching);

public slots:
    Q_INVOKABLE void compute();
    Q_INVOKABLE void move(int from, int to);  // <-- обязательно!
    Q_INVOKABLE qsizetype remove(int row);
    Q_INVOKABLE void requestCell(quint8 mapZoom, const QGeoCoordinate &coordinate);
    Q_INVOKABLE void clearAllCells();

signals:
    void onCompute(const std::vector<H3Index> &indexes);
    void clearingStarted();
    void clearingFinished();

private:
    // [[nodiscard]] std::optional<H3Data *> findCellByID(quint64 id) const;
    // [[nodiscard]] std::optional<H3Data *> findCellByRes(quint8 res) const;
    [[nodiscard]] bool isCoordinateTargetValid(quint8 zoom, const QGeoCoordinate &coordinate) const;
    //[[nodiscard]] QString getColorForResolution(quint8 resolution) const;

    QList<H3Target *> cells_;

    const uint8_t minZoom_c{3};
    const uint8_t maxZoom_c{15};
    std::unordered_map<uint8_t, uint8_t> zoomToRes_;
    const QHash<int, QString> resolutionColors_c = {
        {2, "crimson"},      {3, "orangered"},   {4, "darkorange"},  {5, "orange"},          {6, "gold"},
        {7, "yellow"},       {8, "greenyellow"}, {9, "limegreen"},   {10, "mediumseagreen"}, {11, "turquoise"},
        {12, "deepskyblue"}, {13, "dodgerblue"}, {14, "mediumblue"}, {15, "darkviolet"}};

    std::atomic_bool isClearing_{false};
};

#endif  // QHEXWALKER_H3TARGETSMODEL_H