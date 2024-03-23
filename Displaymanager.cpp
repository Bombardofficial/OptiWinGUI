#include "DisplayManager.h"

#include <vector>

#include <utility>

#include <algorithm>

DisplayManager::DisplayManager(QObject * parent): QObject(parent) {
    // Initialization code...
}

DEVMODE DisplayManager::getDevMode() {
    DEVMODE devMode;
    ZeroMemory( & devMode, sizeof(devMode));
    devMode.dmSize = sizeof(devMode);

    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, & devMode)) {
        //std::cerr << "Error retrieving display settings" << std::endl;
        emit logMessage("Error retrieving display settings");
    }
    return devMode;
}

bool DisplayManager::setDisplayRefreshRate(int refreshRate) {
    DEVMODE devMode = getDevMode();
    if (devMode.dmDisplayFrequency == static_cast < DWORD > (refreshRate)) {
        emit logMessage("The refresh rate is already set to " + QString::number(refreshRate) + " Hz.");
        return true;
    }

    devMode.dmDisplayFrequency = refreshRate;
    devMode.dmFields = DM_DISPLAYFREQUENCY;

    LONG result = ChangeDisplaySettingsEx(NULL, & devMode, NULL, CDS_UPDATEREGISTRY, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL) {
        emit logMessage("Error changing display settings: " + QString::number(result));
        return false;
    } else {
        emit logMessage(QString("Set display refresh rate to %1 Hz.").arg(refreshRate));
    }
    return true;
}

int DisplayManager::getCurrentRefreshRate() {
    DEVMODE devMode = getDevMode();
    return devMode.dmDisplayFrequency;
}

std::vector < std::pair < int, int >> DisplayManager::listSupportedModes() {
    std::vector < std::pair < int, int >> modes;
    DEVMODE currentDevMode = getDevMode(); // Get current resolution
    int currentWidth = currentDevMode.dmPelsWidth;
    int currentHeight = currentDevMode.dmPelsHeight;

    DEVMODE devMode;
    ZeroMemory( & devMode, sizeof(devMode));
    devMode.dmSize = sizeof(devMode);

    int modeNum = 0;
    while (EnumDisplaySettings(NULL, modeNum++, & devMode)) {
        // Only add modes that match the current resolution
        if (devMode.dmPelsWidth == static_cast < DWORD > (currentWidth) && devMode.dmPelsHeight == static_cast < DWORD > (currentHeight)) {
            modes.push_back(std::make_pair(devMode.dmPelsWidth, devMode.dmDisplayFrequency));
        }
    }

    // Remove duplicate refresh rates
    std::sort(modes.begin(), modes.end(), [](const std::pair < int, int > & a,
                                             const std::pair < int, int > & b) {
        return a.second < b.second;
    });
    modes.erase(std::unique(modes.begin(), modes.end(), [](const std::pair < int, int > & a,
                                                           const std::pair < int, int > & b) {
                    return a.second == b.second;
                }), modes.end());

    return modes;
}
