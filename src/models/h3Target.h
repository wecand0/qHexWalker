#ifndef Q_HEX_WALKER_H3TARGET_H
#define Q_HEX_WALKER_H3TARGET_H

#include "iH3Data.h"

class H3Target final : public IH3Data {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Target)
    Q_PROPERTY(quint8 zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(quint16 order READ order WRITE setOrder NOTIFY orderChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

public:
    explicit H3Target(QObject *parent = nullptr);
    ~H3Target() override;

    [[nodiscard]] quint8 zoom() const noexcept;
    [[nodiscard]] quint16 order() const noexcept;
    [[nodiscard]] QGeoCoordinate coordinate() const noexcept;

public slots:
    void setZoom(quint8 zoom);
    void setOrder(quint16 order);
    void setCoordinate(const QGeoCoordinate &coordinate);

signals:
    void zoomChanged();
    void orderChanged();
    void coordinateChanged();

private:
    quint8 zoom_{};
    quint16 order_{};
    QGeoCoordinate coordinate_{};
};

#endif  // Q_HEX_WALKER_H3TARGET_H