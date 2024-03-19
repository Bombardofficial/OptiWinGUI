QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Application.cpp \
    Displaymanager.cpp \
    PowerMonitoringManager.cpp \
    Powermanager.cpp \
    Processmanager.cpp \
    brightness_control.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Application.h \
    Displaymanager.h \
    PowerMonitoringManager.h \
    Powermanager.h \
    Processmanager.h \
    brightness_control.h \
    mainwindow.h
TARGET = OptiWin
FORMS += \
    mainwindow.ui

win32:RC_ICONS += OptiWinLogo-transparentmain2.ico

TRANSLATIONS += \
    OptiWinUI_de_AT.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lDxva2

RESOURCES += \
    resources.qrc

DISTFILES += \
    OptiWinLogo-transparent.png \
    OptiWinLogo-transparentmain2.ico
