QT += core gui sql network positioning

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    databasemanager.cpp \
    locationmanager.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    databasemanager.h \
    locationmanager.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    GeoTourist_12_ru_RU.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    db.qrc \
    icon.qrc \
    image.qrc

android {
    QT += network
    ANDROID_EXTRA_LIBS += $$PWD/libs/armeabi-v7a/libcrypto.so
    ANDROID_EXTRA_LIBS += $$PWD/libs/armeabi-v7a/libssl.so
    ANDROID_PERMISSIONS = \
        android.permission.ACCESS_FINE_LOCATION \
        android.permission.ACCESS_COARSE_LOCATION \
        android.permission.INTERNET \
        android.permission.ACCESS_NETWORK_STATE
}
