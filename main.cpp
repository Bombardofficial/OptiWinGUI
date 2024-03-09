#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QIcon>
/*    */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("./OptiWinLogo-transparentmain2.ico"));
    QString stylesheet = R"(
QWidget {
    background-color: #ced9dc;
    color: #333;
    font: 12pt "Segoe UI";
}

QMainWindow {
    background-color: #ced9dc;
    border: 1px solid #d3d3d3; /* Light border for main window */
}

QPushButton {
    color: #fff;
    background-color: #eb1313;
    border: none;
    border-radius: 5px;
    padding: 10px 25px;
    font: bold 14pt "Segoe UI";
    text-transform: uppercase;
    /* Button Shadow */
    box-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1); /* subtle shadow for buttons */
}

QPushButton:hover {
    background-color: #bc070d;
}

QPushButton:pressed {
    background-color: #520a12;
}

QPushButton:disabled {
    background-color: #5a5a5a;
    color: #aaa;
}

QComboBox, QTextEdit, QListWidget {
    border: 2px solid #d32f2f;
    border-radius: 5px;
    background-color: #ffffff; /* white background for editable widgets */
    color: #333;
    padding: 5px;
    /* Widget Shadow */
    box-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1);
}

QMenuBar {
    background-color: #eaefee;
    color: #333;
}

QMenuBar::item {
    background-color: #eaefee;
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
    color: #333;
}

QMenu::item:selected {
    background-color: #d32f2f;
    color: #ffffff;
}

QStatusBar {
    border-top: 1px solid #d32f2f;
    background: #eaefee;
    color: #333;
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

/* Apply the shadow effect to all items that should have it */
QLabel, QFrame, QTabWidget, QGroupBox {
    box-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1);
}

/* If you're using a QGraphicsDropShadowEffect, apply it to the necessary widgets in your mainwindow setup code */
)";


    a.setStyleSheet(stylesheet);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "OptiWinUI_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}

/*    QString stylesheet = R"(
QWidget {
    background-color: #121212;
    color: #eeeeee;
    font: 14pt "Segoe UI";
    border-radius: 5px;
    box-shadow: 0px 8px 16px 0px rgba(0,0,0,0.2);
}

QMainWindow {
    background-color: #1e1e1e;

}

QPushButton {
    color: #eeeeee;
    background-color: #d32f2f;
    border: none;
    border-radius: 5px;
    padding: 10px 25px;
    font: bold 14pt "Segoe UI";
    text-transform: uppercase;
}

QPushButton:hover {
    background-color: #9a0007;
}

QPushButton:pressed {
    background-color: #680000;
}

QPushButton:disabled {
    background-color: #5a5a5a;
}

QComboBox {
    border: 2px solid #d32f2f;
    border-radius: 5px;
    padding: 5px;
    background: #1e1e1e;
}

QTextEdit {
    background-color: #1e1e1e;
    border: 2px solid #d32f2f;
    border-radius: 5px;
    padding: 5px;
    color: #eeeeee;
}

QMenuBar {
    background-color: #121212;
    color: #eeeeee;
}

QMenuBar::item {
    background-color: #121212;
    padding: 5px 10px;
}

QMenuBar::item:selected {
    background-color: #d32f2f;
}

QMenu {
    background-color: #1e1e1e;
    border: 1px solid #d32f2f;
}

QMenu::item {
    padding: 5px 30px;
}

QMenu::item:selected {
    background-color: #d32f2f;
}

QStatusBar {
    border-top: 1px solid #d32f2f;
    background: #121212;
    color: #eeeeee;
}
QSlider::groove:horizontal {
    border: 1px solid #999999;
    height: 8px;
    background: #3a3a3a;
    margin: 2px 0;
    border-radius: 4px;
}

QSlider::handle:horizontal {
    background: #d32f2f;
    border: 1px solid #5c5c5c;
    width: 18px;
    margin: -2px 0;
    border-radius: 3px;
}

QListWidget {
    border: 2px solid #d32f2f;
    border-radius: 5px;
    background-color: #1e1e1e;
}

QListWidget::item {
    color: #eeeeee;
    padding: 5px;
}

QListWidget::item:selected {
    background-color: #d32f2f;
    color: #eeeeee;
}
)";*/
