#pragma once
#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <vector>
#include <string>

class ProcessManager {
public:
    ProcessManager();
    std::vector<std::string> listProcesses();
    bool terminateProcess(const std::string& processName);
};

#endif // PROCESSMANAGER_H
