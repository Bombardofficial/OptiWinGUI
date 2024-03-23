#include "ProcessManager.h"

#include <windows.h>

#include <tlhelp32.h>

#include <vector>

#include <string>

#include <set>
#include <Psapi.h>
ProcessManager::ProcessManager() {
    // Constructor
}

std::vector<std::string> ProcessManager::listProcesses() {
    std::set<std::string> uniqueProcessNames;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        do {
            TCHAR processPath[MAX_PATH] = { 0 };
            // Retrieve the full path of the executable file for the process
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
            if (hProcess) {
                if (GetModuleFileNameEx(hProcess, NULL, processPath, MAX_PATH) > 0) {
                    // Convert from TCHAR to std::string
                    char cProcessPath[MAX_PATH];
                    wcstombs(cProcessPath, processPath, MAX_PATH);
                    std::string pathStr(cProcessPath);

                    // Check if it's a system process by examining the path
                    if (pathStr.find("\\Windows\\") == std::string::npos) {
                        // Not a system process, so we can add it
                        char cExeName[MAX_PATH];
                        wcstombs(cExeName, entry.szExeFile, MAX_PATH);
                        uniqueProcessNames.insert(std::string(cExeName));
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    // Convert the set to a vector to return
    return std::vector<std::string>(uniqueProcessNames.begin(), uniqueProcessNames.end());
}
bool ProcessManager::terminateProcess(const std::string & processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    bool terminated = false;

    if (Process32First(snapshot, & entry)) {
        do {
            char exeName[MAX_PATH];
            size_t convertedChars = 0;
            wcstombs_s( & convertedChars, exeName, entry.szExeFile, MAX_PATH);
            exeName[MAX_PATH - 1] = '\0'; // Ensure null termination
            if (strcmp(exeName, processName.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (hProcess != NULL) {
                    terminated = TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
                break;
            }
        } while (Process32Next(snapshot, & entry));
    }

    CloseHandle(snapshot);
    return terminated;
}
