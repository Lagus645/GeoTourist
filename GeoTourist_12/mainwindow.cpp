#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "locationmanager.h"
#include "databasemanager.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QDebug>
#include <QMessageBox>
#include <QGeoCoordinate>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QSslSocket>
#include <QNetworkAccessManager>
#include <QListWidgetItem>
#include <QPushButton>
#include <QJniObject>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
void checkAndRequestPermissions()
{
    QStringList permissions = {
        "android.permission.ACCESS_FINE_LOCATION",
        "android.permission.ACCESS_COARSE_LOCATION",
        "android.permission.INTERNET"
    };

    QJniEnvironment env;
    jclass stringClass = env.findClass("java/lang/String");
    jobjectArray jPermissions = env->NewObjectArray(permissions.size(), stringClass, nullptr);

    for (int i = 0; i < permissions.size(); ++i) {
        QJniObject jniString = QJniObject::fromString(permissions.at(i));
        env->SetObjectArrayElement(jPermissions, i, jniString.object());
    }

    bool needRequest = false;
    for (const QString &permission : permissions) {
        auto result = QJniObject::callStaticMethod<jint>(
            "androidx/core/content/ContextCompat",
            "checkSelfPermission",
            "(Landroid/content/Context;Ljava/lang/String;)I",
            QNativeInterface::QAndroidApplication::context(),
            QJniObject::fromString(permission).object<jstring>());

        if (result != 0) {
            needRequest = true;
            break;
        }
    }

    if (needRequest) {
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative",
            "activity",
            "()Landroid/app/Activity;");

        if (activity.isValid()) {
            QJniObject::callStaticMethod<void>(
                "androidx/core/app/ActivityCompat",
                "requestPermissions",
                "(Landroid/app/Activity;[Ljava/lang/String;I)V",
                activity.object(),
                jPermissions,
                0);
        }
    }
}
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dbManager(new DatabaseManager(this))
    , m_pointInfoLabel(nullptr)
    , m_mapLabel(new QLabel(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pointsListWidget(new QListWidget(this))
    , m_toggleViewButton(new QPushButton("Показать все точки", this))
    , m_settingsButton(new QPushButton("Настройки", this))
    , m_settingsPanel(new QWidget(this))
{
    ui->setupUi(this);
    setStyleSheet("background-color: white;");

    initSSL();

    m_mapLabel->setAlignment(Qt::AlignCenter);
    m_mapLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mapLabel->setMinimumSize(400, 300);
    m_mapLabel->setStyleSheet("QLabel { background-color: #F0F0F0; }");

#ifdef Q_OS_ANDROID
    checkAndRequestPermissions();
#endif

    if (!m_dbManager->openDatabase()) {
        qWarning() << "Failed to open database";
        ui->label->setText("Ошибка открытия базы данных");
        return;
    }

    if (!m_dbManager->initializeDatabase()) {
        qWarning() << "Failed to initialize database";
        ui->label->setText("Ошибка инициализации базы данных");
        return;
    }

    setupUI();
    setupPointsListView();
    initLocationManager();
}

void MainWindow::initSSL()
{
    qDebug() << "SSL supported:" << QSslSocket::supportsSsl();
    qDebug() << "SSL version:" << QSslSocket::sslLibraryVersionString();

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    QSslConfiguration::setDefaultConfiguration(sslConfig);
}

void MainWindow::setupUI()
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    QLabel *imageLabel = new QLabel(this);
    QPixmap logo(":/logo/logo.png");
    if(!logo.isNull()) {
        imageLabel->setPixmap(logo.scaledToWidth(200, Qt::SmoothTransformation));
    } else {
        imageLabel->setText("Логотип");
    }
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("margin-top: 0; padding-top: 10px;");

    ui->label->setAlignment(Qt::AlignCenter);
    ui->label->setStyleSheet("QLabel { color: black; font-size: 16px; margin: 10px; }");

    QWidget *infoContainer = new QWidget(this);
    infoContainer->setStyleSheet(
        "QWidget {"
        "   background-color: #F5F5DC;"
        "   border-radius: 15px;"
        "   padding: 15px;"
        "}"
        );

    QVBoxLayout *infoLayout = new QVBoxLayout(infoContainer);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(10);

    m_mapLabel = new QLabel(infoContainer);
    m_mapLabel->setAlignment(Qt::AlignCenter);
    m_mapLabel->setStyleSheet("QLabel { background-color: #F5F5DC; border-radius: 10px; }");
    m_mapLabel->setMinimumSize(350, 300);
    m_mapLabel->setMaximumSize(350, 300);
    m_mapLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_mapLabel->setScaledContents(false);

    // Заменяем QLabel на QTextEdit
    m_pointInfoLabel = new QTextEdit(infoContainer);
    m_pointInfoLabel->setReadOnly(true);
    m_pointInfoLabel->setAlignment(Qt::AlignCenter);
    m_pointInfoLabel->setStyleSheet(
        "QTextEdit {"
        "   color: #333333;"
        "   font-size: 14px;"
        "   padding: 10px;"
        "   border: none;"
        "   background: transparent;"
        "}");
    m_pointInfoLabel->setFrameShape(QFrame::NoFrame);
    m_pointInfoLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pointInfoLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    infoLayout->addWidget(m_mapLabel);
    infoLayout->addWidget(m_pointInfoLabel);

    mainLayout->addWidget(imageLabel, 0, Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->addWidget(ui->label);
    mainLayout->addWidget(infoContainer);

    setCentralWidget(container);
}

void MainWindow::setupPointsListView()
{
    m_pointsListWidget->setStyleSheet(
        "QListWidget {"
        "   background-color: #F5F5DC;"
        "   border-radius: 15px;"
        "   color: black;"
        "}"
        "QListWidget::item {"
        "   border-bottom: 1px solid #DDD;"
        "   padding: 10px;"
        "   color: black;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #EEE;"
        "}"
        );

    m_pointsListWidget->setWordWrap(true);
    m_pointsListWidget->setTextElideMode(Qt::ElideRight);
    m_pointsListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pointsListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pointsListWidget->setMaximumWidth(400);
    m_pointsListWidget->hide();
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->addWidget(m_pointsListWidget);

    m_toggleViewButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #8B4513;"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #A0522D;"
        "}"
        );
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->addWidget(m_toggleViewButton);

    m_settingsButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #000000;"
        "   color: white;"
        "   border-radius: 5px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #556B2F;"
        "}"
        );
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->addWidget(m_settingsButton);

    m_settingsPanel->setStyleSheet(
        "QWidget {"
        "   background-color: #F5F5DC;"
        "   border-radius: 15px;"
        "   padding: 15px;"
        "}"
        );
    m_settingsPanel->hide();

    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsPanel);
    settingsLayout->setContentsMargins(10, 10, 10, 10);
    settingsLayout->setSpacing(10);

    QHBoxLayout *radiusLayout = new QHBoxLayout();
    QLabel *radiusLabel = new QLabel("Радиус поиска (м):", m_settingsPanel);
    radiusLabel->setStyleSheet("QLabel { color: black; font-size: 14px; }");

    QSpinBox *radiusSpinBox = new QSpinBox(m_settingsPanel);
    radiusSpinBox->setRange(100, 20000);
    radiusSpinBox->setSingleStep(100);
    radiusSpinBox->setValue(m_searchRadius);
    radiusSpinBox->setSuffix(" м");
    radiusSpinBox->setStyleSheet(
        "QSpinBox {"
        "   padding: 5px;"
        "   color: black;"
        "}"
        );

    radiusLayout->addWidget(radiusLabel);
    radiusLayout->addWidget(radiusSpinBox);
    radiusLayout->addStretch();

    settingsLayout->addLayout(radiusLayout);
    qobject_cast<QVBoxLayout*>(centralWidget()->layout())->addWidget(m_settingsPanel);

    connect(m_toggleViewButton, &QPushButton::clicked, this, [this]() {
        if (m_pointsListWidget->isVisible()) {
            showSinglePoint();
        } else {
            showPointsList();
        }
    });

    connect(m_settingsButton, &QPushButton::clicked, this, [this]() {
        setUpdatesEnabled(false);
        m_settingsPanel->setVisible(!m_settingsPanel->isVisible());
        setUpdatesEnabled(true);
    });

    connect(radiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
                m_searchRadius = value;
                checkNearbyPoints(m_currentLat, m_currentLng);
            });

    connect(m_pointsListWidget, &QListWidget::itemClicked, this, &MainWindow::onPointSelected);
}

void MainWindow::onRadiusChanged(int value)
{
    m_searchRadius = value;
    checkNearbyPoints(m_currentLat, m_currentLng);
}

void MainWindow::showPointsList()
{
    layout()->setEnabled(true);
    setUpdatesEnabled(true);

    m_pointInfoLabel->hide();
    m_mapLabel->hide();
    m_pointsListWidget->show();
    m_toggleViewButton->setText("Показать карту");

    m_pointsListWidget->clear();
    for (const QVariantMap &point : m_nearbyPoints) {
        QString text = QString("%1\n%2 м").arg(point["name"].toString()).arg(point["distance"].toInt());
        QListWidgetItem *item = new QListWidgetItem(text, m_pointsListWidget);
        item->setData(Qt::UserRole, point["id"]);
        item->setTextAlignment(Qt::AlignLeft);
        item->setToolTip(point["name"].toString());
    }

    layout()->setEnabled(true);
    setUpdatesEnabled(true);
}

void MainWindow::showSinglePoint()
{
    layout()->setEnabled(true);
    setUpdatesEnabled(true);

    m_pointsListWidget->hide();
    m_pointInfoLabel->show();
    m_mapLabel->show();
    m_toggleViewButton->setText("Показать все точки");

    if (!m_nearbyPoints.isEmpty()) {
        showPointInfo(m_nearbyPoints.first());
    }

    layout()->setEnabled(true);
    setUpdatesEnabled(true);
}

void MainWindow::onPointSelected(QListWidgetItem *item)
{
    m_selectedPointId = item->data(Qt::UserRole).toInt();
    for (const QVariantMap &point : m_nearbyPoints) {
        if (point["id"].toInt() == m_selectedPointId) {
            showPointInfo(point);
            break;
        }
    }
}

void MainWindow::showPointInfo(const QVariantMap &point)
{
    QString info = QString("<div style='text-align: center;'>"
                           "<b style='font-size: 16px; color: #8B4513;'>%1</b><br>"
                           "<span style='font-size: 14px;'>%2</span><br>"
                           "<span style='font-size: 12px; color: #696969;'>(%3 м)</span><br>"
                           "<span style='font-size: 13px;'>%4</span>"
                           "</div>")
                       .arg(point["name"].toString())
                       .arg(point["address"].toString())
                       .arg(point["distance"].toInt())
                       .arg(point["description"].toString());

    m_pointInfoLabel->setHtml(info);  // Используем setHtml вместо setText
    loadYandexStaticMap(point["latitude"].toDouble(), point["longitude"].toDouble());
}
void MainWindow::loadYandexStaticMap(double pointLat, double pointLng)
{
    const QString apiKey = "a5183891-5776-4fa6-a843-fac9320c41f3";
    QUrl url("http://static-maps.yandex.ru/1.x/");
    QUrlQuery query;

    double centerLat = (m_currentLat + pointLat) / 2;
    double centerLng = (m_currentLng + pointLng) / 2;

    query.addQueryItem("ll", QString("%1,%2")
                                 .arg(centerLng, 0, 'f', 6)
                                 .arg(centerLat, 0, 'f', 6));
    query.addQueryItem("z", "14");
    query.addQueryItem("size", "400,300");

    QString marks = QString("%1,%2,pm2rdl~%3,%4,pm2gnl")
                        .arg(m_currentLng, 0, 'f', 6)
                        .arg(m_currentLat, 0, 'f', 6)
                        .arg(pointLng, 0, 'f', 6)
                        .arg(pointLat, 0, 'f', 6);

    query.addQueryItem("pt", marks);
    query.addQueryItem("apikey", apiKey);
    query.addQueryItem("l", "map");

    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    request.setSslConfiguration(sslConfig);

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::handleMapImageDownloaded);
    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        qDebug() << "Network error occurred:" << error;
        m_mapLabel->setText("Ошибка загрузки карты");
    });
}

void MainWindow::handleMapImageDownloaded()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyDeleter(reply);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error:" << reply->errorString();
        m_mapLabel->setText("Ошибка сети: " + reply->errorString());
        return;
    }

    const QByteArray imageData = reply->readAll();
    if (imageData.isEmpty()) {
        m_mapLabel->setText("Пустой ответ от сервера");
        return;
    }

    QPixmap map;
    if (!map.loadFromData(imageData)) {
        m_mapLabel->setText("Неверный формат изображения");
        return;
    }

    QSize labelSize = m_mapLabel->size();
    QPixmap scaled = map.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap rounded(scaled.size());
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(scaled));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rounded.rect(), 10, 10);

    m_mapLabel->setPixmap(rounded);
}

void MainWindow::checkNearbyPoints(double lat, double lng)
{
    m_nearbyPoints = m_dbManager->getNearbyPoints(lat, lng, m_searchRadius);

    if (!m_nearbyPoints.isEmpty()) {
        bool selectedPointStillAvailable = false;
        QVariantMap pointToShow;

        if (m_selectedPointId != -1) {
            for (const QVariantMap &point : m_nearbyPoints) {
                if (point["id"].toInt() == m_selectedPointId) {
                    selectedPointStillAvailable = true;
                    pointToShow = point;
                    break;
                }
            }
        }

        if (!selectedPointStillAvailable) {
            m_selectedPointId = -1;
            double minDistance = m_searchRadius;
            for (const QVariantMap &point : m_nearbyPoints) {
                double distance = point["distance"].toDouble();
                if (distance < minDistance) {
                    minDistance = distance;
                    pointToShow = point;
                }
            }
        }

        if (!m_pointsListWidget->isVisible()) {
            showPointInfo(pointToShow);
        }
    } else {
        m_selectedPointId = -1;
        m_pointInfoLabel->setText("Рядом нет интересных точек");
        m_mapLabel->setText("Карта не доступна");
    }
}

void MainWindow::initLocationManager()
{
    LocationManager *locationManager = new LocationManager(this);

    connect(locationManager, &LocationManager::positionUpdated,
            this, &MainWindow::updatePosition);
    connect(locationManager, &LocationManager::errorOccurred,
            this, &MainWindow::showLocationError);

    locationManager->startLocationUpdates();
}

void MainWindow::updatePosition(double lat, double lng)
{
    m_currentLat = lat;
    m_currentLng = lng;
    checkNearbyPoints(lat, lng);
}
void MainWindow::showLocationError(const QString &error)
{
    ui->label->setText("Ошибка: " + error);
    QMessageBox::warning(this, "Ошибка геолокации", error);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_dbManager;
    delete m_networkManager;
    delete m_mapLabel;
}
