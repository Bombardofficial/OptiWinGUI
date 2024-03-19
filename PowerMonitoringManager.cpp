#include "PowerMonitoringManager.h"
#include <QProcess>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

PowerMonitoringManager::PowerMonitoringManager(QObject *parent) :
    QObject(parent), monitoringTimer(new QTimer(this)), logFile(new QFile(this)) {
    connect(monitoringTimer, &QTimer::timeout, this, &PowerMonitoringManager::collectData);
}

void PowerMonitoringManager::startMonitoring(int durationMinutes) {
    initializeLogFile();
    int durationMilliseconds = durationMinutes * 60 * 1000;
    monitoringTimer->start(1000);
    QTimer::singleShot(durationMilliseconds, this, &PowerMonitoringManager::stopMonitoring);
    emit monitoringStarted();
}

void PowerMonitoringManager::stopMonitoring() {
    if (monitoringTimer->isActive()) {
        monitoringTimer->stop();
        logFile->close();
        emit monitoringStopped();
    }
}

void PowerMonitoringManager::collectData() {
    if (!logFile->isOpen()) return;
    double powerConsumption = fetchPowerConsumption();
    QTextStream out(logFile);
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << ", " << powerConsumption << " Watts\n";
}

double PowerMonitoringManager::fetchPowerConsumption() {
    // Replace with actual logic to fetch power consumption.
    // This could involve running a system command or querying an API.
    // Here is a placeholder for command execution logic.
    QProcess process;
    QString command = "your_command_here"; // The command you'd use to get power data
    process.start(command);
    process.waitForFinished(-1); // Wait indefinitely until finished
    QString output = process.readAllStandardOutput();

    // Example parsing logic, assuming the output is just a number representing the power consumption in watts
    bool ok;
    double powerConsumption = output.trimmed().toDouble(&ok);
    return ok ? powerConsumption : 0.0;
}

void PowerMonitoringManager::initializeLogFile() {
    QString filename = "PowerMonitoring_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv";
    logFile->setFileName(filename);
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(logFile);
        out << "Timestamp, Power Consumption (Watts)\n";
    } else {
        qDebug() << "Failed to open log file for writing.";
    }
}
