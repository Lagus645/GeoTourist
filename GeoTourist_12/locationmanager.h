#ifndef LOCATIONMANAGER_H
#define LOCATIONMANAGER_H

#include <QObject>
#include <QGeoPositionInfoSource>
#include <QGeoCoordinate>

class LocationManager : public QObject
{
    Q_OBJECT

public:
    explicit LocationManager(QObject *parent = nullptr);
    ~LocationManager();

    // Основные методы
    void startLocationUpdates();
    void stopLocationUpdates();
    double calculateDistanceTo(double latitude, double longitude) const;
    static double calculateDistanceTo(double fromLat, double fromLng, double toLat, double toLng);

    // Текущие координаты
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    bool hasValidLocation() const { return m_hasValidLocation; }

signals:
    // Сигналы для обновления позиции и ошибок
    void positionUpdated(double latitude, double longitude);
    void errorOccurred(const QString &error);

private slots:
    void handlePositionUpdated(const QGeoPositionInfo &info);
    void handlePositionError(QGeoPositionInfoSource::Error positioningError);

private:
    QGeoPositionInfoSource *m_positionSource = nullptr;
    double m_latitude = 0.0;
    double m_longitude = 0.0;
    bool m_hasValidLocation = false;
};

#endif // LOCATIONMANAGER_H
