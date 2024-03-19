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
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    powerManager(new PowerManager()), powerManagerThread(new QThread(this)), monitoringActive(false), powerMonitoringManager(new PowerMonitoringManager(this)) {
    ui->setupUi(this);
    QPixmap pix(":/OptiWinLogo-transparent.png"); // Load the image
    ui->logomanual->setPixmap(pix); // Set the QPixmap to the label
    ui->logomanual->setScaledContents(true);
    ui->logoautomatic->setPixmap(pix); // Set the QPixmap to the label
    ui->logoautomatic->setScaledContents(true);
    ui->turboModeButton->setCheckable(true);


    connect(ui->startButton, &QPushButton::clicked, this, [this]() {
        int duration = ui->durationComboBox->currentText().toInt(); // Convert duration to integer
        powerMonitoringManager->startMonitoring(duration);
    });

    connect(powerMonitoringManager, &PowerMonitoringManager::logMessage, this, &MainWindow::logMessage);
    connect(powerMonitoringManager, &PowerMonitoringManager::monitoringStarted, this, [this]() {
        ui->statusLabel->setText("Monitoring started...");
        ui->startButton->setEnabled(false); // Optionally disable start button
    });
    connect(powerMonitoringManager, &PowerMonitoringManager::monitoringStopped, this, [this]() {
        ui->statusLabel->setText("Monitoring stopped.");
        ui->startButton->setEnabled(true); // Re-enable start button
    });




    trayIcon = new QSystemTrayIcon(QIcon(":/OptiWinLogo-transparentmain2.ico"), this);
    QMenu *trayMenu = new QMenu(this);
    trayMenu->addAction("Open", this, &MainWindow::showNormal);
    trayMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::DoubleClick) {
            this->showNormal();
        }
    });



    powerManager->moveToThread(powerManagerThread);


    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(10);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(5);
    shadowEffect->setColor(QColor(0, 0, 0, 50));

    ui->powersaverButton->setGraphicsEffect(shadowEffect);
    ui->balancedButton->setGraphicsEffect(shadowEffect);
    ui->highPerformanceButton->setGraphicsEffect(shadowEffect);
    ui->manualButton->setGraphicsEffect(shadowEffect);
    ui->automaticButton->setGraphicsEffect(shadowEffect);
    ui->terminateProcessButton->setGraphicsEffect(shadowEffect);
    ui->listProcessesButton->setGraphicsEffect(shadowEffect);
    ui->turboModeButton->setGraphicsEffect(shadowEffect);
    ui->normalModeButton->setGraphicsEffect(shadowEffect);
    DWORD currentBrightness;
    if (GetExternalMonitorBrightness(currentBrightness)) {
        ui->brightnessSlider->setValue(currentBrightness);
        powerManager->defaultbrightness = currentBrightness;
    }

    connect(powerManager, &PowerManager::powerSourceChangedToAC, this, &MainWindow::powerSourceChangedToAC);
    // Display the current refresh rate
    int currentRefreshRate = displayManager.getCurrentRefreshRate(); // Ensure DisplayManager is properly initialized and accessible
    ui->refreshRateLabel->setText("Current refresh rate is: "+ QString::number(currentRefreshRate) + " Hz"); // Assuming you have a label named refreshRateLabel

    QString currentPowerPlan = powerManager->GetCurrentPowerPlan();
    ui->powerplanLabel->setText("Current power plan: " + currentPowerPlan);

    // Corrected signal-slot connections
    connect(this, &MainWindow::startNormalModeSignal, powerManager, &PowerManager::startNormalModeSlot);
    connect(this, &MainWindow::startTurboModeSignal, powerManager, &PowerManager::startTurboModeSlot);
    connect(this, &MainWindow::stopMonitoringSignal, powerManager, &PowerManager::stopMonitoringSlot);
    connect(ui->brightnessSlider, &QSlider::valueChanged, this, &MainWindow::on_brightnessSlider_valueChanged);
    connect(powerManager, &PowerManager::logMessageAutomatic, this, &MainWindow::logMessageAutomatic);
    connect(ui->brightnessSlider, &QSlider::valueChanged, this, &MainWindow::on_brightnessSlider_valueChanged);

    connect(ui->listProcessesButton, &QPushButton::clicked, this, &MainWindow::on_listProcessesButton_clicked);
    connect(ui->terminateProcessButton, &QPushButton::clicked, this, &MainWindow::on_terminateProcessButton_clicked);

    powerManagerThread->start();


    populateDisplayModes(); // Populate the combo box with display modes
    // Connect the DisplayManager logMessage signal to the MainWindow logMessage slot
    connect(&displayManager, &DisplayManager::logMessage, this, &MainWindow::logMessage);
    connect(ui->normalModeButton, &QPushButton::clicked, this, &MainWindow::on_normalModeButton_clicked);
    connect(ui->turboModeButton, &QPushButton::clicked, this, &MainWindow::on_turboModeButton_clicked);

    connect(powerManager, &PowerManager::monitoringStarted, this, [this](){
        monitoringActive = true;
    });

    connect(powerManager, &PowerManager::monitoringStopped, this, [this](){
        monitoringActive = false;
    });

    prioritycheckbox = new QCheckBox(tr("Process Priority Optimization"), this);
    if (!ui->priorityGroup->layout()) {
        QVBoxLayout *layout = new QVBoxLayout; // Creating a new vertical layout
        ui->priorityGroup->setLayout(layout); // Setting the layout to the widget
    }
    ui->priorityGroup->layout()->addWidget(prioritycheckbox);
    connect(prioritycheckbox, &QCheckBox::toggled, this, &MainWindow::onOptInToggled);

}


MainWindow::~MainWindow() {
    if (powerManager) {
        // Directly invoke cleanup
        powerManager->cleanup();
    }
    powerManagerThread->quit();
    powerManagerThread->wait();
    delete powerManager;
    delete ui;
}

// Check if the current process has administrator privileges
bool hasAdminPrivileges() {
    BOOL fIsRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdministratorsGroup = NULL;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
            &NtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pAdministratorsGroup)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in
    // the primary access token of the process.
    if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin)) {
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
    int currentRefreshRate = displayManager.getCurrentRefreshRate();
    ui->refreshRateLabel->setText("Current refresh rate: " + QString::number(currentRefreshRate) + " Hz");
}

void MainWindow::updatePowerPlanLabel() {
    QString currentPowerPlan = powerManager->GetCurrentPowerPlan();
    ui->powerplanLabel->setText("Current power plan: " + currentPowerPlan);
}

void MainWindow::on_listProcessesButton_clicked() {
    refreshProcessList();
}

void MainWindow::refreshProcessList() {
    ProcessManager processManager;
    auto processes = processManager.listProcesses();
    ui->processListWidget->clear(); // Assuming processListWidget is your QListWidget
    for (const auto& process : processes) {
        ui->processListWidget->addItem(QString::fromStdString(process));
    }
}

void MainWindow::on_terminateProcessButton_clicked() {
    auto selectedItems = ui->processListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        QString processName = selectedItems.first()->text();
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Terminate process", "Are you sure you want to terminate " + processName + "?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            ProcessManager processManager;
            if (processManager.terminateProcess(processName.toStdString())) {
                QMessageBox::information(this, "Process Terminated", processName + " has been terminated.");
            } else {
                QMessageBox::warning(this, "Process Termination Failed", "Failed to terminate " + processName + ".");
            }
            refreshProcessList(); // Refresh the list to reflect the change
        }
    } else {
        QMessageBox::warning(this, "No Process Selected", "Please select a process to terminate.");
    }
}

void MainWindow::on_brightnessSlider_valueChanged(int value) {
    ui->brightnessValueLabel->setText(QString::number(value));
    SetExternalMonitorBrightness(value);
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if(monitoringActive) {
        qDebug() << "monitoringActive: true";
    }
    if(shouldAskBeforeExit()) {
        qDebug() << "shouldAskBeforeExit: true";
    }

    if (monitoringActive && shouldAskBeforeExit()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Do you want to keep the application running in the background?"));
        msgBox.setInformativeText(tr("Your monitoring settings will be active."));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        QCheckBox dontAskAgainBox(tr("Don't ask me again"));
        msgBox.setCheckBox(&dontAskAgainBox);

        int ret = msgBox.exec();
        rememberUserChoice(dontAskAgainBox.isChecked(), ret == QMessageBox::Yes);

        if (ret == QMessageBox::Yes) {
            event->ignore();
            this->hide();
            if (!trayIcon->isVisible()) {
                trayIcon->show();
            }
            return;
        }
    }
    if (powerManager) {
        powerManager->cleanup(); // Directly call cleanup
    }
    QMainWindow::closeEvent(event); // Call the base class implementation
}

void MainWindow::showTrayIcon() {
    if (!trayIcon->isVisible()) {
        trayIcon->show();
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

void MainWindow::updateMonitoringButtonStyle(QPushButton* button, bool monitoringActive) {
    if (monitoringActive) {
        button->setStyleSheet("QPushButton { border: 2px solid green; }");
    } else {
        button->setStyleSheet(""); // Reset to default style
    }
}


void MainWindow::animateButton(QPushButton* button) {
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(button);
    effect->setParent(button);
    button->setGraphicsEffect(effect);

    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(1000); // 1 second
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
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

    bool isChecked = ui->normalModeButton->isChecked();


    if (isChecked && !monitoringActive) {


        // Start monitoring
        if(prioritycheckbox->isChecked()){
            qDebug() << "prioritycheckbox CHECKED";
        }
        bool enableDynamicOptimization = prioritycheckbox->isChecked();

        if (powerManager->isOnBatteryPower() || !enableDynamicOptimization) {
            emit startNormalModeSignal(enableDynamicOptimization);
            logMessageAutomatic("Normal Mode Monitoring started.");
            ui->normalModeButton->setStyleSheet("QPushButton { border:4px solid green; }");
            monitoringActive = true;
            prioritycheckbox->setEnabled(false); // Disable checkbox once monitoring starts.
        } else {
            // Notify the user that monitoring cannot start due to not meeting the conditions.
            QMessageBox::warning(this, "Cannot Start Monitoring",
                                 "Monitoring can only start when plugged in without priority optimization or on battery power.");
        }
    } else if (!isChecked && monitoringActive) {
        prioritycheckbox->setEnabled(true);
        // Stop monitoring
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        ui->normalModeButton->setStyleSheet(""); // Reset to default style
        monitoringActive = false; // Ensure the monitoringActive state is correctly updated
    }

    // No change in monitoring state, just update the visual style based on the button's check state
    else {
        ui->normalModeButton->setStyleSheet(isChecked ? "QPushButton { border: 4px solid green; }" : "");
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



    bool isChecked = ui->turboModeButton->isChecked();

    if (isChecked && !monitoringActive) {
        prioritycheckbox->setEnabled(false);
        // Start monitoring
        if(prioritycheckbox->isChecked()){
            qDebug() << "prioritycheckbox CHECKED";
        }

        emit startTurboModeSignal(prioritycheckbox->isChecked());
        logMessageAutomatic("Turbo Mode Activated.");
        ui->turboModeButton->setStyleSheet("QPushButton { border: 4px solid green; }");
        monitoringActive = true; // Ensure the monitoringActive state is correctly updated
    } else if (!isChecked && monitoringActive) {
        prioritycheckbox->setEnabled(true);
        // Stop monitoring
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        ui->turboModeButton->setStyleSheet(""); // Reset to default style
        monitoringActive = false; // Ensure the monitoringActive state is correctly updated
    }

    // No change in monitoring state, just update the visual style based on the button's check state
    else {
        ui->turboModeButton->setStyleSheet(isChecked ? "QPushButton { border: 4px solid green; }" : "");
    }

    updatePowerPlanLabel(); // Ensure the power plan label is updated accordingly
}

// Define the slot in MainWindow:
void MainWindow::powerSourceChangedToAC() {
    if (!acPowerSourceChangeHandled) {
        QMessageBox::warning(this, tr("Power Source Changed"),
                             tr("Monitoring has been stopped because the power source changed to AC power."));
        emit stopMonitoringSignal(); // Stop monitoring
        acPowerSourceChangeHandled = true; // Ensure this block runs only once per AC connection event
        logMessageAutomatic("Monitoring stopped.");
        ui->normalModeButton->setStyleSheet(""); // Reset to default style
        monitoringActive = false; // Ensure the monitoringActive state is correctly updated
        prioritycheckbox->setEnabled(true);
    }
}

void MainWindow::on_manualButton_clicked() {
    ui->stackedWidget->setCurrentIndex(0); // Index 0 for Manual mode page
    animateButton(ui->manualButton); // Animate manual button
}

void MainWindow::on_automaticButton_clicked() {
    ui->stackedWidget->setCurrentIndex(1); // Index 1 for Automatic mode page
    animateButton(ui->automaticButton); // Animate automatic button
}


void MainWindow::logMessage(const QString &message) {
    ui->consoleLogViewer->append(message); // Append the message to the QTextEdit
}
void MainWindow::logMessageAutomatic(const QString &message) {
    ui->consoleLogViewer_automatic->append(message); // Assuming this is the QTextEdit for automatic mode logs
}
void MainWindow::on_balancedButton_clicked()
{
    powerManager->SetPowerPlan("Balanced");
    animateButton(ui->balancedButton);
    updatePowerPlanLabel();
    QMessageBox::information(this, tr("Success"), tr("Power Plan has been set to Balanced."));

}
void MainWindow::on_powersaverButton_clicked()
{
    powerManager->SetPowerPlan("Power saver");
    animateButton(ui->powersaverButton);
    updatePowerPlanLabel();
    QMessageBox::information(this, tr("Success"), tr("Power Plan has been set to Power Saver."));

}
void MainWindow::on_highPerformanceButton_clicked()
{
    powerManager->SetPowerPlan("High performance");
    animateButton(ui->highPerformanceButton);
    updatePowerPlanLabel();
    QMessageBox::information(this, tr("Success"), tr("Power Plan has been set to High Performance."));

}
void MainWindow::populateDisplayModes() {
    auto modes = displayManager.listSupportedModes();
    for (const auto& mode : modes) {
        QString modeStr = QString::number(mode.first) + "x" + QString::number(mode.second) + " Hz";
        ui->displayModeComboBox->addItem(modeStr, QVariant(mode.second));
    }
}
void MainWindow::on_displayModeComboBox_activated(int index)
{
    bool ok;
    int refreshRate = ui->displayModeComboBox->itemData(index).toInt(&ok);
    if (ok) {
        bool result = displayManager.setDisplayRefreshRate(refreshRate);
        if (result) {
            updateRefreshRateLabel();
            QMessageBox::information(this, tr("Success"), tr("Display mode set successfully."));
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Failed to set display mode."));
        }
    }

}
