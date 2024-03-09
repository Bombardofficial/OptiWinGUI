#pragma once
#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <vector>
#include <thread>
#include "PowerManager.h"
class Application {
public:
    Application();
    void StartApplication();

private:
    void activateNormalMode();
    void activateTurboMode();
    void pauseAndClear();
    void DynamicMenu(PowerManager& powerManager, std::thread& powerManagerThread, bool& monitoringEnabled);
    void startMonitoringMode(PowerManager& powerManager, std::thread& powerManagerThread, bool& monitoringEnabled, void(PowerManager::* monitoringFunction)());
    void StaticMenu(PowerManager& powerManager, std::thread& powerManagerThread, bool& monitoringEnabled);
    // Other private methods as needed
};

#endif // APPLICATION_H
