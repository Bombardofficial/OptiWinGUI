#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QIcon>
#include <QSettings>
/*    */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("./OptiWinLogo-transparentmain2.ico"));
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

)";


    a.setStyleSheet(stylesheet);

    MainWindow w;
    QObject::connect(&w, &MainWindow::darkModeChanged, [&](bool isDarkMode) {
        QString modeSpecificStylesheet;
        if (isDarkMode) {
            // Apply dark theme specifics
            modeSpecificStylesheet = R"(
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
        } else {
            // Apply light theme specifics
            modeSpecificStylesheet = R"(
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
        }
        // Apply the combined stylesheets
        a.setStyleSheet(stylesheet + modeSpecificStylesheet);
    });


    QSettings settings;
    bool isDarkMode = settings.value("isDarkMode", false).toBool();
    // Trigger the style update to the initial preference
    emit w.darkModeChanged(isDarkMode);

    w.show();
    return a.exec();
}

