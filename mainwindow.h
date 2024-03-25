#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "PowerManager.h"
#include "DisplayManager.h"
#include "brightness_control.h"
#include "PowerMonitoringManager.h"
#include "Processmanager.h"
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QSettings>
#include <QCheckBox>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
enum class Mode {
    Normal,
    Turbo,
    Dynamic
};

extern Mode currentMode;
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
    //void updatePowerPlanLabel();
signals:
    void startNormalModeSignal(bool enableDynamicOptimization); // Signal declaration
    void startTurboModeSignal(bool enableDynamicOptimization); // Signal declaration
    void stopMonitoringSignal(); // Signal declaration
    void darkModeChanged(bool isDarkMode);
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
    void onOptInToggled(bool checked);
    void powerSourceChangedToAC();
    void on_dynamicModeButton_clicked();
    void toggleDarkMode(bool checked);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::MainWindow *ui;
    PowerManager* powerManager; // Pointer to PowerManager instance
    QThread* powerManagerThread; // Pointer to the QThread
    DisplayManager displayManager;
    bool monitoringActive;
    QSystemTrayIcon *trayIcon; // System Tray Icon
    void showTrayIcon();
    void rememberUserChoice(bool dontAskAgain, bool runInBackground);
    bool shouldAskBeforeExit();
    QCheckBox *prioritycheckbox;
    PowerMonitoringManager* powerMonitoringManager;
    bool acPowerSourceChangeHandled = false;
    void updateButtonStyles();
    void updatePowerPlanButtons();
    void updatePageButtonStyles();
};
#endif // MAINWINDOW_H
