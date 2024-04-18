#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <QObject>
#include <QTimer>
#include <string>
#include "DisplayManager.h" // Ensure this is defined elsewhere in your project
#include "brightness_control.h"

#include <map>
#include <windows.h>

struct ProcessCpuTimes {
    ULARGE_INTEGER lastSysCPU;
    ULARGE_INTEGER lastUserCPU;
    ULARGE_INTEGER lastCPU;
    double previousCpuUsage = 0.0; // Add this line to store the previous CPU usage

};

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
    void restorePriorities();
    DWORD defaultbrightness;
    DWORD minBrightness;
    DWORD normalBrightness;
    bool isOnBatteryPower();
    void startMonitoringBasedOnPowerSource();
    void stopMonitoringAndRestart();
    bool speechModeEnabled;
signals:
    void logMessageAutomatic(QString message);
    void monitoringStarted();
    void monitoringStopped();
    void requestPriorityAdjustment(DWORD processID, DWORD newPriorityClass);
    void powerSourceChangedToAC();
    void brightnessChanged(int newBrightness);
    void refreshRateChanged(int newRefreshRate);
    void powerPlanChanged();

public slots:
    void startNormalModeSlot(bool enableDynamicOptimization);
    void startTurboModeSlot(bool enableDynamicOptimization);
    void stopMonitoringSlot();
    void cleanup();
    void adjustPrioritySlot(DWORD processID, DWORD newPriorityClass);
    void checkAndAdjustProcessPriorities();

    void checkAndAdjustProcessPrioritiesforBattery();

private:
    ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    int numProcessors;
    HANDLE self;
    std::unordered_map<QString, double> aggregatedCpuUsageMap;
    std::unordered_map<QString, SIZE_T> aggregatedMemoryUsageMap;
    std::string lastPowerPlan;
    void initCpuUsage();
    double getProcessCpuUsage(HANDLE process, DWORD processID);
    std::map<DWORD, ProcessCpuTimes> processCpuTimesMap;
    std::map<DWORD, DWORD> originalPriorities;
    QTimer* checkTimer;
    QTimer* checkPriorityTimer;
    DisplayManager displayManager;
    bool menuDisplayed;
    std::string currentPowerPlan;
    long idleThreshold;
    long accumulatedIdleTime;
    long checkInterval;
    long checkPriorityInterval;
    std::string powerSaverName;
    std::string highPerformanceName;
    std::string balancedName;
    bool monitoringActive; // Flag to control the monitoring loop
    void switchPowerPlan(const std::string& powerPlanName);
    bool isSystemIdle();
    void checkIdleAndSwitchPlan();
    bool isHighResourceUsage(DWORD processID);
    QString priorityToString(DWORD priorityClass);
    DWORD determinePriorityBasedOnUsage(SIZE_T memoryUsageMB, double cpuUsagePercent);
    bool dynamicOptimizationEnabled;
    void reduceBackgroundProcessPriorities();
    double calculateCpuUsage(const ProcessCpuTimes& prevCpuTimes, const ULARGE_INTEGER& sys, const ULARGE_INTEGER& user, const ULARGE_INTEGER& now, int numProcessors);


};

#endif // POWERMANAGER_H
