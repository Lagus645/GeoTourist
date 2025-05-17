#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QVariantMap>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    bool updateDatabaseSchema();

    bool openDatabase();
    void closeDatabase();
    bool initializeDatabase(); // Добавлен новый метод
    QList<QVariantMap> getNearbyPoints(double latitude, double longitude, double maxDistance);

private:
    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
