#include "mainwindow.h"

#include "ui_mainwindow.h"

#include <QMessageBox>

#include <QComboBox>

#include <QGraphicsOpacityEffect>

#include <QThread>

#include <QDateTime>

#include <QPixmap>

#include <QCheckBox>

#include <QGraphicsDropShadowEffect>

#include <QDebug>

Mode currentMode = Mode::Dynamic;
MainWindow::MainWindow(QWidget * parent): QMainWindow(parent), ui(new Ui::MainWindow),
    powerManager(new PowerManager()), powerManagerThread(new QThread(this)), monitoringActive(false), powerMonitoringManager(new PowerMonitoringManager(this)) {
    ui -> setupUi(this);
    QPixmap pix(":/OptiWinLogo-transparent.png"); // Load the image
    ui -> logomanual -> setPixmap(pix); // Set the QPixmap to the label
    ui -> logomanual -> setScaledContents(true);
    ui -> logoautomatic -> setPixmap(pix); // Set the QPixmap to the label
    ui -> logoautomatic -> setScaledContents(true);
    ui -> turboModeButton -> setCheckable(true);
    updatePowerPlanButtons();
    connect(ui -> dynamicModeButton, & QPushButton::clicked, this, & MainWindow::on_dynamicModeButton_clicked);

    trayIcon = new QSystemTrayIcon(QIcon(":/OptiWinLogo-transparentmain2.ico"), this);
    QMenu * trayMenu = new QMenu(this);
    trayMenu -> addAction("Open", this, & MainWindow::showNormal);
    trayMenu -> addAction("Quit", qApp, & QCoreApplication::quit);
    trayIcon -> setContextMenu(trayMenu);
    connect(trayIcon, & QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            this -> showNormal();
        }
    });

    powerManager -> moveToThread(powerManagerThread);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(30); // Increased blur radius for a more blurred effect
    shadowEffect->setXOffset(5); // Adjusted to make the shadow appear wider
    shadowEffect->setYOffset(10); // Adjusted to make the shadow appear wider
    shadowEffect->setColor(QColor(0, 0, 0, 150)); // Increased opacity for a deeper shadow
    ui->dynamicModeButton->setGraphicsEffect(shadowEffect);

    DWORD currentBrightness;
    if (GetExternalMonitorBrightness(currentBrightness)) {
        ui -> brightnessSlider -> setValue(currentBrightness);
        powerManager -> defaultbrightness = currentBrightness;
    }

    connect(powerManager, & PowerManager::powerSourceChangedToAC, this, & MainWindow::powerSourceChangedToAC);

    QString currentPowerPlan = powerManager -> GetCurrentPowerPlan();
    ui -> powerplanLabel -> setText("Current power plan: " + currentPowerPlan);
    updateRefreshRateLabel();
    // Corrected signal-slot connections
    connect(this, & MainWindow::startNormalModeSignal, powerManager, & PowerManager::startNormalModeSlot);
    connect(this, & MainWindow::startTurboModeSignal, powerManager, & PowerManager::startTurboModeSlot);
    connect(this, & MainWindow::stopMonitoringSignal, powerManager, & PowerManager::stopMonitoringSlot);
    connect(ui -> brightnessSlider, & QSlider::valueChanged, this, & MainWindow::on_brightnessSlider_valueChanged);
    connect(powerManager, & PowerManager::logMessageAutomatic, this, & MainWindow::logMessageAutomatic);
    connect(ui -> brightnessSlider, & QSlider::valueChanged, this, & MainWindow::on_brightnessSlider_valueChanged);

    connect(ui -> listProcessesButton, & QPushButton::clicked, this, & MainWindow::on_listProcessesButton_clicked);
    connect(ui -> terminateProcessButton, & QPushButton::clicked, this, & MainWindow::on_terminateProcessButton_clicked);

    powerManagerThread -> start();

    populateDisplayModes(); // Populate the combo box with display modes
    // Connect the DisplayManager logMessage signal to the MainWindow logMessage slot
    connect( & displayManager, & DisplayManager::logMessage, this, & MainWindow::logMessage);
    connect(ui -> normalModeButton, & QPushButton::clicked, this, & MainWindow::on_normalModeButton_clicked);
    connect(ui -> turboModeButton, & QPushButton::clicked, this, & MainWindow::on_turboModeButton_clicked);

    connect(powerManager, & PowerManager::monitoringStarted, this, [this]() {
        monitoringActive = true;
    });

    connect(powerManager, & PowerManager::monitoringStopped, this, [this]() {
        monitoringActive = false;
    });

    prioritycheckbox = new QCheckBox(tr("Process Priority Optimization"), this);
    if (!ui -> priorityGroup -> layout()) {
        QVBoxLayout * layout = new QVBoxLayout; // Creating a new vertical layout
        ui -> priorityGroup -> setLayout(layout); // Setting the layout to the widget
    }
    ui -> priorityGroup -> layout() -> addWidget(prioritycheckbox);
    connect(prioritycheckbox, & QCheckBox::toggled, this, & MainWindow::onOptInToggled);

}

MainWindow::~MainWindow() {
    if (powerManager) {
        // Directly invoke cleanup
        powerManager -> cleanup();
    }
    powerManagerThread -> quit();
    powerManagerThread -> wait();
    delete powerManager;
    delete ui;
}

void MainWindow::updatePowerPlanButtons() {
    QString currentPowerPlan = powerManager->GetCurrentPowerPlan();
    qDebug() << "updatePowerPlanButtons is called, currentPowerPlan:" << currentPowerPlan;
    // Reset styles for all buttons
    ui->balancedButton->setStyleSheet("");
    ui->powersaverButton->setStyleSheet("");
    ui->highPerformanceButton->setStyleSheet("");

    // Highlight the active plan
    if (currentPowerPlan.contains("Balanced", Qt::CaseInsensitive) || currentPowerPlan.contains("Kiegyens", Qt::CaseInsensitive)) {
        ui->balancedButton->setStyleSheet("border: 4px solid green;");
        qDebug() << "Balanced button";
    } else if (currentPowerPlan.contains("Power saver", Qt::CaseInsensitive) || currentPowerPlan.contains("Energiatakar", Qt::CaseInsensitive)) {
        ui->powersaverButton->setStyleSheet("border: 4px solid green;");
        qDebug() << "Power saver button";
    } else if (currentPowerPlan.contains("High performance", Qt::CaseInsensitive) || currentPowerPlan.contains("nycentrikus", Qt::CaseInsensitive)) {
        ui->highPerformanceButton->setStyleSheet("border: 4px solid green;");
        qDebug() << "High performance button";
    }
}


// Check if the current process has administrator privileges
bool hasAdminPrivileges() {
    BOOL fIsRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdministratorsGroup = NULL;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid( &
                                  NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &
                                  pAdministratorsGroup)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in
    // the primary access token of the process.
    if (!CheckTokenMembership(NULL, pAdministratorsGroup, & fIsRunAsAdmin)) {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (pAdministratorsGroup) {
        FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = NULL;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError) {
        throw dwError;
    }

    return fIsRunAsAdmin == TRUE;
}

void MainWindow::onOptInToggled(bool checked) {
    if (checked) {
        try {
            /*if (!hasAdminPrivileges()) {
          QMessageBox::critical(this, "Administrator Privileges Required",
                                "For this function, you need to start OptiWin as an administrator.");
          // Uncheck the box since we can't proceed without admin rights
          prioritycheckbox->setChecked(false);
          return;
      }*/
            QMessageBox::information(this, "Dynamic Priority Optimization Enabled",
                                     "Dynamic piority optimization is now enabled.");

        } catch (DWORD dwError) {
            QMessageBox::critical(this, "Error",
                                  QString("An error occurred while checking administrator privileges: %1").arg(dwError));
        }
    }
}

void MainWindow::updateRefreshRateLabel() {
    // Get the current refresh rate from the display manager
    int currentRefreshRate = displayManager.getCurrentRefreshRate();

    // Update the combo box to the current refresh rate
    int count = ui->displayModeComboBox->count();
    for (int i = 0; i < count; ++i) {
        if (ui->displayModeComboBox->itemData(i).toInt() == currentRefreshRate) {
            ui->displayModeComboBox->setCurrentIndex(i);
            break;
        }
    }
}

void MainWindow::updatePowerPlanLabel() {
    QString currentPowerPlan = powerManager -> GetCurrentPowerPlan();
    ui -> powerplanLabel -> setText("Current power plan: " + currentPowerPlan);
}

void MainWindow::on_listProcessesButton_clicked() {
    refreshProcessList();
}

void MainWindow::refreshProcessList() {
    ProcessManager processManager;
    auto processes = processManager.listProcesses();
    ui -> processListWidget -> clear(); // Assuming processListWidget is your QListWidget
    for (const auto & process: processes) {
        ui -> processListWidget -> addItem(QString::fromStdString(process));
    }
}

void MainWindow::on_terminateProcessButton_clicked() {

    auto selectedItems = ui->processListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        QString processName = selectedItems.first()->text();
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Terminate process", "Are you sure you want to terminate " + processName + "?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            ProcessManager processManager;
            if (processManager.terminateProcess(processName.toStdString())) {
                QMessageBox::information(this, "Process Terminated", processName + " has been terminated.");
                refreshProcessList(); // Refresh the list to reflect the change
                ui->processListWidget->clearSelection(); // Clear the selection
            } else {
                QMessageBox::warning(this, "Process Termination Failed", "Failed to terminate " + processName + ".");
            }
        }
    } else {
        QMessageBox::warning(this, "No Process Selected", "Please select a process to terminate.");
    }
}

void MainWindow::on_brightnessSlider_valueChanged(int value) {
    ui -> brightnessValueLabel -> setText(QString::number(value));
    SetExternalMonitorBrightness(value);
}
void MainWindow::closeEvent(QCloseEvent * event) {
    if (monitoringActive) {
        qDebug() << "monitoringActive: true";
    }
    if (shouldAskBeforeExit()) {
        qDebug() << "shouldAskBeforeExit: true";
    }

    if (monitoringActive && shouldAskBeforeExit()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Do you want to keep the application running in the background?"));
        msgBox.setInformativeText(tr("Your monitoring settings will be active."));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        QCheckBox dontAskAgainBox(tr("Don't ask me again"));
        msgBox.setCheckBox( & dontAskAgainBox);

        int ret = msgBox.exec();
        rememberUserChoice(dontAskAgainBox.isChecked(), ret == QMessageBox::Yes);

        if (ret == QMessageBox::Yes) {
            event -> ignore();
            this -> hide();
            if (!trayIcon -> isVisible()) {
                trayIcon -> show();
            }
            return;
        }
    }
    if (powerManager) {
        powerManager -> cleanup(); // Directly call cleanup
    }
    QMainWindow::closeEvent(event); // Call the base class implementation
}

void MainWindow::showTrayIcon() {
    if (!trayIcon -> isVisible()) {
        trayIcon -> show();
    }
}

void MainWindow::rememberUserChoice(bool dontAskAgain, bool runInBackground) {
    QSettings settings;
    settings.setValue("dontAskBeforeExit", dontAskAgain);
    settings.setValue("runInBackground", runInBackground);
}

bool MainWindow::shouldAskBeforeExit() {
    QSettings settings;
    return !settings.value("dontAskBeforeExit", false).toBool();
}

void MainWindow::updateMonitoringButtonStyle(QPushButton * button, bool monitoringActive) {
    if (monitoringActive) {
        button -> setStyleSheet("QPushButton { border: 2px solid green; }");
    } else {
        button -> setStyleSheet(""); // Reset to default style
    }
}

void MainWindow::animateButton(QPushButton * button) {
    QGraphicsOpacityEffect * effect = new QGraphicsOpacityEffect(button);
    effect -> setParent(button);
    button -> setGraphicsEffect(effect);

    QPropertyAnimation * animation = new QPropertyAnimation(effect, "opacity");
    animation -> setDuration(1000); // 1 second
    animation -> setStartValue(0.0);
    animation -> setEndValue(1.0);
    animation -> setEasingCurve(QEasingCurve::InOutQuad);
    animation -> start(QPropertyAnimation::DeleteWhenStopped);
}

void MainWindow::on_normalModeButton_clicked() {
    static QDateTime lastClickTime;
    QDateTime now = QDateTime::currentDateTime();

    if (lastClickTime.isValid() && lastClickTime.msecsTo(now) < 500) { // 500 ms threshold
        qDebug() << "Ignoring rapid second click.";
        return;
    }

    lastClickTime = now;
    qDebug() << "Normal Mode Button Clicked. Current state:" << monitoringActive;
    currentMode = Mode::Normal;
    bool isChecked = ui -> normalModeButton -> isChecked();

    if (isChecked && !monitoringActive) {

        // Start monitoring
        if (prioritycheckbox -> isChecked()) {
            qDebug() << "prioritycheckbox CHECKED";
        }
        bool enableDynamicOptimization = prioritycheckbox -> isChecked();

        if (powerManager -> isOnBatteryPower() || !enableDynamicOptimization) {
            emit startNormalModeSignal(enableDynamicOptimization);
            logMessageAutomatic("Normal Mode Monitoring started.");
            ui -> normalModeButton -> setStyleSheet("QPushButton { border:4px solid green; }");
            updatePowerPlanButtons();
            monitoringActive = true;
            prioritycheckbox -> setEnabled(false); // Disable checkbox once monitoring starts.
        } else {
            // Notify the user that monitoring cannot start due to not meeting the conditions.
            QMessageBox::warning(this, "Cannot Start Monitoring",
                                 "Monitoring can only start when plugged in without priority optimization or on battery power.");
        }
    } else if (!isChecked && monitoringActive) {
        prioritycheckbox -> setEnabled(true);
        // Stop monitoring
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        ui -> normalModeButton -> setStyleSheet(""); // Reset to default style
        monitoringActive = false; // Ensure the monitoringActive state is correctly updated
    }

    // No change in monitoring state, just update the visual style based on the button's check state
    else {
        ui -> normalModeButton -> setStyleSheet(isChecked ? "QPushButton { border: 4px solid green; }" : "");
    }

    updatePowerPlanLabel(); // Ensure the power plan label is updated accordingly
}

void MainWindow::on_turboModeButton_clicked() {
    static QDateTime lastClickTime;
    QDateTime now = QDateTime::currentDateTime();

    if (lastClickTime.isValid() && lastClickTime.msecsTo(now) < 500) { // 500 ms threshold
        qDebug() << "Ignoring rapid second click.";
        return;
    }

    lastClickTime = now;
    qDebug() << "Turbo Mode Button Clicked. Current state:" << monitoringActive;

    currentMode = Mode::Turbo;

    bool isChecked = ui -> turboModeButton -> isChecked();

    if (isChecked && !monitoringActive) {
        prioritycheckbox -> setEnabled(false);
        // Start monitoring
        if (prioritycheckbox -> isChecked()) {
            qDebug() << "prioritycheckbox CHECKED";
        }

        emit startTurboModeSignal(prioritycheckbox -> isChecked());
        logMessageAutomatic("Turbo Mode Activated.");
        ui -> turboModeButton -> setStyleSheet("QPushButton { border: 4px solid green; }");
        updatePowerPlanButtons();
        monitoringActive = true; // Ensure the monitoringActive state is correctly updated
    } else if (!isChecked && monitoringActive) {
        prioritycheckbox -> setEnabled(true);
        // Stop monitoring
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        ui -> turboModeButton -> setStyleSheet(""); // Reset to default style
        monitoringActive = false; // Ensure the monitoringActive state is correctly updated
    }

    // No change in monitoring state, just update the visual style based on the button's check state
    else {
        ui -> turboModeButton -> setStyleSheet(isChecked ? "QPushButton { border: 4px solid green; }" : "");
    }

    updatePowerPlanLabel(); // Ensure the power plan label is updated accordingly
}

void MainWindow::on_dynamicModeButton_clicked() {
    static QDateTime lastClickTime;
    QDateTime now = QDateTime::currentDateTime();

    if (lastClickTime.isValid() && lastClickTime.msecsTo(now) < 500) { // 500 ms threshold
        qDebug() << "Ignoring rapid second click.";
        return;
    }

    lastClickTime = now;
    currentMode = Mode::Dynamic;
    bool isChecked = ui -> dynamicModeButton -> isChecked();
    if (isChecked && !monitoringActive) {
        prioritycheckbox -> setEnabled(false);
        ui -> normalModeButton -> setEnabled(false); // Disable normal mode button
        ui -> turboModeButton -> setEnabled(false);
        // Start monitoring
        if (prioritycheckbox -> isChecked()) {
            qDebug() << "prioritycheckbox CHECKED";
        }
        // Check the current power source and decide the mode
        if (powerManager -> isOnBatteryPower()) {
            emit startNormalModeSignal(prioritycheckbox -> isChecked());
        } else {
            emit startTurboModeSignal(prioritycheckbox -> isChecked());
        }
        monitoringActive = true;
        logMessageAutomatic("Dynamic Mode Activated.");
    } else if (!isChecked && monitoringActive) {
        prioritycheckbox -> setEnabled(true);
        ui -> normalModeButton -> setEnabled(true); // Re-enable normal mode button
        ui -> turboModeButton -> setEnabled(true);
        // Stop monitoring
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        monitoringActive = false; // Ensure the monitoringActive state is correctly updated
    }
    updatePowerPlanButtons();
}
void MainWindow::updateButtonStyles() {
    // Reset all buttons to default style
    ui -> normalModeButton -> setStyleSheet("");
    ui -> turboModeButton -> setStyleSheet("");
    // Then highlight the current mode
    switch (currentMode) {
    case Mode::Normal:
        ui -> normalModeButton -> setStyleSheet("QPushButton { border: 4px solid green; }");
        break;
    case Mode::Turbo:
        ui -> turboModeButton -> setStyleSheet("QPushButton { border: 4px solid green; }");
        break;
    case Mode::Dynamic:
        // Optionally style the dynamic mode button
        break;
    }
}
// Define the slot in MainWindow:
void MainWindow::powerSourceChangedToAC() {
    // This will handle the change in power source
    if (currentMode == Mode::Dynamic) {
        if (powerManager -> isOnBatteryPower()) {
            qDebug() << "Switched to battery, switching to Normal Mode";
            emit startNormalModeSignal(prioritycheckbox -> isChecked());
        } else {
            qDebug() << "Switched to AC, switching to Turbo Mode";
            emit startTurboModeSignal(prioritycheckbox -> isChecked());
        }
    } else if (!acPowerSourceChangeHandled) {
        QMessageBox::warning(this, tr("Power Source Changed"), tr("Monitoring has been stopped because the power source changed."));
        emit stopMonitoringSignal();
        acPowerSourceChangeHandled = true;
        updateButtonStyles();
        monitoringActive = false;
        prioritycheckbox -> setEnabled(true);
        // Re-evaluate the power mode after stopping the monitoring
        powerManager -> stopMonitoringAndRestart();
    }
}

void MainWindow::on_manualButton_clicked() {
    ui -> stackedWidget -> setCurrentIndex(0); // Index 0 for Manual mode page
    animateButton(ui -> manualButton); // Animate manual button
}

void MainWindow::on_automaticButton_clicked() {
    ui -> stackedWidget -> setCurrentIndex(1); // Index 1 for Automatic mode page
    animateButton(ui -> automaticButton); // Animate automatic button
}

void MainWindow::logMessage(const QString & message) {
    ui -> consoleLogViewer -> append(message); // Append the message to the QTextEdit
}
void MainWindow::logMessageAutomatic(const QString & message) {
    ui -> consoleLogViewer_automatic -> append(message); // Assuming this is the QTextEdit for automatic mode logs
}
void MainWindow::on_balancedButton_clicked() {
    powerManager -> SetPowerPlan("Balanced");
    animateButton(ui -> balancedButton);
    updatePowerPlanButtons();
    updatePowerPlanLabel();
    QMessageBox::information(this, tr("Success"), tr("Power Plan has been set to Balanced."));

}
void MainWindow::on_powersaverButton_clicked() {
    powerManager -> SetPowerPlan("Power saver");
    animateButton(ui -> powersaverButton);
    updatePowerPlanButtons();
    updatePowerPlanLabel();
    QMessageBox::information(this, tr("Success"), tr("Power Plan has been set to Power Saver."));

}
void MainWindow::on_highPerformanceButton_clicked() {
    powerManager -> SetPowerPlan("High performance");
    animateButton(ui -> highPerformanceButton);
    updatePowerPlanButtons();
    updatePowerPlanLabel();
    QMessageBox::information(this, tr("Success"), tr("Power Plan has been set to High Performance."));

}
void MainWindow::populateDisplayModes() {
    auto modes = displayManager.listSupportedModes();
    int currentRefreshRate = displayManager.getCurrentRefreshRate();
    int currentIndex = -1;

    ui->displayModeComboBox->clear(); // Clear existing items first

    int index = 0;
    for (const auto& mode : modes) {
        QString modeStr = QString("%1x%2 Hz").arg(mode.first).arg(mode.second);
        ui->displayModeComboBox->addItem(modeStr, QVariant(mode.second));

        // Check if this mode's refresh rate matches the current refresh rate
        if (mode.second == currentRefreshRate) {
            currentIndex = index;
        }
        ++index;
    }

    // Set the current index to the active refresh rate
    if (currentIndex != -1) {
        ui->displayModeComboBox->setCurrentIndex(currentIndex);
    }
}

void MainWindow::on_displayModeComboBox_activated(int index) {
    bool ok;
    int refreshRate = ui->displayModeComboBox->itemData(index).toInt(&ok);

    if (ok) {
        bool result = displayManager.setDisplayRefreshRate(refreshRate);
        if (result) {
            updateRefreshRateLabel(); // Assuming this function updates the label correctly
            QMessageBox::information(this, tr("Success"), tr("Display mode set successfully."));
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to set display mode."));
        }
    }
}


