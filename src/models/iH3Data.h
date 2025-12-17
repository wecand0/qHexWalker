#ifndef QHEXWALKER_IH3DATA_H
#define QHEXWALKER_IH3DATA_H

class IH3Data : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(IH3Data)
    Q_PROPERTY(quint8 res READ res WRITE setRes NOTIFY resChanged)
    Q_PROPERTY(quint64 index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QVariantList path READ path WRITE setPath NOTIFY pathChanged)

public:
    explicit IH3Data(QObject *parent = nullptr);
    ~IH3Data() override;

    [[nodiscard("0-15")]] quint8 res() const noexcept;
    [[nodiscard("h3 identificator")]] quint64 index() const noexcept;
    [[nodiscard]] QColor color() const noexcept;
    [[nodiscard("h3 polygon")]] QVariantList path() const;

public slots:
    void setRes(quint8 res);
    void setIndex(quint64 index);
    void setColor(const QColor &color);
    void setPath(const QVariantList &path);

signals:
    void resChanged();
    void indexChanged();
    void colorChanged();
    void pathChanged();

private:
    quint8 res_{};
    quint64 index_{};
    QColor color_{};
    QVariantList path_;
};

#endif  // QHEXWALKER_IH3DATA_H