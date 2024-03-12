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
//priority 120000
std::mutex consoleMutex;

PowerManager::PowerManager(QObject *parent)
    : QObject(parent), isTurboMode(false), checkTimer(new QTimer(this)), checkPriorityTimer(new QTimer(this)), menuDisplayed(false),
    currentPowerPlan(""), idleThreshold(180000), accumulatedIdleTime(0), checkInterval(3000), checkPriorityInterval(6000),powerSaverName("Power saver"),highPerformanceName("High performance"), balancedName("Balanced"), monitoringActive(false) {

    connect(checkTimer, &QTimer::timeout, this, &PowerManager::checkIdleAndSwitchPlan);
    connect(checkPriorityTimer, &QTimer::timeout, this, &PowerManager::checkAndAdjustProcessPriorities); // Additional connection for resource check
    connect(this, &PowerManager::requestPriorityAdjustment, this, &PowerManager::adjustPrioritySlot, Qt::QueuedConnection);
}
PowerManager::~PowerManager() {
    if (checkTimer) {
        checkTimer->stop();
        delete checkTimer;
        checkTimer = nullptr;
    }
    if (checkPriorityTimer) {
        checkPriorityTimer->stop();
        delete checkPriorityTimer;
        checkPriorityTimer = nullptr;
    }
    restorePriorities();
}
void PowerManager::cleanup() {
    // It's safe to stop the timer here because this slot will be called in the correct thread
    if (checkTimer && checkTimer->isActive()) {
        checkTimer->stop();
    }
    if (checkPriorityTimer && checkPriorityTimer->isActive()) {
        checkPriorityTimer->stop();
    }
    // Perform any other necessary cleanup.
}

void PowerManager::initCpuUsage() {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    self = GetCurrentProcess();
    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}
const double K = 0.5;

double PowerManager::getProcessCpuUsage(HANDLE process, DWORD processID) {
    FILETIME ftime, fsys, fuser, fnow;
    ULARGE_INTEGER now, sys, user;
    double percent = -1.0;

    GetSystemTimeAsFileTime(&fnow);
    memcpy(&now, &fnow, sizeof(FILETIME));

    if (!GetProcessTimes(process, &ftime, &ftime, &fsys, &fuser)) {
        qDebug() << "Failed to get process times for process ID:" << processID;
        return percent;
    }

    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    ProcessCpuTimes& cpuTimes = processCpuTimesMap[processID];
    unsigned long long sysDiff = sys.QuadPart - cpuTimes.lastSysCPU.QuadPart;
    unsigned long long userDiff = user.QuadPart - cpuTimes.lastUserCPU.QuadPart;
    unsigned long long nowDiff = now.QuadPart - cpuTimes.lastCPU.QuadPart;

    if (nowDiff > 0) {
        double currentUsage = (double)(sysDiff + userDiff) * 100.0 / (double)nowDiff / numProcessors;
        if (cpuTimes.previousCpuUsage == 0.0) { // Initial setup with the first actual usage calculation
            cpuTimes.previousCpuUsage = currentUsage;
        }
        percent = K * currentUsage + (1 - K) * cpuTimes.previousCpuUsage;

        if (percent < 0.0) { // Ensure percent is not negative
            percent = 0.0;
        }
    } else {
        qDebug() << "No elapsed time for CPU usage calculation for process ID:" << processID;
    }

    cpuTimes.lastSysCPU = sys;
    cpuTimes.lastUserCPU = user;
    cpuTimes.lastCPU = now;
    cpuTimes.previousCpuUsage = percent; // Store the current CPU usage for next time

    return percent;
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

void PowerManager::SetPowerPlan(const std::string& planName) {
    // This function assumes you have power plans with these exact names.
    // The GUIDs are examples and need to be replaced with the actual GUIDs of the power plans.
    if (currentPowerPlan == planName) {
        return; // No need to switch if it's already the selected plan
    }
    std::string command;
    if (planName == "Balanced") {
        command = "powercfg /s SCHEME_BALANCED";
    }
    else if (planName == "Power saver") {
        command = "powercfg /s SCHEME_MAX";
    }
    else if (planName == "High performance") {
        command = "powercfg /s SCHEME_MIN";
    }
    else {
        emit logMessageAutomatic(QString("Error: Failed to set power plan to \"%1\". Check system configuration.").arg(QString::fromStdString(planName)));
        return;
    }

    system(command.c_str());
}


void PowerManager::switchPowerPlan(const std::string& powerPlanName) {
    if (currentPowerPlan == powerPlanName) {
        return; // No need to switch if it's already the selected plan
    }
    std::string command;
    if (powerPlanName == "Balanced") {
        command = "powercfg /s SCHEME_BALANCED";
    }
    else if (powerPlanName == "Power saver") {
        command = "powercfg /s SCHEME_MAX";
    }
    else if (powerPlanName == "High performance") {
        command = "powercfg /s SCHEME_MIN";
    }
    else {
        std::cout << "Power plan \"" << powerPlanName << "\" not recognized." << std::endl;
        return;
    }
    int result = system(command.c_str());
    if (result == 0) {
        currentPowerPlan = powerPlanName; // Update the current power plan
        int maxRefreshRate = displayManager.listSupportedModes().back().second; // Get the maximum refresh rate
         emit logMessageAutomatic(QString("Successfully switched to %1 plan.").arg(QString::fromStdString(powerPlanName)));

        if (powerPlanName == "Balanced" || powerPlanName == "Power saver") {
            displayManager.setDisplayRefreshRate(60); // Set to 60Hz for Balanced and Power saver
        }
        else if (powerPlanName == "High performance") {
            displayManager.setDisplayRefreshRate(maxRefreshRate); // Set to maximum for High performance
        }
    }
    else {
        emit logMessageAutomatic(QString("Failed to switch to %1 plan.").arg(QString::fromStdString(powerPlanName)));
    }
}

bool PowerManager::isSystemIdle() {
    LASTINPUTINFO lastInputInfo;
    lastInputInfo.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lastInputInfo);

    ULONGLONG currentTime = GetTickCount64();
    long lastInputTime = lastInputInfo.dwTime;

    // If there was recent activity, reset accumulated idle time
    if ((currentTime - lastInputTime) < static_cast<ULONGLONG>(checkInterval)) {
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

void PowerManager::startNormalModeSlot() {
    isTurboMode = false;
    SetExternalMonitorBrightness(defaultbrightness);
    qDebug() << "Starting Normal Mode Monitoring";
    if (!monitoringActive) {
        monitoringActive = true;
        checkTimer->start(checkInterval);
        qDebug() << "Normal Mode Monitoring Active";
    }
    emit monitoringStarted();
}

void PowerManager::startTurboModeSlot(bool enableDynamicOptimization) {
    isTurboMode = true;
    SetExternalMonitorBrightness(defaultbrightness); // Example brightness adjustment for Turbo Mode

    if (!monitoringActive) {
        monitoringActive = true;
        if (enableDynamicOptimization) {
            qDebug() << "[Turbo Mode] Starting with Dynamic Optimization:" << enableDynamicOptimization;
            checkAndAdjustProcessPriorities();
            checkPriorityTimer->start(checkPriorityInterval);
        }
        checkTimer->start(checkInterval);
        qDebug() << "Turbo Mode Monitoring Active with Dynamic Optimization: " << enableDynamicOptimization;
        emit monitoringStarted();
    }

}

void PowerManager::checkAndAdjustProcessPriorities() {
    aggregatedCpuUsageMap.clear(); // Clear previous data
    aggregatedMemoryUsageMap.clear(); // Clear previous data

    DWORD processes[1024], bytesNeeded, processesCount;
    if (!EnumProcesses(processes, sizeof(processes), &bytesNeeded)) {
        qDebug() << "Failed to enumerate processes.";
        return;
    }

    processesCount = bytesNeeded / sizeof(DWORD);
    QMap<QString, QList<DWORD>> executableProcesses; // Map from executable name to list of process IDs

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
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            aggregatedMemoryUsageMap[executableName] += pmc.WorkingSetSize / (1024 * 1024); // Convert to MB
        }

        executableProcesses[executableName].append(processes[i]);

        CloseHandle(hProcess);
    }

    for (auto it = executableProcesses.begin(); it != executableProcesses.end(); ++it) {
        const QString &exeName = it.key();
        const QList<DWORD> &processIds = it.value();

        double totalCpuUsage = aggregatedCpuUsageMap[exeName];
        SIZE_T totalMemoryUsage = aggregatedMemoryUsageMap[exeName];
        DWORD newPriority = determinePriorityBasedOnUsage(totalMemoryUsage, totalCpuUsage);

        qDebug() << "Executable: " << exeName << " | Total CPU Usage: " << totalCpuUsage
                 << "% | Total Memory Usage: " << totalMemoryUsage << "MB | New Priority: " << priorityToString(newPriority);

        for (DWORD processID : processIds) {
            adjustPrioritySlot(processID, newPriority);
        }
    }
}


// Helper function to determine the new priority based on memory usage and current priority
DWORD PowerManager::determinePriorityBasedOnUsage(SIZE_T memoryUsageMB, double cpuUsagePercent) {
    // Example logic: adjust these thresholds based on your needs
    if (cpuUsagePercent > 20 || memoryUsageMB > 2000) {
        return ABOVE_NORMAL_PRIORITY_CLASS;
    } else if (cpuUsagePercent > 50 || memoryUsageMB > 4000) {
        return HIGH_PRIORITY_CLASS;
    } else {
        return NORMAL_PRIORITY_CLASS;
    }
}

// Helper function to convert priority class to string for logging purposes
QString PowerManager::priorityToString(DWORD priorityClass) {
    switch (priorityClass) {
    case IDLE_PRIORITY_CLASS: return "IDLE";
    case BELOW_NORMAL_PRIORITY_CLASS: return "BELOW_NORMAL";
    case NORMAL_PRIORITY_CLASS: return "NORMAL";
    case ABOVE_NORMAL_PRIORITY_CLASS: return "ABOVE_NORMAL";
    case HIGH_PRIORITY_CLASS: return "HIGH";
    case REALTIME_PRIORITY_CLASS: return "REALTIME";
    default: return "UNKNOWN";
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
    if (currentPriority == newPriorityClass) {
        // The priority is already set to the desired value, so no action is needed
        qDebug() << "[Info] Process ID:" << processID << " already at desired priority. No adjustment needed.";
    } else {
        // Priority needs to be changed to the new value
        if (SetPriorityClass(hProcess, newPriorityClass)) {
            qDebug() << "[Priority Adjustment] Successfully changed priority for Process ID:" << processID << " to new priority.";
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
        checkTimer->stop();

        // Also stop the priority check timer to cease priority adjustments
        if (checkPriorityTimer->isActive()) {
            checkPriorityTimer->stop();
            qDebug() << "Priority optimization stopped.";
        }

        monitoringActive = false;
        SetExternalMonitorBrightness(defaultbrightness); // Reset brightness to default
        qDebug() << "Monitoring Stopped";
        restorePriorities(); // Restore the original priorities of any adjusted processes
    }

    emit monitoringStopped();
}

void PowerManager::checkIdleAndSwitchPlan() {
    qDebug() << "Checking system idle status and adjusting power plan accordingly.";

    bool isIdle = isSystemIdle();
    qDebug() << "System Idle Status:" << isIdle << ", Current Power Plan:" << QString::fromStdString(currentPowerPlan);

    if (isIdle) {
        if (currentPowerPlan != powerSaverName) {
            qDebug() << "Switching to Power Saver due to idle status.";
            switchPowerPlan(powerSaverName);
            SetExternalMonitorBrightness(30);
        }
    } else {
        // Determine the correct target plan based on the mode
        std::string targetPlan = isTurboMode ? highPerformanceName : balancedName;


        if (currentPowerPlan != targetPlan) {
            qDebug() << "System is active. Switching to" << QString::fromStdString(targetPlan);
            switchPowerPlan(targetPlan);
            SetExternalMonitorBrightness(defaultbrightness);
        }
    }
}

void PowerManager::restorePriorities() {
    DWORD processes[1024], bytesNeeded, processesCount;
    if (!EnumProcesses(processes, sizeof(processes), &bytesNeeded)) {
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

        // Check if the process is set to a higher priority than normal, and adjust it back to normal
        if (currentPriority > NORMAL_PRIORITY_CLASS) {
            if (!SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS)) {
                qDebug() << "Failed to reset priority for process with PID:" << processes[i];
            } else {
                qDebug() << "Priority reset to NORMAL for process with PID:" << processes[i];
            }
        }
        CloseHandle(hProcess);
    }
    qDebug() << "[Restore Priorities] Process priority restoration completed.";
}
