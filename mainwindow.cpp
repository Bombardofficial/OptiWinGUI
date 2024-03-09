#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QComboBox>
#include <QGraphicsOpacityEffect>
#include <QThread>
#include <QDateTime>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    powerManager(new PowerManager()), powerManagerThread(new QThread(this)), monitoringActive(false) {
    ui->setupUi(this);
    QPixmap pix(":/OptiWinLogo-transparent.png"); // Load the image
    ui->logomanual->setPixmap(pix); // Set the QPixmap to the label
    ui->logomanual->setScaledContents(true);
    ui->logoautomatic->setPixmap(pix); // Set the QPixmap to the label
    ui->logoautomatic->setScaledContents(true);
    ui->turboModeButton->setCheckable(true);
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
    if (powerManager) {
        powerManager->cleanup(); // Directly call cleanup
    }
    QMainWindow::closeEvent(event); // Call the base class implementation
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
    qDebug() << "Turbo Mode Button Clicked. Current state:" << monitoringActive;
    if (ui->normalModeButton->isChecked()) {
        emit startNormalModeSignal();
        logMessageAutomatic("Normal (Battery life) Mode Activated.");
        ui->normalModeButton->setStyleSheet("QPushButton { border: 2px solid green; }");
    } else {
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        ui->normalModeButton->setStyleSheet(""); // Reset to default style
    }
    updatePowerPlanLabel();
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
    if (ui->turboModeButton->isChecked()) {
        emit startTurboModeSignal();
        logMessageAutomatic("Turbo (Performance) Mode Activated.");
        ui->turboModeButton->setStyleSheet("QPushButton { border: 2px solid green; }");
    } else {
        emit stopMonitoringSignal();
        logMessageAutomatic("Monitoring stopped.");
        ui->turboModeButton->setStyleSheet(""); // Reset to default style
    }
    qDebug() << "Turbo Mode Button Processed. New state:" << monitoringActive;
    updatePowerPlanLabel();
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