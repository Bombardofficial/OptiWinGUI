#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QIcon>
#include <QSettings>


QString stylesheet = R"(

QPushButton#dynamicModeButton {
    background-color: white;
    color: red; /* Text color, change it as needed */
    border: 5px solid #eb1313;
    border-radius: 50%; /* This will help to make the button rounded */
    min-width: 100px;
    max-width: 100px;
    min-height: 100px;
    max-height: 100px; /* Adjust width and height equally to get a perfect circle */
    font: bold 18pt; /* Optional: Adjust font as needed */
    padding: 10px 15px;
}
QPushButton#dynamicModeButton:hover {
    background-color: #f2f2f2; /* Lighter grey for hover effect, adjust as needed */
}
QPushButton#dynamicModeButton:pressed {
    background-color: #e6e6e6; /* Even lighter grey for pressed effect, adjust as needed */
}
QLabel#statusLabel {
    font: bold 10pt "Segoe UI";
    text-transform: uppercase;
}
QLabel#statusLabel_4 {
    font: bold 12pt "Segoe UI";
    text-transform: uppercase;
}
QLabel#statusLabel_6 {
    font: bold 10pt "Segoe UI";
    text-transform: uppercase;
}
QLabel#statusLabel_7 {
    font: bold 10pt "Segoe UI";
    text-transform: uppercase;
}
QWidget {

    font: 12pt "Segoe UI";
}

QPushButton#darkModeToggleButton {

    font: bold 10pt; /* Optional: Adjust font as needed */
}

QPushButton {
    border: none;
    border-radius: 5px;
    padding: 10px 5px;
    font: bold 14pt "Segoe UI";
    text-transform: uppercase;
}
QPushButton#turboModeButton, QPushButton#normalModeButton {
    font: bold 10pt "Segoe UI"; /* Smaller font size */
}


QComboBox, QTextEdit, QListWidget {
    border: 2px solid #d32f2f;
    border-radius: 5px;
    padding: 5px;
}


QMenuBar::item {
    padding: 5px 10px;
}

QMenuBar::item:selected {
    background-color: #d32f2f;
    color: #ffffff;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #d32f2f;
}

QMenu::item {
    padding: 5px 30px;
    color: black;
    background-color: #ced9dc;
}

QMenu::item:selected {
    background-color: #d32f2f;
    color: #ffffff;
}

QStatusBar {
    border-top: 1px solid #d32f2f;
}

QSlider::groove:horizontal {
    border: 1px solid #999999;
    height: 8px; /* The height of the slider */
    background: #d3d3d3; /* Lighter grey for slider groove */
    margin: 2px 0;
    border-radius: 4px;
}

QSlider::handle:horizontal {
    background: #d32f2f;
    border: 1px solid #5c5c5c;
    width: 18px;
    margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
    border-radius: 3px;
}

QWidget { background-color: #ced9dc; color: #333; }
QMainWindow {
    background-color: #ced9dc;
    border: 1px solid #d3d3d3; /* Light border for main window */
}
QComboBox, QTextEdit, QListWidget {
    background-color: #ffffff; /* white background for editable widgets */
    color: #333;
}
QMenuBar {
    background-color: #eaefee;
    color: #333;
}
QMenuBar::item {
    background-color: #eaefee;
}
QStatusBar {
    background: #eaefee;
    color: #333;
}
QLabel, QCheckBox { color: black; }
QPushButton {
    color: black;
    background-color: #fff;
    border: 2px solid black;
}
QPushButton:hover {
    background-color: #bc070d;
    color: white;
}

QPushButton:pressed {
    background-color: #520a12;
}

QPushButton:disabled {
    background-color: #5a5a5a;
    color: #aaa;
}

)";

const QString darkModeStyleSheet = R"(
QPushButton#dynamicModeButton {
    background-color: white;
    color: red; /* Text color, change it as needed */
    border: 5px solid #eb1313;
    border-radius: 50%; /* This will help to make the button rounded */
    min-width: 100px;
    max-width: 100px;
    min-height: 100px;
    max-height: 100px; /* Adjust width and height equally to get a perfect circle */
    font: bold 18pt; /* Optional: Adjust font as needed */
    padding: 10px 15px;
}
QPushButton#dynamicModeButton:hover {
    background-color: #f2f2f2; /* Lighter grey for hover effect, adjust as needed */
}
QPushButton#dynamicModeButton:pressed {
    background-color: #e6e6e6; /* Even lighter grey for pressed effect, adjust as needed */
}
QLabel#statusLabel {
    font: bold 10pt "Segoe UI";
    text-transform: uppercase;
}
QLabel#statusLabel_4 {
    font: bold 12pt "Segoe UI";
    text-transform: uppercase;
}
QLabel#statusLabel_6 {
    font: bold 10pt "Segoe UI";
    text-transform: uppercase;
}
QLabel#statusLabel_7 {
    font: bold 10pt "Segoe UI";
    text-transform: uppercase;
}
QWidget {

    font: 12pt "Segoe UI";
}

QPushButton#darkModeToggleButton {

    font: bold 10pt; /* Optional: Adjust font as needed */
}

QPushButton {
    border: none;
    border-radius: 5px;
    padding: 10px 5px;
    font: bold 14pt "Segoe UI";
    text-transform: uppercase;
}
QPushButton#turboModeButton, QPushButton#normalModeButton {
    font: bold 10pt "Segoe UI"; /* Smaller font size */
}


QComboBox, QTextEdit, QListWidget {
    border: 2px solid #d32f2f;
    border-radius: 5px;
    padding: 5px;
}


QMenuBar::item {
    padding: 5px 10px;
}

QMenuBar::item:selected {
    background-color: #d32f2f;
    color: #ffffff;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #d32f2f;
}

QMenu::item {
    padding: 5px 30px;
    color: black;
    background-color: #ced9dc;
}

QMenu::item:selected {
    background-color: #d32f2f;
    color: #ffffff;
}

QStatusBar {
    border-top: 1px solid #d32f2f;
}

QSlider::groove:horizontal {
    border: 1px solid #999999;
    height: 8px; /* The height of the slider */
    background: #d3d3d3; /* Lighter grey for slider groove */
    margin: 2px 0;
    border-radius: 4px;
}

QSlider::handle:horizontal {
    background: #d32f2f;
    border: 1px solid #5c5c5c;
    width: 18px;
    margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
    border-radius: 3px;
}

QWidget { background-color: #181818; color: #e0e0e0; }
QMainWindow {
    background-color: #181818;
    border: 1px solid white; /* Darker border for main window */
}

QComboBox, QTextEdit, QListWidget {
    background-color: #252525; /* darker background for editable widgets */
    color: #e0e0e0;
}
QMenuBar {
    background-color: #181818;
    color: #e0e0e0;
}
QMenuBar::item {
    background-color: #181818;
}
QMenuBar::item:selected {
    background-color: #505050;
}
QStatusBar {
    background: #eaefee;
    color: #e0e0e0;
}
QLabel, QCheckBox { color: #e0e0e0; }
QPushButton {
    color: #fff;
    background-color: #eb1313;

    border: 2px solid black;
}
QPushButton:hover {
    background-color: #fff;
    color: black;
}

QPushButton:pressed {
    background-color: #520a12;
}

QPushButton:disabled {
    background-color: #5a5a5a;
    color: #aaa;
}

)";

const QString colorVisionStylesheets[3] = {
    R"(
// Protanopia Stylesheet

QWidget {
    background-color: #ffe4c4; /* Bisque background for softer contrast */
    color: #000000;
    font: 12pt "Segoe UI";
}

QPushButton {
    background-color: #4682b4; /* Steel blue */
    color: #ffffff;
    border: 5px solid #6495ed; /* Cornflower blue border */
    border-radius: 50%;
    padding: 10px 15px;
    font: bold 18pt "Segoe UI";
    min-width: 100px;
    max-width: 100px;
    min-height: 100px;
    max-height: 100px;
}

QPushButton:hover {
    background-color: #b0c4de; /* Light steel blue for hover */
}

QPushButton:pressed {
    background-color: #add8e6; /* Light blue for pressed */
}

QPushButton#dynamicModeButton, QPushButton#turboModeButton, QPushButton#normalModeButton {
    border: 5px solid #eb1313; /* Maintain red border for visibility */
}

QComboBox, QTextEdit, QListWidget {
    background-color: #ffffff;
    border: 2px solid #6495ed;
    border-radius: 5px;
    padding: 5px;
}

QMenuBar::item, QMenuBar::item:selected {
    background-color: #6495ed;
    color: #ffffff;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #6495ed;
}

QMenu::item {
    background-color: transparent;
    color: #000000;
}

QMenu::item:selected {
    background-color: #6495ed;
    color: #ffffff;
}

QStatusBar {
    border-top: 1px solid #6495ed;
}

QSlider::groove:horizontal {
    background: #b0c4de;
    border: 1px solid #999999;
    height: 8px;
    margin: 2px 0;
    border-radius: 4px;
}

QSlider::handle:horizontal {
    background: #4682b4;
    border: 1px solid #5c5c5c;
    width: 18px;
    border-radius: 3px;
}

    )",
    R"(
// Deuteranopia Stylesheet
QWidget {
    background-color: #fafad2; /* Light Goldenrod Yellow for softer background */
    color: #000000;
    font: 12pt "Segoe UI";
}

QPushButton {
    background-color: #cd5c5c; /* Indian Red */
    color: #ffffff;
    border: 5px solid #f08080; /* Light Coral border */
    border-radius: 50%;
    padding: 10px 15px;
    font: bold 18pt "Segoe UI";
    min-width: 100px;
    max-width: 100px;
    min-height: 100px;
    max-height: 100px;
}

QPushButton:hover {
    background-color: #ffb6c1; /* Light Pink for hover */
}

QPushButton:pressed {
    background-color: #ff69b4; /* Hot Pink for pressed */
}

QComboBox, QTextEdit, QListWidget {
    background-color: #ffffff;
    border: 2px solid #f08080;
    border-radius: 5px;
    padding: 5px;
}

QMenuBar::item, QMenuBar::item:selected {
    background-color: #f08080;
    color: #ffffff;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #f08080;
}

QMenu::item {
    background-color: transparent;
    color: #000000;
}

QMenu::item:selected {
    background-color: #f08080;
    color: #ffffff;
}

QStatusBar {
    border-top: 1px solid #f08080;
}

QSlider::groove:horizontal {
    background: #ffb6c1;
    border: 1px solid #999999;
    height: 8px;
    margin: 2px 0;
    border-radius: 4px;
}

QSlider::handle:horizontal {
    background: #cd5c5c;
    border: 1px solid #5c5c5c;
    width: 18px;
    border-radius: 3px;
}

    )",
    R"(
// Tritanopia Stylesheet
QWidget {
    background-color: #e0ffff; /* Light Cyan for a very soft background */
    color: #000000;
    font: 12pt "Segoe UI";
}

QPushButton {
    background-color: #20b2aa; /* Light Sea Green */
    color: #ffffff;
    border: 5px solid #3cb371; /* Medium Sea Green border */
    border-radius: 50%;
    padding: 10px 15px;
    font: bold 18pt "Segoe UI";
    min-width: 100px;
    max-width: 100px;
    min-height: 100px;
    max-height: 100px;
}

QPushButton:hover {
    background-color: #66cdaa; /* Medium Aquamarine for hover */
}

QPushButton:pressed {
    background-color: #8fbc8f; /* Dark Sea Green for pressed */
}

QComboBox, QTextEdit, QListWidget {
    background-color: #ffffff;
    border: 2px solid #3cb371;
    border-radius: 5px;
    padding: 5px;
}

QMenuBar::item, QMenuBar::item:selected {
    background-color: #3cb371;
    color: #ffffff;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #3cb371;
}

QMenu::item {
    background-color: transparent;
    color: #000000;
}

QMenu::item:selected {
    background-color: #3cb371;
    color: #ffffff;
}

QStatusBar {
    border-top: 1px solid #3cb371;
}

QSlider::groove:horizontal {
    background: #66cdaa;
    border: 1px solid #999999;
    height: 8px;
    margin: 2px 0;
    border-radius: 4px;
}

QSlider::handle:horizontal {
    background: #20b2aa;
    border: 1px solid #5c5c5c;
    width: 18px;
    border-radius: 3px;
}

    )"
};
void updateStylesheet(QApplication& app, int colorVisionMode, bool isDarkMode) {
    QString finalStyleSheet = stylesheet;  // Base stylesheet
    qDebug() << "Base stylesheet loaded.";

    if (isDarkMode) {
        finalStyleSheet += darkModeStyleSheet;
        qDebug() << "Dark mode stylesheet appended.";
    }

    if (colorVisionMode >= 0 && colorVisionMode < 3) {
        finalStyleSheet = colorVisionStylesheets[colorVisionMode];
        qDebug() << "Color vision stylesheet set for mode:" << colorVisionMode;
    }

    qDebug() << "Final stylesheet being applied:" << finalStyleSheet;
    app.setStyleSheet(finalStyleSheet);
    qDebug() << "Stylesheet applied successfully.";
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("./OptiWinLogo-transparentmain2.ico"));

    MainWindow w;
    QSettings settings;
    bool isDarkMode = settings.value("isDarkMode", false).toBool();
    int colorVisionMode = settings.value("colorVisionMode", 0).toInt();
    updateStylesheet(a, colorVisionMode, isDarkMode);

    // Connect signals to the stylesheet update function
    QObject::connect(&w, &MainWindow::darkModeChanged, &w, [&](bool dark) {
        updateStylesheet(a, settings.value("colorVisionMode", 0).toInt(), dark);
    });

    QObject::connect(&w, &MainWindow::colorVisionChanged,&w, [&](int mode) {
        updateStylesheet(a, mode, settings.value("isDarkMode", false).toBool());
    });

    w.show();
    return a.exec();
}

