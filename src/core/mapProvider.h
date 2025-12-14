#ifndef Q_HEX_WALKER_MAP_PROVIDER_H
#define Q_HEX_WALKER_MAP_PROVIDER_H

class QTemporaryFile;

enum class MapProviderError { Ok = 0, NoDb, CannotOpenDb, OtherErrors };

class MapProvider final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)

public:
    explicit MapProvider(QObject *parent = nullptr);

    ~MapProvider() override;

    [[nodiscard]] QString url() const noexcept { return url_; }
    static MapProviderError isSQLiteFileValidOffline(const QString &filePath);
    void exchangeUrlOffline(const QString &pathToMap);

public slots:
    void setUrl(const QString &url) noexcept {
        if (url_ != url) {
            url_ = url;
            emit urlChanged();
        }
    }

signals:
    void urlChanged();

private:
    QString url_;
    QTemporaryFile *tempStyleFile_;
};

#endif  // Q_HEX_WALKER_MAP_PROVIDER_H