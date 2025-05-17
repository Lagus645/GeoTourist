#include "databasemanager.h"
#include "locationmanager.h"
#include <QFile>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
#ifdef QT_DEBUG
    QFile::remove("points.db"); // Удаляем в режиме отладки
#endif

    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName("points.db");
    qDebug() << "Database path:" << m_database.databaseName();
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::openDatabase()
{
    if (!m_database.open()) {
        qWarning() << "Error opening database:" << m_database.lastError().text();
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initializeDatabase() {
    if (!openDatabase()) return false;

#ifdef QT_DEBUG
    // В режиме отладки всегда пересоздаем структуру
    QSqlQuery("DROP TABLE IF EXISTS points");
#endif
    // Проверяем, есть ли таблица points
    if (!m_database.tables().contains("points")) {
        // Таблицы нет - создаем новую из скрипта
        QFile scriptFile(":/bd/init_db.sql");
        if (!scriptFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open SQL script";
            return false;
        }

        QSqlQuery query;
        QStringList scriptQueries = QString(scriptFile.readAll()).split(';', Qt::SkipEmptyParts);

        for (const QString &queryStr : scriptQueries) {
            if (queryStr.trimmed().isEmpty()) continue;
            if (!query.exec(queryStr)) {
                qWarning() << "Query failed:" << query.lastError().text();
                return false;
            }
        }
    } else {
        // Таблица существует - обновляем схему
        if (!updateDatabaseSchema()) {
            return false;
        }
    }

    return true;
}

QList<QVariantMap> DatabaseManager::getNearbyPoints(double latitude, double longitude, double maxDistance)
{
    QList<QVariantMap> result;
    if (!m_database.isOpen()) return result;

    QSqlQuery query;
    query.prepare("SELECT id, name, description, address, latitude, longitude, image_path FROM points");

    if (!query.exec()) {
        qWarning() << "Query error:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        double pointLat = query.value(4).toDouble();
        double pointLng = query.value(5).toDouble();
        double distance = LocationManager::calculateDistanceTo(latitude, longitude, pointLat, pointLng);

        if (distance <= maxDistance) {
            QVariantMap point;
            point["id"] = query.value(0);
            point["name"] = query.value(1);
            point["description"] = query.value(2);
            point["address"] = query.value(3);  // Добавляем адрес
            point["latitude"] = pointLat;
            point["longitude"] = pointLng;
            point["image_path"] = query.value(6);
            point["distance"] = distance;
            result.append(point);
        }
    }

    return result;
}

bool DatabaseManager::updateDatabaseSchema()
{
    if (!openDatabase()) return false;

    // Проверяем, есть ли столбец address
    QSqlQuery checkQuery("PRAGMA table_info(points)");
    bool hasAddressColumn = false;

    while (checkQuery.next()) {
        if (checkQuery.value(1).toString() == "address") {
            hasAddressColumn = true;
            break;
        }
    }

    // Если столбца нет - добавляем
    if (!hasAddressColumn) {
        QSqlQuery alterQuery;
        if (!alterQuery.exec("ALTER TABLE points ADD COLUMN address TEXT")) {
            qWarning() << "Failed to add address column:" << alterQuery.lastError().text();
            return false;
        }
        qDebug() << "Added address column to existing database";
    }

    return true;
}
