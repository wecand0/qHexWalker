//
// Created by user on 02/12/2025.
//

#ifndef Q_HEX_WALKER_H3MODEL_H
#define Q_HEX_WALKER_H3MODEL_H

#include <QAbstractListModel>

class H3Data;
namespace H3_VIEWER {
class H3Worker;
}
class H3Model final : public QAbstractListModel {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Model)
public:
    enum Roles { ResRole = Qt::UserRole + 1, IndexRole, CellColor, PathRole };

    explicit H3Model(QObject *parent = nullptr);
    ~H3Model() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private slots:
    void onCellComputed(quint8 res, H3Index index, const QVariantList &polygon, bool isSearching);

public:
    void Init();

public slots:
    Q_INVOKABLE void requestCell(quint8 mapZoom, const QGeoCoordinate &coordinate);
    Q_INVOKABLE void clearAllCells();

signals:
    void clearingStarted();
    void clearingFinished();

private:
    [[nodiscard]] std::optional<H3Data *> findCellByID(quint64 id) const;
    [[nodiscard]] std::optional<H3Data *> findCellByRes(quint8 res) const;
    [[nodiscard]] bool isCoordinateTargetValid(quint8 zoom, const QGeoCoordinate &coordinate) const;
    [[nodiscard]] QString getColorForResolution(quint8 resolution) const;

    H3_VIEWER::H3Worker *worker_{};
    QThread *thread_{};

    QList<H3Data *> cells_;

    uint8_t minZoom_c{3};
    uint8_t maxZoom_c{15};
    std::unordered_map<uint8_t, uint8_t> zoomToRes_;
    const QHash<int, QString> resolutionColors_c = {
        {2, "crimson"},      {3, "orangered"},   {4, "darkorange"},  {5, "orange"},          {6, "gold"},
        {7, "yellow"},       {8, "greenyellow"}, {9, "limegreen"},   {10, "mediumseagreen"}, {11, "turquoise"},
        {12, "deepskyblue"}, {13, "dodgerblue"}, {14, "mediumblue"}, {15, "darkviolet"}};

    bool isClearing_{false};
};

#endif  // Q_HEX_WALKER_H3MODEL_H