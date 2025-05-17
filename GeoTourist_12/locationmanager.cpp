#include "locationmanager.h"
#include <QGeoPositionInfoSource>
#include <QGeoCoordinate>
#include <QDebug>

LocationManager::LocationManager(QObject *parent)
    : QObject(parent),
    m_positionSource(QGeoPositionInfoSource::createDefaultSource(this)),
    m_latitude(0.0),
    m_longitude(0.0),
    m_hasValidLocation(false)
{
    if (m_positionSource) {
        // Правильное подключение сигналов
        connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated,
                this, &LocationManager::handlePositionUpdated);

// Исправленный сигнал таймаута (если доступен)
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        connect(m_positionSource, &QGeoPositionInfoSource::errorOccurred,
                this, &LocationManager::handlePositionError);
#else
        connect(m_positionSource, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error),
                this, &LocationManager::handlePositionError);
#endif
    } else {
        qWarning() << "No position source available";
        emit errorOccurred(tr("No position source available on this device"));
    }
}

LocationManager::~LocationManager()
{
    stopLocationUpdates();
}

void LocationManager::startLocationUpdates()
{
    if (!m_positionSource) {
        emit errorOccurred(tr("Position source not available"));
        return;
    }

    m_positionSource->setUpdateInterval(10000);
    m_positionSource->startUpdates();
}

void LocationManager::stopLocationUpdates()
{
    if (m_positionSource) {
        m_positionSource->stopUpdates();
    }
}

double LocationManager::calculateDistanceTo(double latitude, double longitude) const
{
    if (!m_hasValidLocation) {
        return -1.0;
    }
    return QGeoCoordinate(m_latitude, m_longitude).distanceTo(QGeoCoordinate(latitude, longitude));
}

double LocationManager::calculateDistanceTo(double fromLat, double fromLng, double toLat, double toLng)
{
    return QGeoCoordinate(fromLat, fromLng).distanceTo(QGeoCoordinate(toLat, toLng));
}

void LocationManager::handlePositionUpdated(const QGeoPositionInfo &info)
{
    // Исправленная проверка валидности координат
    if (info.coordinate().isValid()) {
        m_latitude = info.coordinate().latitude();
        m_longitude = info.coordinate().longitude();
        m_hasValidLocation = true;

        emit positionUpdated(m_latitude, m_longitude);
    }
}

void LocationManager::handlePositionError(QGeoPositionInfoSource::Error positioningError)
{
    QString errorString;

    switch (positioningError) {
    case QGeoPositionInfoSource::AccessError:
        errorString = tr("Access to position information denied");
        break;
    case QGeoPositionInfoSource::ClosedError:
        errorString = tr("Position source closed");
        break;
    case QGeoPositionInfoSource::NoError:
        return;
    default:
        errorString = tr("Unknown positioning error");
    }

    qWarning() << "Location error:" << errorString;
    emit errorOccurred(errorString);
}
