#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager;
    QTextEdit *m_pointInfoLabel;  // Изменено с QLabel на QTextEdit
    QLabel *m_mapLabel;
    QNetworkAccessManager *m_networkManager;
    QListWidget *m_pointsListWidget;
    QPushButton *m_toggleViewButton;
    QPushButton *m_settingsButton;
    QWidget *m_settingsPanel;
    double m_currentLat = 0.0;
    double m_currentLng = 0.0;
    QList<QVariantMap> m_nearbyPoints;
    double m_searchRadius = 500.0;
    int m_selectedPointId = -1;

    void setupUI();
    void initSSL();
    void showPointInfo(const QVariantMap &point);
    void loadYandexStaticMap(double pointLat, double pointLng);
    void updateMapImage(const QByteArray &imageData);
    void setupPointsListView();
    void showPointsList();
    void showSinglePoint();
    void initLocationManager();
    void checkNearbyPoints(double lat, double lng);

private slots:
    void updatePosition(double lat, double lng);
    void showLocationError(const QString &error);
    void handleMapImageDownloaded();
    void onPointSelected(QListWidgetItem *item);
    void onRadiusChanged(int value);
};

#endif // MAINWINDOW_H
