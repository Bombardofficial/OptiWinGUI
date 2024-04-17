#include "brightness_control.h"

#include <objbase.h>
#include <stdio.h>
#include <comdef.h>
#include <wbemidl.h>
#include <windows.h>
#include <ShellAPI.h>
#include <highlevelmonitorconfigurationapi.h>
#include <QDebug>// provides functions for controlling display brightness at a high level. It allows you to adjust the brightness of monitors connected to the system.


//#pragma comment(lib, "wbemuuid.lib")


// Function to set the brightness for internal display
bool SetInternalMonitorBrightness(DWORD brightness) {

    if (brightness < 10) brightness = 10;
    else if (brightness > 100) brightness = 100;

    // Set the brightness of the laptop display using PowerShell command
    std::wstring command = L"powershell.exe -Command \"(Get-WmiObject -Namespace root\\wmi -Class WmiMonitorBrightnessMethods).WmiSetBrightness(0," + std::to_wstring(brightness) + L")\"";
    HINSTANCE result = ShellExecute(nullptr, L"open", L"powershell.exe", command.c_str(), nullptr, SW_HIDE);

    // Use reinterpret_cast to convert HINSTANCE to intptr_t for comparison without losing precision
    if (reinterpret_cast<intptr_t>(result) <= 32) {
        qDebug() << "Failed to execute PowerShell command. Error code: " << reinterpret_cast<intptr_t>(result);
        return false; // Return false to indicate failure
    }
    return true; // Return true to indicate success
}


// Wrapper function to set brightness for both external and internal displays
bool SetMonitorBrightness(DWORD brightness) {
    bool result = false;
    DWORD currentBrightness;

    // First, try to adjust external monitor brightness
    bool externalResult = SetExternalMonitorBrightness(brightness);

    // If no external monitor is found or adjusted, fall back to the internal monitor
    if (!externalResult || !GetExternalMonitorBrightness(currentBrightness)) {
        result = SetInternalMonitorBrightness(brightness);
    } else {
        result = externalResult;
    }

    return result;
}


bool SetExternalMonitorBrightness(DWORD brightness) {
    BOOL bSuccess = FALSE;
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    std::vector<HMONITOR> hMonitors;

    // Enumerate all display devices
    for (DWORD i = 0; EnumDisplayDevices(NULL, i, &dd, 0); i++) {
        if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE && !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
            DEVMODE dm;
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
                // Obtain the monitor handle using the device name
                POINT pt = { dm.dmPosition.x + 1, dm.dmPosition.y + 1 };
                HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
                if (std::find(hMonitors.begin(), hMonitors.end(), hMonitor) == hMonitors.end()) {
                    hMonitors.push_back(hMonitor);
                }
            }
        }
    }

    // Adjust brightness for each monitor
    for (HMONITOR hMonitor : hMonitors) {
        DWORD cPhysicalMonitors = 0;
        LPPHYSICAL_MONITOR pPhysicalMonitors = nullptr;

        if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &cPhysicalMonitors) && cPhysicalMonitors > 0) {
            pPhysicalMonitors = new PHYSICAL_MONITOR[cPhysicalMonitors];

            if (GetPhysicalMonitorsFromHMONITOR(hMonitor, cPhysicalMonitors, pPhysicalMonitors)) {
                for (DWORD i = 0; i < cPhysicalMonitors; i++) {
                    bSuccess = SetMonitorBrightness(pPhysicalMonitors[i].hPhysicalMonitor, brightness) || bSuccess;
                }
            }

            DestroyPhysicalMonitors(cPhysicalMonitors, pPhysicalMonitors);
            delete[] pPhysicalMonitors;
        }
    }

    return bSuccess != 0;
}
bool GetExternalMonitorBrightness(DWORD & currentBrightness) {
    DWORD cPhysicalMonitors = 0;
    LPPHYSICAL_MONITOR pPhysicalMonitors = nullptr;
    BOOL bSuccess = FALSE;

    // Obtain the monitor handle
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

    // Retrieve the number of physical monitors.
    if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, & cPhysicalMonitors) && cPhysicalMonitors > 0) {
        pPhysicalMonitors = new PHYSICAL_MONITOR[cPhysicalMonitors];

        // Retrieve the physical monitors
        if (GetPhysicalMonitorsFromHMONITOR(hMonitor, cPhysicalMonitors, pPhysicalMonitors)) {
            DWORD minBrightness, maxBrightness;
            // Assuming you're only interested in the first monitor
            bSuccess = GetMonitorBrightness(pPhysicalMonitors[0].hPhysicalMonitor, & minBrightness, & currentBrightness, & maxBrightness);

            // Cleanup
            DestroyPhysicalMonitors(cPhysicalMonitors, pPhysicalMonitors);
        }

        delete[] pPhysicalMonitors;
    }

    return bSuccess != 0;
}

