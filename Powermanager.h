#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <QObject>
#include <QTimer>
#include <string>
#include "DisplayManager.h" // Ensure this is defined elsewhere in your project
#include "brightness_control.h"

class PowerManager : public QObject {
    Q_OBJECT

public:
    explicit PowerManager(QObject *parent = nullptr);
    ~PowerManager() override;

    void setMenuDisplayed(bool displayed);
    QString GetCurrentPowerPlan();
    void SetPowerPlan(const std::string& planName);
    bool isTurboMode;
    QString removeAccents(const QString &input);

    DWORD defaultbrightness;

signals:
    void logMessageAutomatic(QString message);

public slots:
    void startNormalModeSlot();
    void startTurboModeSlot();
    void stopMonitoringSlot();
    void cleanup();

private:
    QTimer* checkTimer;
    DisplayManager displayManager;
    bool menuDisplayed;
    std::string currentPowerPlan;
    long idleThreshold;
    long accumulatedIdleTime;
    long checkInterval;
    std::string powerSaverName;
    std::string highPerformanceName;
    std::string balancedName;
    bool monitoringActive; // Flag to control the monitoring loop

    void switchPowerPlan(const std::string& powerPlanName);
    bool isSystemIdle();
    void checkIdleAndSwitchPlan();
};

#endif // POWERMANAGER_H
