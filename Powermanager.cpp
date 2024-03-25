#include "PowerManager.h"

#include <windows.h>

#include <iostream>

#include <cstdlib>

#include <mutex>

#include <QDebug>

#include <QProcess>

#include <QRegularExpression>

#include <QStringList>

#include <psapi.h>

#include <QMap>

#include <QMultiMap>

#include "mainwindow.h"
//priority 120000
std::mutex consoleMutex;

PowerManager::PowerManager(QObject * parent): QObject(parent), isTurboMode(false), checkTimer(new QTimer(this)), checkPriorityTimer(new QTimer(this)), menuDisplayed(false),
    currentPowerPlan(""), idleThreshold(180000), accumulatedIdleTime(0), checkInterval(3000), checkPriorityInterval(3000), powerSaverName("Power saver"), highPerformanceName("High performance"), balancedName("Balanced"), monitoringActive(false) {

    connect(checkTimer, & QTimer::timeout, this, & PowerManager::checkIdleAndSwitchPlan);
    connect(checkPriorityTimer, & QTimer::timeout, this, & PowerManager::checkAndAdjustProcessPriorities); // Additional connection for resource check
    connect(this, & PowerManager::requestPriorityAdjustment, this, & PowerManager::adjustPrioritySlot, Qt::QueuedConnection);
}
PowerManager::~PowerManager() {
    if (checkTimer) {
        checkTimer -> stop();
        delete checkTimer;
        checkTimer = nullptr;
    }
    if (checkPriorityTimer) {
        checkPriorityTimer -> stop();
        delete checkPriorityTimer;
        checkPriorityTimer = nullptr;
    }
    restorePriorities();
}
void PowerManager::cleanup() {
    // It's safe to stop the timer here because this slot will be called in the correct thread
    if (checkTimer && checkTimer -> isActive()) {
        checkTimer -> stop();
    }
    if (checkPriorityTimer && checkPriorityTimer -> isActive()) {
        checkPriorityTimer -> stop();
    }
    // Perform any other necessary cleanup.
}


void PowerManager::initCpuUsage() {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo( & sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime( & ftime);
    memcpy( & lastCPU, & ftime, sizeof(FILETIME));

    self = GetCurrentProcess();
    GetProcessTimes(self, & ftime, & ftime, & fsys, & fuser);
    memcpy( & lastSysCPU, & fsys, sizeof(FILETIME));
    memcpy( & lastUserCPU, & fuser, sizeof(FILETIME));
}
const double K = 0.5;

double PowerManager::getProcessCpuUsage(HANDLE process, DWORD processID) {
    FILETIME now, sys, user, ftime;
    ULARGE_INTEGER ul_sys, ul_user, ul_now;

    GetSystemTimeAsFileTime(&now);

    // Now immediately after getting system time, check if GetProcessTimes failed
    if (!GetProcessTimes(process, &ftime, &ftime, &sys, &user)) {
        qDebug() << "Failed to get process times for process ID:" << processID;
        return -1.0; // Indicates failure to get process times
    }

    ul_sys.LowPart = sys.dwLowDateTime;
    ul_sys.HighPart = sys.dwHighDateTime;
    ul_user.LowPart = user.dwLowDateTime;
    ul_user.HighPart = user.dwHighDateTime;
    ul_now.LowPart = now.dwLowDateTime;
    ul_now.HighPart = now.dwHighDateTime;

    // Check and update the map for processID
    if (processCpuTimesMap.find(processID) == processCpuTimesMap.end()) {
        // Initialize if not present
        ProcessCpuTimes cpuTimes = {ul_sys, ul_user, ul_now, 0.0};
        processCpuTimesMap[processID] = cpuTimes;
        return 0.0; // Initial call, no usage data available yet
    }

    // Calculate CPU usage since last update
    ProcessCpuTimes& cpuTimes = processCpuTimesMap[processID];
    double cpuUsage = calculateCpuUsage(cpuTimes, ul_sys, ul_user, ul_now, numProcessors);
    cpuTimes.lastSysCPU = ul_sys;
    cpuTimes.lastUserCPU = ul_user;
    cpuTimes.lastCPU = ul_now;
    cpuTimes.previousCpuUsage = cpuUsage;

    return cpuUsage;
}
double PowerManager::calculateCpuUsage(const ProcessCpuTimes& prevCpuTimes,
                                       const ULARGE_INTEGER& sys,
                                       const ULARGE_INTEGER& user,
                                       const ULARGE_INTEGER& now,
                                       int numProcessors) {
    unsigned long long prevSystemTime = prevCpuTimes.lastSysCPU.QuadPart;
    unsigned long long prevUserTime = prevCpuTimes.lastUserCPU.QuadPart;
    unsigned long long prevTotalTime = prevCpuTimes.lastCPU.QuadPart;

    unsigned long long systemTime = sys.QuadPart;
    unsigned long long userTime = user.QuadPart;
    unsigned long long totalTime = now.QuadPart;

    // Diagnostic logging to help identify issues
    //qDebug() << "System Time Diff:" << (systemTime - prevSystemTime);
    //qDebug() << "User Time Diff:" << (userTime - prevUserTime);
    //qDebug() << "Total Time Diff:" << (totalTime - prevTotalTime);

    unsigned long long systemTimeDiff = systemTime - prevSystemTime;
    unsigned long long userTimeDiff = userTime - prevUserTime;
    unsigned long long totalTimeDiff = totalTime - prevTotalTime;

    if (totalTimeDiff == 0 || systemTimeDiff > totalTimeDiff || userTimeDiff > totalTimeDiff) {
        qDebug() << "Invalid time diff calculation, using previous CPU usage.";
        return prevCpuTimes.previousCpuUsage; // Fallback to previous usage
    }

    unsigned long long totalCpuTimeDiff = systemTimeDiff + userTimeDiff;
    double cpuUsage = (static_cast<double>(totalCpuTimeDiff) * 100.0) / static_cast<double>(totalTimeDiff);
    cpuUsage /= numProcessors;

    //qDebug() << "Calculated CPU Usage:" << cpuUsage;
    return cpuUsage >= 0 ? cpuUsage : 0;
}

QString PowerManager::GetCurrentPowerPlan() {
    QProcess powercfg;
    powercfg.start("powercfg", QStringList() << "/GetActiveScheme");
    powercfg.waitForFinished();
    QByteArray output = powercfg.readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);

    // Make the regular expression object static so it is not recreated every time
    static QRegularExpression re(R"(\(([^)]+)\))");

    QRegularExpressionMatch match = re.match(outputStr);
    if (match.hasMatch()) {
        QString powerPlanName = match.captured(1); // This captures the power plan name e.g., "Balanced"

        // Check if the powerPlanName contains the substring "centrikus"
        if (powerPlanName.contains("centrikus")) {
            // Replace the matched string with the full word
            powerPlanName = "Teljesítménycentrikus";
        }
        if (powerPlanName.contains("Energiatakar")) {
            // Replace the matched string with the full word
            powerPlanName = "Energiatakarékos";
        }
        if (powerPlanName.contains("Kiegyens")) {
            // Replace the matched string with the full word
            powerPlanName = "Kiegyensúlyozott";
        }

        return powerPlanName;
    }
    return "Unknown"; // Return "Unknown" if we can't find a match
}

void PowerManager::SetPowerPlan(const std::string & planName) {
    // This function assumes you have power plans with these exact names.
    // The GUIDs are examples and need to be replaced with the actual GUIDs of the power plans.
    if (currentPowerPlan == planName) {
        return; // No need to switch if it's already the selected plan
    }

    std::string command = "powercfg /s ";
    if (planName == "Balanced") {
        command += "SCHEME_BALANCED";
    } else if (planName == "Power saver") {
        command += "SCHEME_MAX";
        // Adjust screen and sleep settings for Power saver plan
        system("powercfg /change monitor-timeout-dc 1"); // Turn off the display after 1 minute on battery
        system("powercfg /change standby-timeout-dc 10"); // Put the computer to sleep after 10 minutes on battery
    } else if (planName == "High performance") {
        command += "SCHEME_MIN";
        // Adjust screen and sleep settings for High performance plan
        // Example: setting longer or disabled timeouts
        system("powercfg /change monitor-timeout-dc 5"); // Adjust as needed
        system("powercfg /change standby-timeout-dc 20"); // Adjust as needed
    } else {
        emit logMessageAutomatic(QString("Error: Failed to set power plan to \"%1\". Check system configuration.").arg(QString::fromStdString(planName)));
        return;
    }

    system(command.c_str());
    currentPowerPlan = planName; // Update the currentPowerPlan variable
    // Optionally log the successful power plan change
    emit logMessageAutomatic(QString("Power plan changed to \"%1\".").arg(QString::fromStdString(planName)));
}

void PowerManager::switchPowerPlan(const std::string & powerPlanName) {
    if (currentPowerPlan == powerPlanName) {
        return; // No need to switch if it's already the selected plan
    }
    lastPowerPlan = currentPowerPlan;
    std::string command = "powercfg /s ";
    if (powerPlanName == "Balanced") {
        command += "SCHEME_BALANCED";
    } else if (powerPlanName == "Power saver") {
        command += "SCHEME_MAX";
        // Adjust screen and sleep settings for Power saver plan
        system("powercfg /change monitor-timeout-dc 1"); // Turn off the display after 1 minute on battery
        system("powercfg /change standby-timeout-dc 10"); // Put the computer to sleep after 10 minutes on battery
    } else if (powerPlanName == "High performance") {
        command += "SCHEME_MIN";
        // Adjust screen and sleep settings for High performance plan
        // Example: setting longer or disabled timeouts
        system("powercfg /change monitor-timeout-dc 5"); // Adjust as needed
        system("powercfg /change standby-timeout-dc 20"); // Adjust as needed
    } else {
        std::cout << "Power plan \"" << powerPlanName << "\" not recognized." << std::endl;
        return;
    }

    int result = system(command.c_str());
    if (result == 0) {
        currentPowerPlan = powerPlanName; // Update the current power plan
        int maxRefreshRate = displayManager.listSupportedModes().back().second;
        if (powerPlanName == "Balanced" || powerPlanName == "Power saver") {
            displayManager.setDisplayRefreshRate(60); // Set to 60Hz for these plans
        } else if (powerPlanName == "High performance") {
            displayManager.setDisplayRefreshRate(maxRefreshRate); // Set to max for High performance
        }
        emit logMessageAutomatic(QString("Successfully switched to %1 plan. Screen and sleep settings adjusted accordingly.").arg(QString::fromStdString(powerPlanName)));
    } else {
        emit logMessageAutomatic(QString("Failed to switch to %1 plan.").arg(QString::fromStdString(powerPlanName)));
    }
}

bool PowerManager::isSystemIdle() {

    if (!isOnBatteryPower() && isTurboMode == false && monitoringActive && dynamicOptimizationEnabled) { // Assuming dynamicOptimizationEnabled is a flag indicating if dynamic optimization was enabled
        // If we've switched from battery to AC power, emit a signal to stop monitoring
        emit powerSourceChangedToAC(); // You need to define this signal in your PowerManager class
        return false; // Return false to avoid further processing in this call
    }
    LASTINPUTINFO lastInputInfo;
    lastInputInfo.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo( & lastInputInfo);

    ULONGLONG currentTime = GetTickCount64();
    long lastInputTime = lastInputInfo.dwTime;

    // If there was recent activity, reset accumulated idle time
    if ((currentTime - lastInputTime) < static_cast < ULONGLONG > (checkInterval)) {
        accumulatedIdleTime = 0;
        return false;
    }

    // Accumulate idle time only if the system was idle in the last interval
    accumulatedIdleTime += checkInterval;
    return accumulatedIdleTime >= idleThreshold;
}

void PowerManager::setMenuDisplayed(bool displayed) {
    menuDisplayed = displayed;
}

void PowerManager::startNormalModeSlot(bool enableDynamicOptimization) {
    isTurboMode = false;
    dynamicOptimizationEnabled = enableDynamicOptimization;
    SetMonitorBrightness(defaultbrightness);
    if (!monitoringActive) {
        monitoringActive = true;
        if (enableDynamicOptimization) {
            qDebug() << "[Normal Mode] Starting with Dynamic Optimization:" << enableDynamicOptimization;
            if (isOnBatteryPower()) {
                checkAndAdjustProcessPrioritiesforBattery();
                checkPriorityTimer -> start(checkPriorityInterval);
            }

        }
        reduceBackgroundProcessPriorities();
        checkTimer -> start(checkInterval);
        qDebug() << "Normal Mode Monitoring Active with Dynamic Optimization: " << enableDynamicOptimization;
        emit monitoringStarted();
    }
}

void PowerManager::startTurboModeSlot(bool enableDynamicOptimization) {
    isTurboMode = true;
    dynamicOptimizationEnabled = enableDynamicOptimization;
    SetMonitorBrightness(defaultbrightness); // Example brightness adjustment for Turbo Mode

    if (!monitoringActive) {
        monitoringActive = true;
        if (enableDynamicOptimization) {
            qDebug() << "[Turbo Mode] Starting with Dynamic Optimization:" << enableDynamicOptimization;
            checkAndAdjustProcessPriorities();
            checkPriorityTimer -> start(checkPriorityInterval);
        }
        checkTimer -> start(checkInterval);
        qDebug() << "Turbo Mode Monitoring Active with Dynamic Optimization: " << enableDynamicOptimization;
        emit monitoringStarted();
    }

}
bool PowerManager::isOnBatteryPower() {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus( & sps)) {
        return sps.ACLineStatus == 0; // 0 means running on battery, 1 means plugged in
    }
    return false; // Default to false if unable to get power status
}

void PowerManager::checkAndAdjustProcessPrioritiesforBattery() {
    aggregatedCpuUsageMap.clear(); // Clear previous data
    aggregatedMemoryUsageMap.clear(); // Clear previous data
    DWORD processes[1024], bytesNeeded, processesCount;
    if (!EnumProcesses(processes, sizeof(processes), & bytesNeeded)) {
        qDebug() << "Failed to enumerate processes for battery life optimization.";
        return;
    }

    processesCount = bytesNeeded / sizeof(DWORD);
    QMap < QString, QList < DWORD >> executableProcesses;

    for (unsigned int i = 0; i < processesCount; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
        if (!hProcess) continue;

        TCHAR processPath[MAX_PATH] = TEXT("<unknown>");
        GetModuleFileNameEx(hProcess, NULL, processPath, sizeof(processPath) / sizeof(TCHAR));
        QString executableName = QString::fromWCharArray(processPath).split("\\").last();

        double cpuUsage = getProcessCpuUsage(hProcess, processes[i]);
        QString executablePath = QString::fromWCharArray(processPath);
        if (executablePath.startsWith("C:\\Windows\\System32", Qt::CaseInsensitive) ||
            (executablePath.startsWith("C:\\Windows\\", Qt::CaseInsensitive) && !executablePath.contains("\\Windows\\SystemApps\\"))) {
            // This is likely a system process, skip adjustments
            continue;
        }
        aggregatedCpuUsageMap[executableName] += cpuUsage;

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, & pmc, sizeof(pmc))) {
            aggregatedMemoryUsageMap[executableName] += pmc.WorkingSetSize / (1024 * 1024); // Convert to MB
        }

        executableProcesses[executableName].append(processes[i]);

        CloseHandle(hProcess);
    }

    for (auto it = executableProcesses.begin(); it != executableProcesses.end(); ++it) {
        const QString & exeName = it.key();
        const QList < DWORD > & processIds = it.value();

        double totalCpuUsage = aggregatedCpuUsageMap[exeName];
        SIZE_T totalMemoryUsage = aggregatedMemoryUsageMap[exeName];
        DWORD newPriority = determinePriorityBasedOnUsage(totalMemoryUsage, totalCpuUsage);

        //qDebug() << "Executable: " << exeName << " | Total CPU Usage: " << totalCpuUsage << "% | Total Memory Usage: " << totalMemoryUsage << "MB | New Priority: " << priorityToString(newPriority);

        for (DWORD processID: processIds) {
            adjustPrioritySlot(processID, newPriority);
        }
    }
}

void PowerManager::reduceBackgroundProcessPriorities() {
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), & cbNeeded)) {
        return;
    }

    cProcesses = cbNeeded / sizeof(DWORD);

    for (i = 0; i < cProcesses; i++) {
        DWORD processID = aProcesses[i];
        // Open the process with QUERY and SET information privileges
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_SET_INFORMATION, FALSE, processID);
        if (hProcess == NULL) continue;

        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
        HMODULE hMod;
        DWORD cbNeeded;

        // Retrieve the process name
        if (EnumProcessModules(hProcess, & hMod, sizeof(hMod), & cbNeeded)) {
            GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
        }

        QString processName = QString::fromWCharArray(szProcessName);

        // Skip system and essential processes, and adjust as necessary
        if (processName.contains("System", Qt::CaseInsensitive) ||
            processName.contains("svchost", Qt::CaseInsensitive) ||
            processName.contains("explorer.exe", Qt::CaseInsensitive)) {
            CloseHandle(hProcess);
            continue;
        }

        // Retrieve current priority class of the process
        DWORD currentPriority = GetPriorityClass(hProcess);
        if (currentPriority == BELOW_NORMAL_PRIORITY_CLASS || currentPriority == IDLE_PRIORITY_CLASS) {

            CloseHandle(hProcess);
            continue;
        }

        // Otherwise, reduce priority to BELOW_NORMAL
        SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);

        CloseHandle(hProcess);
    }
}

void PowerManager::checkAndAdjustProcessPriorities() {
    aggregatedCpuUsageMap.clear(); // Clear previous data
    aggregatedMemoryUsageMap.clear(); // Clear previous data

    DWORD processes[1024], bytesNeeded, processesCount;
    if (!EnumProcesses(processes, sizeof(processes), & bytesNeeded)) {
        qDebug() << "Failed to enumerate processes.";
        return;
    }

    processesCount = bytesNeeded / sizeof(DWORD);
    QMap < QString, QList < DWORD >> executableProcesses; // Map from executable name to list of process IDs

    for (unsigned int i = 0; i < processesCount; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
        if (!hProcess) continue;

        TCHAR processPath[MAX_PATH] = TEXT("<unknown>");
        GetModuleFileNameEx(hProcess, NULL, processPath, sizeof(processPath) / sizeof(TCHAR));
        QString executableName = QString::fromWCharArray(processPath).split("\\").last();

        double cpuUsage = getProcessCpuUsage(hProcess, processes[i]);
        QString executablePath = QString::fromWCharArray(processPath);
        if (executablePath.startsWith("C:\\Windows\\System32", Qt::CaseInsensitive) ||
            (executablePath.startsWith("C:\\Windows\\", Qt::CaseInsensitive) && !executablePath.contains("\\Windows\\SystemApps\\"))) {
            // This is likely a system process, skip adjustments
            continue;
        }
        aggregatedCpuUsageMap[executableName] += cpuUsage;

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, & pmc, sizeof(pmc))) {
            aggregatedMemoryUsageMap[executableName] += pmc.WorkingSetSize / (1024 * 1024); // Convert to MB
        }

        executableProcesses[executableName].append(processes[i]);

        CloseHandle(hProcess);
    }

    for (auto it = executableProcesses.begin(); it != executableProcesses.end(); ++it) {
        const QString & exeName = it.key();
        const QList < DWORD > & processIds = it.value();

        double totalCpuUsage = aggregatedCpuUsageMap[exeName];
        SIZE_T totalMemoryUsage = aggregatedMemoryUsageMap[exeName];
        DWORD newPriority = determinePriorityBasedOnUsage(totalMemoryUsage, totalCpuUsage);

        //qDebug() << "Executable: " << exeName << " | Total CPU Usage: " << totalCpuUsage << "% | Total Memory Usage: " << totalMemoryUsage << "MB | New Priority: " << priorityToString(newPriority);

        for (DWORD processID: processIds) {
            adjustPrioritySlot(processID, newPriority);
        }
    }
}

// Helper function to determine the new priority based on memory usage and current priority
DWORD PowerManager::determinePriorityBasedOnUsage(SIZE_T memoryUsageMB, double cpuUsagePercent) {
    bool isIdle = isSystemIdle();

    if (!isOnBatteryPower()) {
        // Example logic: adjust these thresholds based on your needs
        if (cpuUsagePercent > 20 || memoryUsageMB > 2000) {
            return ABOVE_NORMAL_PRIORITY_CLASS;
        } else if (cpuUsagePercent > 50 || memoryUsageMB > 4000) {
            return HIGH_PRIORITY_CLASS;
        } else {
            return NORMAL_PRIORITY_CLASS;
        }
    } else if (isIdle) {
        return BELOW_NORMAL_PRIORITY_CLASS;
    } else {
        if (cpuUsagePercent < 10 || memoryUsageMB < 1000) {
            return BELOW_NORMAL_PRIORITY_CLASS;
        } else if (cpuUsagePercent > 20 || memoryUsageMB > 2000) {
            return NORMAL_PRIORITY_CLASS;
        } else {
            return NORMAL_PRIORITY_CLASS;
        }
    }
}

// Helper function to convert priority class to string for logging purposes
QString PowerManager::priorityToString(DWORD priorityClass) {
    switch (priorityClass) {
    case IDLE_PRIORITY_CLASS:
        return "IDLE";
    case BELOW_NORMAL_PRIORITY_CLASS:
        return "BELOW_NORMAL";
    case NORMAL_PRIORITY_CLASS:
        return "NORMAL";
    case ABOVE_NORMAL_PRIORITY_CLASS:
        return "ABOVE_NORMAL";
    case HIGH_PRIORITY_CLASS:
        return "HIGH";
    case REALTIME_PRIORITY_CLASS:
        return "REALTIME";
    default:
        return "UNKNOWN";
    }
}

void PowerManager::adjustPrioritySlot(DWORD processID, DWORD newPriorityClass) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION, FALSE, processID);
    if (!hProcess) {
        qDebug() << "[Error] Failed to open process for adjustment, Process ID:" << processID;
        return;
    }

    DWORD currentPriority = GetPriorityClass(hProcess);
    if (!currentPriority) {
        qDebug() << "[Error] Failed to get current priority for process, Process ID:" << processID;
        CloseHandle(hProcess);
        return;
    }

    // Check if the process's current priority matches the new priority
    if (currentPriority != newPriorityClass) {
        // Priority needs to be changed to the new value
        if (SetPriorityClass(hProcess, newPriorityClass)) {
            //qDebug() << "[Priority Adjustment] Successfully changed priority for Process ID:" << processID << " to new priority.";
        } else {
            qDebug() << "[Error] Failed to change priority for Process ID:" << processID;
        }
    }

    CloseHandle(hProcess);
}

void PowerManager::stopMonitoringSlot() {
    qDebug() << "Stopping Monitoring";
    if (monitoringActive) {
        // Stop the regular monitoring timer
        checkTimer -> stop();

        // Also stop the priority check timer to cease priority adjustments
        if (checkPriorityTimer -> isActive()) {
            checkPriorityTimer -> stop();
            qDebug() << "Priority optimization stopped.";

            restorePriorities(); // Restore the original priorities of any adjusted processes
        }

        monitoringActive = false;
        SetMonitorBrightness(defaultbrightness); // Reset brightness to default
        qDebug() << "Monitoring Stopped";
    }

    emit monitoringStopped();
}

void PowerManager::checkIdleAndSwitchPlan() {
    //qDebug() << "Checking system idle status and adjusting power plan accordingly.";

    bool isIdle = isSystemIdle();
    //qDebug() << "System Idle Status:" << isIdle << ", Current Power Plan:" << QString::fromStdString(currentPowerPlan);
    std::string previousPowerPlan = currentPowerPlan;

    if (currentMode == Mode::Normal || (currentMode == Mode::Dynamic && (currentPowerPlan == powerSaverName || currentPowerPlan == balancedName))) {
        reduceBackgroundProcessPriorities();
    }

    if (isOnBatteryPower()) {
        if ((currentMode == Mode::Dynamic || currentMode == Mode::Normal) && !isIdle) {
            switchPowerPlan(powerSaverName);
            SetMonitorBrightness(defaultbrightness);
        } else if (isIdle) {
            qDebug() << "Switching to Power Saver due to idle status.";
            switchPowerPlan(powerSaverName);
            SetMonitorBrightness(30);
        }
    } else { // Not on battery power
        if (currentMode == Mode::Dynamic || currentMode == Mode::Turbo) {
            switchPowerPlan(highPerformanceName);
            SetMonitorBrightness(defaultbrightness);
        }
    }
    if ((previousPowerPlan == highPerformanceName && currentPowerPlan == powerSaverName) ||
        (previousPowerPlan == powerSaverName && currentPowerPlan == highPerformanceName)) {
        qDebug() << "Power plan switched. Adjusting process priorities.";
        checkAndAdjustProcessPriorities();
    }
}

void PowerManager::startMonitoringBasedOnPowerSource() {
    if (isOnBatteryPower()) {
        // If on battery, start monitoring with normal mode
        startNormalModeSlot(dynamicOptimizationEnabled);
    } else {
        // If plugged in, start monitoring with turbo mode
        startTurboModeSlot(dynamicOptimizationEnabled);
    }
}

void PowerManager::stopMonitoringAndRestart() {
    stopMonitoringSlot(); // Stop current monitoring
    startMonitoringBasedOnPowerSource(); // Start monitoring with the correct settings based on power source
}

void PowerManager::restorePriorities() {
    DWORD processes[1024], bytesNeeded, processesCount;
    if (!EnumProcesses(processes, sizeof(processes), & bytesNeeded)) {
        qDebug() << "Failed to enumerate processes for priority restoration.";
        return;
    }

    processesCount = bytesNeeded / sizeof(DWORD);

    for (unsigned int i = 0; i < processesCount; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, processes[i]);
        if (!hProcess) continue; // Skip to the next process if unable to open

        DWORD currentPriority = GetPriorityClass(hProcess);
        if (currentPriority == 0) {
            CloseHandle(hProcess); // Close handle and continue if unable to get priority
            continue;
        }

        // Reset priority to NORMAL for all processes except those already at NORMAL_PRIORITY_CLASS
        if (currentPriority != NORMAL_PRIORITY_CLASS) {
            if (!SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS)) {
                qDebug() << "Failed to reset priority for process with PID:" << processes[i];
            } else {
                //qDebug() << "Priority reset to NORMAL for process with PID:" << processes[i];
            }
        }

        CloseHandle(hProcess);
    }
    qDebug() << "[Restore Priorities] Process priority restoration completed.";
}
