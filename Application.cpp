/*#include "Application.h"
#include "brightness_control.h"
#include "DisplayManager.h"
#include "ProcessManager.h"
#include "PowerManager.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <mutex>

std::mutex monitoringMutex;

Application::Application() {
    // Constructor implementation (if needed)
}

void Application::StartApplication() {
    int mainChoice = 0;
    bool running = true;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed with error: " << hr << std::endl;
    }

    PowerManager powerManager;
    std::thread powerManagerThread;
    bool monitoringEnabled = false;

    while (running) {
        std::cout << "Choose a menu:" << std::endl;
        std::cout << "1. Static Menu" << std::endl;
        std::cout << "2. Dynamic Menu" << std::endl;
        std::cout << "3. Exit" << std::endl;
        std::cout << "> ";
        std::cin >> mainChoice;

        // Validate user input
        if (std::cin.fail()) {
            std::cin.clear(); // Clear error flags
#undef max
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard input
            std::cout << "Invalid input. Please enter a number." << std::endl;
            continue;
        }

        switch (mainChoice) {
        case 1:
            system("cls");
            StaticMenu(powerManager, powerManagerThread, monitoringEnabled);
            break;
        case 2:
            system("cls");
            DynamicMenu(powerManager, powerManagerThread, monitoringEnabled);
            break;
        case 3:
            running = false;
            break;
        default:
            std::cout << "Invalid option selected." << std::endl;
            break;
        }
    }
    std::lock_guard<std::mutex> lock(monitoringMutex);
    if (monitoringEnabled && powerManagerThread.joinable()) {
        powerManager.stopMonitoring();
        powerManagerThread.join();
    }

    CoUninitialize();
}

void Application::StaticMenu(PowerManager& powerManager, std::thread& powerManagerThread, bool& monitoringEnabled) {
    // Your existing menu code goes here
    // Main application loop (move the content of your current main function here)
    int staticChoice = 0;
    bool staticRunning = true;

    while (staticRunning) {
        std::cout << "Choose an option:" << std::endl;
        std::cout << "1. Get current power plan" << std::endl;
        std::cout << "2. Set power plan" << std::endl;
        std::cout << "3. Set monitor brightness (external monitors only)" << std::endl;
        std::cout << "4. Set refresh rate" << std::endl;
        std::cout << "5. Get list of running processes" << std::endl;
        std::cout << "6. Back to Main Menu" << std::endl;
        std::cout << "7. Exit from application" << std::endl;
        std::cout << "> ";
        std::cin >> staticChoice;

        if (!std::cin) {
            std::cin.clear(); // Clear the error flag
#undef max
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore the rest of the line
            std::cout << "Invalid input. Please enter a number." << std::endl;
            continue;
        }

        switch (staticChoice) {
        case 1:
            system("cls");
            powerManager.GetCurrentPowerPlan();
            pauseAndClear();
            break;
        case 2: {
            system("cls");
            std::cin.ignore(); // to consume the leftover newline from std::cin
            std::string powerPlan;
            std::cout << "Enter new power plan name (Balanced, Power saver, High performance): ";
            std::getline(std::cin, powerPlan);
            powerManager.SetPowerPlan(powerPlan);
            pauseAndClear();
            break;
        }
        case 3: {
            system("cls");
            DWORD brightness;
            std::cout << "Enter brightness level (0-100): ";
            std::cin >> brightness;
            if (SetExternalMonitorBrightness(brightness)) {
                std::cout << "Brightness set successfully." << std::endl;
            }
            else {
                std::cout << "Failed to set brightness." << std::endl;
            }
            pauseAndClear();
            break;
        }
        case 4: {
            system("cls");
            DisplayManager displayManager;
            auto supportedModes = displayManager.listSupportedModes();

            // Filter and display only relevant modes
            std::sort(supportedModes.begin(), supportedModes.end());
            supportedModes.erase(std::unique(supportedModes.begin(), supportedModes.end(),
                                             [](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second == b.second; }),
                                 supportedModes.end());

            std::cout << "Supported display modes (Refresh Rate):" << std::endl;
            int modeIndex = 1;
            for (const auto& mode : supportedModes) {
                std::cout << modeIndex++ << ". " << mode.second << " Hz" << std::endl;
            }

            std::cout << "Enter the number of the mode you want to set: ";
            int userChoice;
            std::cin >> userChoice;

            if (userChoice < 1 || userChoice > supportedModes.size()) {
                std::cout << "Invalid choice. Please select a valid mode number." << std::endl;
                break;
            }

            int selectedRefreshRate = supportedModes[userChoice - 1].second;

            if (displayManager.setDisplayRefreshRate(selectedRefreshRate)) {
                std::cout << "Refresh rate set to " << selectedRefreshRate << " Hz successfully." << std::endl;
            }
            else {
                std::cout << "Failed to set refresh rate to " << selectedRefreshRate << " Hz." << std::endl;
            }
            pauseAndClear();
            break;
        }
        case 5: {
            system("cls");
            ProcessManager processManager;
            std::vector<std::string> processes = processManager.listProcesses();

            std::cout << "List of running processes:" << std::endl;
            for (size_t i = 0; i < processes.size(); ++i) {
                std::cout << i + 1 << ". " << processes[i] << std::endl;
            }

            std::cout << "\nEnter the number of the process you want to terminate (or 0 to exit): ";
            size_t choice;
            std::cin >> choice;

            if (choice > 0 && choice <= processes.size()) {
                std::string processToTerminate = processes[choice - 1];

                std::cout << "Are you sure you want to terminate \"" << processToTerminate << "\"? (yes/no): ";
                std::string confirmation;
                std::cin >> confirmation;

                if (confirmation == "yes") {
                    if (processManager.terminateProcess(processToTerminate)) {
                        std::cout << "\"" << processToTerminate << "\" has been terminated." << std::endl;
                    }
                    else {
                        std::cout << "Failed to terminate \"" << processToTerminate << "\"." << std::endl;
                    }
                }
            }
            system("cls");
            break;
        }
        case 6:
            system("cls");
            staticRunning = false; // Just exit the static menu loop
            break;
        case 7:
            system("cls");
            if (monitoringEnabled && powerManagerThread.joinable()) {
                powerManager.stopMonitoring(); // Ensure the monitoring thread is stopped before exiting
                powerManagerThread.join();
            }
            exit(0); // Exit the entire application
            break;
        default:
            std::cout << "Invalid option selected." << std::endl;
            break;
        }

    }
    // Clean up COM
    CoUninitialize();
}

void Application::DynamicMenu(PowerManager& powerManager, std::thread& powerManagerThread, bool& monitoringEnabled) {
    int dynamicChoice = 0;
    bool dynamicRunning = true;

    while (dynamicRunning) {
        powerManager.setMenuDisplayed(true);
        std::cout << "Dynamic Menu:" << std::endl;
        std::cout << "1. Normal Mode" << std::endl;
        std::cout << "2. Turbo Mode" << std::endl;
        std::cout << "3. Stop Current Monitoring" << std::endl;
        std::cout << "4. Back to Main Menu" << std::endl;
        std::cout << "> ";
        powerManager.setMenuDisplayed(false);
        std::cin >> dynamicChoice;


        switch (dynamicChoice) {
        case 1:
            system("cls");
            std::cout << "Normal Mode activated.\n" << std::endl;
            startMonitoringMode(powerManager, powerManagerThread, monitoringEnabled, &PowerManager::startNormalModeMonitoring);

            break;
        case 2:
            system("cls");
            std::cout << "Turbo Mode activated.\n" << std::endl;
            startMonitoringMode(powerManager, powerManagerThread, monitoringEnabled, &PowerManager::startTurboModeMonitoring);

            break;
        case 3:
            system("cls");
            if (monitoringEnabled && powerManagerThread.joinable()) {
                powerManager.stopMonitoring();
                powerManagerThread.join();
                monitoringEnabled = false;
                std::cout << "Monitoring stopped.\n" << std::endl;
            }
            else {
                std::cout << "No monitoring is currently running.\n" << std::endl;
            }
            break;
        case 4:
            dynamicRunning = false;
            system("cls");
            break;
        default:
            system("cls");
            std::cout << "Invalid option selected.\n" << std::endl;
            break;
        }
    }
}

void Application::startMonitoringMode(PowerManager& powerManager, std::thread& powerManagerThread, bool& monitoringEnabled, void (PowerManager::* monitoringFunction)()) {
    std::lock_guard<std::mutex> lock(monitoringMutex);
    if (monitoringEnabled && powerManagerThread.joinable()) {
        powerManager.stopMonitoring();
        powerManagerThread.join();
    }
    monitoringEnabled = true;
    powerManagerThread = std::thread(monitoringFunction, &powerManager);
}

void Application::pauseAndClear() {
    std::cout << "\nPress Enter to continue...";

    // Check if the input buffer is empty
    if (std::cin.rdbuf()->in_avail() > 0) {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::cin.get(); // Wait for user to press Enter
    system("cls");
}*/

// Implement other private methods as needed
