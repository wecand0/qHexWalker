//
// Created by user on 02/12/2025.
//

#ifndef Q_HEX_WALKER_H3DATA_H
#define Q_HEX_WALKER_H3DATA_H

class H3Data final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Data)
    Q_PROPERTY(quint8 res READ res WRITE setRes NOTIFY resChanged)
    Q_PROPERTY(quint64 index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(QVariantList path READ path WRITE setPath NOTIFY pathChanged)

public:
    explicit H3Data(QObject *parent = nullptr);
    ~H3Data() override;

    [[nodiscard("0-15")]] quint8 res() const noexcept;
    [[nodiscard("h3 identificator")]] quint64 index() const noexcept;
    [[nodiscard]] QColor color() const noexcept;
    [[nodiscard]] QGeoCoordinate coordinate() const noexcept;
    [[nodiscard("h3 polygon")]] QVariantList path() const;

public slots:
    void setRes(quint8 res);
    void setIndex(quint64 index);
    void setColor(const QColor &color);
    void setCoordinate(const QGeoCoordinate &coordinate);
    void setPath(const QVariantList &path);

signals:
    void resChanged();
    void indexChanged();
    void colorChanged();
    void coordinateChanged();
    void pathChanged();

private:
    uint8_t res_{};
    uint64_t index_{};
    QColor color_{};
    QGeoCoordinate coordinate_{};
    QVariantList path_;
};

#endif  // Q_HEX_WALKER_H3DATA_H