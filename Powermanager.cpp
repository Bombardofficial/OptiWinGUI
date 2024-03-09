#include "PowerManager.h"
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>


std::mutex consoleMutex;

PowerManager::PowerManager(QObject *parent)
    : QObject(parent), isTurboMode(false), checkTimer(new QTimer(this)), menuDisplayed(false),
    currentPowerPlan(""), idleThreshold(180000), accumulatedIdleTime(0), checkInterval(3000), powerSaverName("Power saver"),highPerformanceName("High performance"), balancedName("Balanced"), monitoringActive(false) {

    connect(checkTimer, &QTimer::timeout, this, &PowerManager::checkIdleAndSwitchPlan);
}
PowerManager::~PowerManager() {
    if (checkTimer) {
        checkTimer->stop();
        delete checkTimer;
        checkTimer = nullptr;
    }
}
void PowerManager::cleanup() {
    // It's safe to stop the timer here because this slot will be called in the correct thread
    if (checkTimer && checkTimer->isActive()) {
        checkTimer->stop();
    }
    // Perform any other necessary cleanup.
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
    SetExternalMonitorBrightness(75);
    qDebug() << "Starting Normal Mode Monitoring";
    if (!monitoringActive) {
        monitoringActive = true;
        checkTimer->start(checkInterval);
        qDebug() << "Normal Mode Monitoring Active";
    }
}

void PowerManager::startTurboModeSlot() {
    isTurboMode = true;
    SetExternalMonitorBrightness(75);
    qDebug() << "Starting Turbo Mode Monitoring";
    if (!monitoringActive) {
        monitoringActive = true;
        checkTimer->start(checkInterval);
        qDebug() << "Turbo Mode Monitoring Active";
    }
}

void PowerManager::stopMonitoringSlot() {
    qDebug() << "Stopping Monitoring";
    if (monitoringActive) {
        checkTimer->stop();
        monitoringActive = false;
        SetExternalMonitorBrightness(defaultbrightness);
        qDebug() << "Monitoring Stopped";
    }
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
            SetExternalMonitorBrightness(75);
        }
    }

}
