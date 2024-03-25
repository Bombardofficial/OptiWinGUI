#ifndef BRIGHTNESS_CONTROL_H
#define BRIGHTNESS_CONTROL_H

#include <Windows.h>
#include <HighLevelMonitorConfigurationAPI.h>
#include <PhysicalMonitorEnumerationAPI.h>




bool SetExternalMonitorBrightness(DWORD brightness);
bool GetExternalMonitorBrightness(DWORD &currentBrightness);
bool SetMonitorBrightness(DWORD brightness);
bool SetInternalMonitorBrightness(DWORD brightness);


#endif // BRIGHTNESS_CONTROL_H
