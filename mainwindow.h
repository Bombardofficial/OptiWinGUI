#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "PowerManager.h"
#include "DisplayManager.h"
#include "brightness_control.h"
#include "Processmanager.h"
#include <QPropertyAnimation>
#include <QPushButton>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void populateDisplayModes();
    void logMessage(const QString &message);
    void logMessageAutomatic(const QString &message);
    void animateButton(QPushButton* button);
    void updateMonitoringButtonStyle(QPushButton* button, bool monitoringActive);
    void updateRefreshRateLabel();
    void updatePowerPlanLabel();
signals:
    void startNormalModeSignal(); // Signal declaration
    void startTurboModeSignal(); // Signal declaration
    void stopMonitoringSignal(); // Signal declaration
private slots:
    void on_highPerformanceButton_clicked(); // Slot for button click
    void on_balancedButton_clicked(); // Slot for button click
    void on_powersaverButton_clicked(); // Slot for button click
    void on_displayModeComboBox_activated(int index);
    void on_manualButton_clicked();
    void on_automaticButton_clicked();
    void on_normalModeButton_clicked();
    void on_turboModeButton_clicked();
    void on_brightnessSlider_valueChanged(int value);
    void refreshProcessList();
    void on_terminateProcessButton_clicked();
    void on_listProcessesButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::MainWindow *ui;
    PowerManager* powerManager; // Pointer to PowerManager instance
    QThread* powerManagerThread; // Pointer to the QThread
    DisplayManager displayManager;
    bool monitoringActive;
};
#endif // MAINWINDOW_H