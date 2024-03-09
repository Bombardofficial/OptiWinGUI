#include "ProcessManager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>

ProcessManager::ProcessManager() {
    // Constructor
}

std::vector<std::string> ProcessManager::listProcesses() {
    std::vector<std::string> processList;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        do {
            char processName[MAX_PATH];
            memset(processName, 0, sizeof(processName)); // Zero-initialize the array
            size_t convertedChars = 0;
            wcstombs_s(&convertedChars, processName, entry.szExeFile, MAX_PATH);
            processName[MAX_PATH - 1] = '\0'; // Ensure null termination
            processList.push_back(std::string(processName));
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return processList;
}

bool ProcessManager::terminateProcess(const std::string& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    bool terminated = false;

    if (Process32First(snapshot, &entry)) {
        do {
            char exeName[MAX_PATH];
            size_t convertedChars = 0;
            wcstombs_s(&convertedChars, exeName, entry.szExeFile, MAX_PATH);
            exeName[MAX_PATH - 1] = '\0'; // Ensure null termination
            if (strcmp(exeName, processName.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (hProcess != NULL) {
                    terminated = TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return terminated;
}
