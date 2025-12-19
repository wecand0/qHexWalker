#include "mapProvider.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>

MapProvider::MapProvider(QObject *parent) : QObject(parent) { tempStyleFile_ = new QTemporaryFile(this); }

MapProvider::~MapProvider() { tempStyleFile_->deleteLater(); }

MapProviderError MapProvider::isSQLiteFileValidOffline(const QString &filePath) {
    // Сначала проверим, существует ли файл
    if (!QFile::exists(filePath)) {
        spdlog::critical("{} {}", "Карта maplibre.mbtiles не существует:", filePath.toStdString());
        return MapProviderError::NoDb;
    }

    // Подключаемся к базе
    const QString connectionName = "temp_connection";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(filePath);

    if (!db.open()) {
        spdlog::critical("{} {}", "Не удалось открыть maplibre.mbtiles:", db.lastError().text().toStdString());
        QSqlDatabase::removeDatabase(connectionName);
        return MapProviderError::CannotOpenDb;
    }

    bool isValid = true;
    {
        if (QSqlQuery query(db); !query.exec("PRAGMA integrity_check;")) {
            spdlog::critical("{} {}", "Не удалось выполнить проверку целостности карты map.mbtiles:",
                             db.lastError().text().toStdString());
            isValid = false;
        } else {
            while (query.next()) {
                if (QString result = query.value(0).toString(); result != "ok") {
                    spdlog::critical("{} {}", "Карта maplibre.mbtiles повреждена:", result.toStdString());
                    isValid = false;
                    break;
                }
            }
        }
    }

    db.close();
    if (!isValid) {
        return MapProviderError::OtherErrors;
    }

    return MapProviderError::Ok;
}

void MapProvider::exchangeUrlOffline(const QString &pathToMap) {
    QFile file;
    file.setFileName(QStringLiteral(":/QHexWalker/style.json"));

    auto isOpened = file.open(QIODevice::ReadOnly);
    if (!isOpened) {
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    data.replace("%URL%", pathToMap.toUtf8());

    tempStyleFile_ = new QTemporaryFile(this);
    isOpened = tempStyleFile_->open();
    if (!isOpened) {
        return;
    }
    tempStyleFile_->write(data);
    tempStyleFile_->close();

    ///setUrl("file:///" + tempStylePreviewFile_->fileName());
    setUrl(/*"file:///" + */ tempStyleFile_->fileName());
}