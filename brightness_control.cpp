#include "brightness_control.h"

#include <comdef.h> // for _com_error

#include <Wbemidl.h>

//#pragma comment(lib, "wbemuuid.lib")

// Helper function to initialize COM and set security

bool SetExternalMonitorBrightness(DWORD brightness) {
    DWORD cPhysicalMonitors = 0;
    LPPHYSICAL_MONITOR pPhysicalMonitors = nullptr;
    BOOL bSuccess = FALSE;

    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

    if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, & cPhysicalMonitors)) {
        pPhysicalMonitors = new PHYSICAL_MONITOR[cPhysicalMonitors];

        if (GetPhysicalMonitorsFromHMONITOR(hMonitor, cPhysicalMonitors, pPhysicalMonitors)) {
            for (DWORD i = 0; i < cPhysicalMonitors; i++) {
                bSuccess = SetMonitorBrightness(pPhysicalMonitors[i].hPhysicalMonitor, brightness);
            }
        }

        DestroyPhysicalMonitors(cPhysicalMonitors, pPhysicalMonitors);
        delete[] pPhysicalMonitors;
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

/*
bool SetAppropriateMonitorBrightness(DWORD brightness) {
    MONITORINFOEX mi;
    mi.cbSize = sizeof(MONITORINFOEX);
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

    if (GetMonitorInfo(hMonitor, &mi)) {
        if (mi.dwFlags == MONITORINFOF_PRIMARY) {
            // This is the primary monitor - we assume it's the laptop's built-in display.
            return SetLaptopScreenBrightness(brightness);
        }
        else {
            // More than one monitor is connected (or a monitor other than the primary one)
            return SetExternalMonitorBrightness(brightness);
        }
    }

    return false; // If GetMonitorInfo failed
}*/
