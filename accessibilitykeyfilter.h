#ifndef ACCESSIBILITYKEYFILTER_H
#define ACCESSIBILITYKEYFILTER_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>

class AccessibilityKeyFilter : public QObject {
    Q_OBJECT

public:
    AccessibilityKeyFilter(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            handleKeyPress(keyEvent);
        } else if (event->type() == QEvent::KeyRelease) {
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            handleKeyRelease(keyEvent);
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QMap<int, bool> keyStatus;

    void handleKeyPress(QKeyEvent *event) {
        keyStatus[event->key()] = true;

        if (keyStatus[Qt::Key_Shift] && keyStatus[Qt::Key_Enter] && !keyStatus[Qt::Key_Space]) {
            emit dynamicModeRequested(false);
        } else if (keyStatus[Qt::Key_Shift] && keyStatus[Qt::Key_Space] && keyStatus[Qt::Key_Enter]) {
            emit dynamicModeRequested(true);
        } else if (keyStatus[Qt::Key_Escape] && keyStatus[Qt::Key_Enter]) {
            emit stopDynamicModeRequested();
        }
    }

    void handleKeyRelease(QKeyEvent *event) {
        keyStatus[event->key()] = false;
    }

signals:
    void dynamicModeRequested(bool withPriorityOptimization);
    void stopDynamicModeRequested();
};


#endif // ACCESSIBILITYKEYFILTER_H
