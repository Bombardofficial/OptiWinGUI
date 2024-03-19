#ifndef POWERMONITORINGMANAGER_H
#define POWERMONITORINGMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

class PowerMonitoringManager : public QObject {
    Q_OBJECT

public:
    explicit PowerMonitoringManager(QObject *parent = nullptr);
    void startMonitoring(int durationMinutes);
    void stopMonitoring();

signals:
    void monitoringStarted();
    void monitoringStopped();
    void logMessage(const QString &message);

private slots:
    void collectData();

private:
    QTimer *monitoringTimer;
    QFile *logFile;
    void initializeLogFile();
    double fetchPowerConsumption();
};

#endif // POWERMONITORINGMANAGER_H
