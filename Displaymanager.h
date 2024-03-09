#pragma once
#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H
#include <QObject>
#include <windows.h>
#include <vector>
#include <utility>
class DisplayManager : public QObject {
    Q_OBJECT
public:
    DisplayManager(QObject *parent = nullptr);
    bool setDisplayRefreshRate(int refreshRate);
    int getCurrentRefreshRate();
    std::vector<std::pair<int, int>> listSupportedModes();
private:
    DEVMODE getDevMode();
signals:
    void logMessage(QString message);
};

#endif // DISPLAYMANAGER_H
