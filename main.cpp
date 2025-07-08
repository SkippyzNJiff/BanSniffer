#include "SystemInfoChecker.h"
#include "ConsoleUtils.h"
#include <iostream>
#include <conio.h>
#include <string>
#define NOMINMAX  // Prevent windows.h from defining aids
#include <windows.h>
#include <iomanip>
#include <algorithm>
#include <limits>

int width = 9005;
int height = 4000;

//void setConsoleSquare(int chars, int px) {
//    // 1. Buffer/window in characters
//    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
//    SMALL_RECT tempWindow = { 0,0,0,0 };
//    SetConsoleWindowInfo(hOut, TRUE, &tempWindow);
//    COORD bufSize = { (SHORT)chars, (SHORT)chars };
//    SetConsoleScreenBufferSize(hOut, bufSize);
//    SMALL_RECT winSize = { 0,0,(SHORT)(chars - 1),(SHORT)(chars - 1) };
//    SetConsoleWindowInfo(hOut, TRUE, &winSize);
//
//    // 2. Pixel size in MoveWindow
//    HWND hwnd = GetConsoleWindow();
//    MoveWindow(hwnd, 100, 100, px, px, TRUE);
//}

class SystemCheckerApp {
private:
    SystemInfoChecker checker;
    std::string serialsFile = "system_serials.dat";

    void clearInputBuffer() {
        // Clear both _kbhit buffer and cin buffer
        while (_kbhit()) {
            _getch();
        }
        if (std::cin.rdbuf()->in_avail() > 0) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    void displayMainMenu() {
        ConsoleUtils::clearScreen();
        ConsoleUtils::printBox("SYSTEM INFO CHECKER V1.67\nOne-Click Summary\n\nMade By The Splosh Larp", ConsoleUtils::CYAN);

        std::cout << "\n";
        ConsoleUtils::setColor(ConsoleUtils::YELLOW);
        std::cout << "Main Menu:\n";
        ConsoleUtils::resetColor();

        std::cout << "  1. Show System Summary\n";
        std::cout << "  2. Save Current Serials\n";
        std::cout << "  0. Exit\n\n";

        ConsoleUtils::setColor(ConsoleUtils::DARK_WHITE);
        std::cout << "Select option: ";
        ConsoleUtils::resetColor();
    }

    void printSerialWithStatus(const std::string& label, const std::string& value, bool hasChanged, bool hasSavedData, int labelWidth = 20, int valueWidth = 30) {
        // Truncate label if too long
        std::string displayLabel = label;
        if (displayLabel.length() > labelWidth - 1) {
            displayLabel = displayLabel.substr(0, labelWidth - 4) + "...";
        }

        // Print label
        ConsoleUtils::setColor(ConsoleUtils::DARK_WHITE);
        std::cout << "  " << std::left << std::setw(labelWidth) << displayLabel << ": ";

        // Truncate value if too long
        std::string displayValue = value;
        if (displayValue.length() > valueWidth - 1) {
            displayValue = displayValue.substr(0, valueWidth - 4) + "...";
        }

        // Print value
        ConsoleUtils::setColor(ConsoleUtils::CYAN);
        std::cout << std::left << std::setw(valueWidth) << displayValue;

        // Print status
        std::cout << " | ";

        // Updated logic: treat "Not Available" as CHANGED if we have saved data
        if (!hasSavedData) {
            ConsoleUtils::setColor(ConsoleUtils::YELLOW);
            std::cout << "NO BASELINE";
        }
        else if (value == "Not Available" || value.empty()) {
            // If we have saved data but current value is not available, it's been nulled/changed
            ConsoleUtils::setColor(ConsoleUtils::GREEN);
            std::cout << "CHANGED";
        }
        else if (hasChanged) {
            ConsoleUtils::setColor(ConsoleUtils::GREEN);
            std::cout << "CHANGED";
        }
        else {
            ConsoleUtils::setColor(ConsoleUtils::RED);
            std::cout << "UNCHANGED";
        }

        ConsoleUtils::resetColor();
        std::cout << "\n";
    }

    void showSystemSummary() {
        ConsoleUtils::clearScreen();
        ConsoleUtils::printHeader("SYSTEM SUMMARY", ConsoleUtils::CYAN);

        // System Info
        auto info = checker.getSystemInfo();
        ConsoleUtils::printItem("Windows Version", info.windowsVersion);
        ConsoleUtils::printItem("Computer Name", info.computerName);
        ConsoleUtils::printItem("Current User", info.userName);
        ConsoleUtils::printItem("Architecture", info.architecture);
        ConsoleUtils::printItem("Total Memory", info.totalMemory);
        ConsoleUtils::printItem("System Uptime", info.uptime);

        // Serials
        auto serials = checker.getSystemSerials();
        SystemSerials savedSerials;
        bool hasSaved = checker.loadSerials(savedSerials, serialsFile);
        std::map<std::string, bool> changes;
        if (hasSaved) {
            changes = checker.compareSerials(serials, savedSerials);
        }

        ConsoleUtils::printSubHeader("Serial Numbers");

        // Add legend for status colors
        std::cout << "  Status: ";
        ConsoleUtils::setColor(ConsoleUtils::GREEN);
        std::cout << "CHANGED";
        ConsoleUtils::resetColor();
        std::cout << " = Modified | ";
        ConsoleUtils::setColor(ConsoleUtils::RED);
        std::cout << "UNCHANGED";
        ConsoleUtils::resetColor();
        std::cout << " = Same | ";
        ConsoleUtils::setColor(ConsoleUtils::YELLOW);
        std::cout << "NO BASELINE";
        ConsoleUtils::resetColor();
        std::cout << " = First run\n\n";

        // Calculate max width for better alignment
        int maxSerialLength = 0;
        maxSerialLength = (std::max)(maxSerialLength, (int)serials.cpuId.length());
        maxSerialLength = (std::max)(maxSerialLength, (int)serials.motherboardSerial.length());
        maxSerialLength = (std::max)(maxSerialLength, (int)serials.biosSerial.length());
        for (const auto& disk : serials.diskSerials) {
            maxSerialLength = (std::max)(maxSerialLength, (int)disk.length());
        }
        for (const auto& adapter : serials.networkAdapters) {
            maxSerialLength = (std::max)(maxSerialLength, (int)adapter.second.length());
        }
        int valueWidth = (std::min)(40, (std::max)(30, maxSerialLength + 2)); // Cap at 40 chars
        int labelWidth = 25; // Increased for network adapter names

        // CPU ID
        printSerialWithStatus("CPU ID", serials.cpuId, hasSaved && changes["CPU ID"], hasSaved, labelWidth, valueWidth);

        // Motherboard Serial
        printSerialWithStatus("Motherboard Serial", serials.motherboardSerial, hasSaved && changes["Motherboard Serial"], hasSaved, labelWidth, valueWidth);

        // BIOS Serial
        printSerialWithStatus("BIOS Serial", serials.biosSerial, hasSaved && changes["BIOS Serial"], hasSaved, labelWidth, valueWidth);

        // Disk Serials
        bool diskChanged = hasSaved && changes["Disk Serials"];
        int diskIndex = 0;
        for (const auto& disk : serials.diskSerials) {
            std::stringstream label;
            label << "Disk " << diskIndex++;
            printSerialWithStatus(label.str(), disk, diskChanged, hasSaved, labelWidth, valueWidth);
        }

        // Network Adapters
        bool netChanged = hasSaved && changes["Network Adapters"];
        for (const auto& adapter : serials.networkAdapters) {
            printSerialWithStatus(adapter.first, adapter.second, netChanged, hasSaved, labelWidth, valueWidth);
        }

        // Security
        auto status = checker.getSecurityStatus();
        ConsoleUtils::printSubHeader("Security Status");
        ConsoleUtils::printItem("Defender Service", status.defenderServiceStatus,
            ConsoleUtils::DARK_WHITE,
            status.defenderServiceStatus == "Running" ? ConsoleUtils::GREEN : ConsoleUtils::RED);
        ConsoleUtils::printItem("Real-time Protection",
            status.realtimeProtectionEnabled ? "Enabled" : "Disabled",
            ConsoleUtils::DARK_WHITE,
            status.realtimeProtectionEnabled ? ConsoleUtils::GREEN : ConsoleUtils::RED);
        ConsoleUtils::printItem("DEP", status.depEnabled ? "Enabled" : "Disabled",
            ConsoleUtils::DARK_WHITE,
            status.depEnabled ? ConsoleUtils::GREEN : ConsoleUtils::YELLOW);
        ConsoleUtils::printItem("ASLR", status.aslrStatus);
        ConsoleUtils::printItem("Control Flow Guard",
            status.controlFlowGuardEnabled ? "Enabled" : "Disabled",
            ConsoleUtils::DARK_WHITE,
            status.controlFlowGuardEnabled ? ConsoleUtils::GREEN : ConsoleUtils::YELLOW);
        if (!status.antivirusProducts.empty()) {
            ConsoleUtils::printSubHeader("Antivirus Products");
            for (const auto& av : status.antivirusProducts)
                ConsoleUtils::printItem("Antivirus", av, ConsoleUtils::DARK_WHITE, ConsoleUtils::GREEN);
        }
    }

    void saveCurrentSerials() {
        ConsoleUtils::clearScreen();
        ConsoleUtils::printHeader("SAVE SERIALS", ConsoleUtils::YELLOW);
        auto serials = checker.getSystemSerials();

        std::cout << "Save to file (default: " << serialsFile << "): ";
        std::string filename;
        std::getline(std::cin, filename);
        if (filename.empty()) filename = serialsFile;

        if (checker.saveSerials(serials, filename)) {
            ConsoleUtils::printSuccess("Serials saved successfully to " + filename);
            ConsoleUtils::printInfo("Saved at: " + serials.timestamp);
        }
        else {
            ConsoleUtils::printError("Failed to save serials to " + filename);
        }
    }

    void waitForKey() {
        ConsoleUtils::setColor(ConsoleUtils::DARK_WHITE);
        std::cout << "\nPress any key to continue...";
        ConsoleUtils::resetColor();

        // Clear any existing input first
        clearInputBuffer();

        // Wait for a single key press
        _getch();

        // Clear buffer again after key press
        clearInputBuffer();
    }

public:
    void run() {
        ConsoleUtils::initialize();

        if (!checker.isWMIInitialized()) {
            ConsoleUtils::printError("Failed to initialize WMI. Some features may not work."); // retigga
            ConsoleUtils::printWarning("Try running as Administrator for full functionality.");
            std::cout << "\n";
        }

        char choice;
        bool running = true;

        while (running) {
            clearInputBuffer(); // Clear before showing menu
            displayMainMenu();

            choice = _getch();
            std::cout << choice << "\n\n";

            switch (choice) {
            case '1':
                showSystemSummary();
                waitForKey();
                break;
            case '2':
                clearInputBuffer(); // Clear before getting input
                saveCurrentSerials();
                waitForKey();
                break;
            case '0':
                running = false;
                ConsoleUtils::printInfo("Exiting...");
                break;
            default:
                ConsoleUtils::printError("Invalid option. Please try again.");
                Sleep(1000); // Brief pause for error message
                break;
            }
        }
    }
};

HWND findOwnWindow() {
    struct EnumData {
        DWORD pid;
        HWND hwnd;
    } data;
    data.pid = GetCurrentProcessId();
    data.hwnd = NULL;

    auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* data = reinterpret_cast<EnumData*>(lParam);
        DWORD wndPid = 0;
        GetWindowThreadProcessId(hwnd, &wndPid);
        if (wndPid == data->pid && IsWindowVisible(hwnd)) {
            data->hwnd = hwnd;
            return FALSE; // Stop enumeration, found one
        }
        return TRUE;
        };

    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

void resizeConsole(short width, short height) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // 1. Set the screen buffer size
    COORD newSize = { width, height };
    if (!SetConsoleScreenBufferSize(hOut, newSize)) {
        std::cerr << "Failed to set buffer size." << std::endl;
    }

    // 2. Set the window size (MUST be <= buffer size)
    SMALL_RECT displayArea = { 0, 0, width - 1, height - 1 };
    if (!SetConsoleWindowInfo(hOut, TRUE, &displayArea)) {
        std::cerr << "Failed to set window size." << std::endl;
    }
}


int main() {
    resizeConsole(85, 40);
    ConsoleUtils::initialize();
    try {
        SystemCheckerApp app;
        app.run();
    }
    catch (const std::exception& e) {
        ConsoleUtils::printError(std::string("Fatal error: ") + e.what());
        ConsoleUtils::pause();
        return 1;
    }
    return 0;
}