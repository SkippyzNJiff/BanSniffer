#include "system_serials.hpp"      // FAST WinAPI hardware serials (new code)
#include "SystemInfoChecker.h"     // WMI OS info, security info (old code)
#include "ConsoleUtils.h"
#include <iostream>
#include <conio.h>
#include <string>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <fstream>

class SystemCheckerApp {
private:
    SystemInfoChecker checker; // For WMI/OS/security info
    std::string serialsFile = "system_serials.dat";

    void clearInputBuffer() {
        while (_kbhit()) { _getch(); }
        if (std::cin.rdbuf()->in_avail() > 0)
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void displayMainMenu() {
        ConsoleUtils::clearScreen();
        ConsoleUtils::printBox("SYSTEM INFO CHECKER \n Made By The Splosh Larp \n Version 2.0", ConsoleUtils::CYAN);
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
        std::string displayLabel = label;
        if (displayLabel.length() > labelWidth - 1)
            displayLabel = displayLabel.substr(0, labelWidth - 4) + "...";
        ConsoleUtils::setColor(ConsoleUtils::DARK_WHITE);
        std::cout << "  " << std::left << std::setw(labelWidth) << displayLabel << ": ";
        std::string displayValue = value;
        if (displayValue.length() > valueWidth - 1)
            displayValue = displayValue.substr(0, valueWidth - 4) + "...";
        ConsoleUtils::setColor(ConsoleUtils::CYAN);
        std::cout << std::left << std::setw(valueWidth) << displayValue;
        std::cout << " | ";
        if (!hasSavedData) {
            ConsoleUtils::setColor(ConsoleUtils::YELLOW); std::cout << "NO BASELINE";
        }
        else if (value == "Not Available" || value.empty() || hasChanged) {
            ConsoleUtils::setColor(ConsoleUtils::GREEN); std::cout << "CHANGED";
        }
        else {
            ConsoleUtils::setColor(ConsoleUtils::RED); std::cout << "UNCHANGED";
        }
        ConsoleUtils::resetColor(); std::cout << "\n";
    }

    // ---- Serial save/load/compare using WinAPI-only serials ----
    bool saveSerials(const SystemSerials& s, const std::string& filename) {
        std::ofstream out(filename, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << s.cpuId << "\n" << s.biosSerial << "\n" << s.motherboardSerial << "\n";
        out << s.diskSerials.size() << "\n";
        for (const auto& d : s.diskSerials) out << d << "\n";
        out << s.networkAdapters.size() << "\n";
        for (const auto& p : s.networkAdapters) out << p.first << "\n" << p.second << "\n";
        return true;
    }
    bool loadSerials(SystemSerials& s, const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) return false;
        getline(in, s.cpuId);
        getline(in, s.biosSerial);
        getline(in, s.motherboardSerial);
        size_t n = 0;
        in >> n; in.ignore();
        s.diskSerials.clear();
        for (size_t i = 0; i < n; ++i) {
            std::string d; getline(in, d); s.diskSerials.push_back(d);
        }
        in >> n; in.ignore();
        s.networkAdapters.clear();
        for (size_t i = 0; i < n; ++i) {
            std::string k, v; getline(in, k); getline(in, v);
            s.networkAdapters.push_back(std::make_pair(k, v)); 
        }
        return true;
    }

    std::map<std::string, bool> compareSerials(const SystemSerials& a, const SystemSerials& b) {
        std::map<std::string, bool> result;
        result["CPU ID"] = (a.cpuId != b.cpuId);
        result["BIOS Serial"] = (a.biosSerial != b.biosSerial);
        result["Motherboard Serial"] = (a.motherboardSerial != b.motherboardSerial);
        result["Disk Serials"] = (a.diskSerials != b.diskSerials);
        result["Network Adapters"] = (a.networkAdapters != b.networkAdapters);
        return result;
    }

    void showSystemSummary() {
        ConsoleUtils::clearScreen();
        ConsoleUtils::printHeader("SYSTEM SUMMARY", ConsoleUtils::CYAN);



        // ----- PART 1: OS / User / Memory / Uptime (WMI) -----
        auto info = checker.getSystemInfo();
        ConsoleUtils::printItem("Windows Version", info.windowsVersion);
        ConsoleUtils::printItem("Computer Name", info.computerName);
        ConsoleUtils::printItem("Current User", info.userName);
        ConsoleUtils::printItem("Architecture", info.architecture);
        ConsoleUtils::printItem("Total Memory", info.totalMemory);
        ConsoleUtils::printItem("System Uptime", info.uptime);

        // ----- PART 2: Hardware Serials (WinAPI only) -----
        auto serials = checker.getSystemSerials();
        SystemSerials savedSerials;
        bool hasSaved = loadSerials(savedSerials, serialsFile);
        std::map<std::string, bool> changes;
        if (hasSaved)
            changes = compareSerials(serials, savedSerials);

        ConsoleUtils::printSubHeader("Hardware Serials");
        std::cout << "\033[1;31m Please note that it may take up to a minute to display recently changed serials!\033[0m\n";
        std::cout << "  Status: ";
        ConsoleUtils::setColor(ConsoleUtils::GREEN); std::cout << "CHANGED"; ConsoleUtils::resetColor(); std::cout << " = Modified | ";
        ConsoleUtils::setColor(ConsoleUtils::RED); std::cout << "UNCHANGED"; ConsoleUtils::resetColor(); std::cout << " = Same | ";
        ConsoleUtils::setColor(ConsoleUtils::YELLOW); std::cout << "NO BASELINE"; ConsoleUtils::resetColor(); std::cout << " = First run\n\n";

        int maxSerialLength = 0;
        maxSerialLength = (std::max)(maxSerialLength, (int)serials.cpuId.length());
        maxSerialLength = (std::max)(maxSerialLength, (int)serials.motherboardSerial.length());
        maxSerialLength = (std::max)(maxSerialLength, (int)serials.biosSerial.length());
        for (const auto& disk : serials.diskSerials)
            maxSerialLength = (std::max)(maxSerialLength, (int)disk.length());
        for (const auto& adapter : serials.networkAdapters)
            maxSerialLength = (std::max)(maxSerialLength, (int)adapter.second.length());
        int valueWidth = (std::min)(40, (std::max)(30, maxSerialLength + 2));
        int labelWidth = 25;

        printSerialWithStatus("CPU ID", serials.cpuId, hasSaved && changes["CPU ID"], hasSaved, labelWidth, valueWidth);
        printSerialWithStatus("Motherboard Serial", serials.motherboardSerial, hasSaved && changes["Motherboard Serial"], hasSaved, labelWidth, valueWidth);
        printSerialWithStatus("BIOS Serial", serials.biosSerial, hasSaved && changes["BIOS Serial"], hasSaved, labelWidth, valueWidth);

        bool diskChanged = hasSaved && changes["Disk Serials"];
        int diskIndex = 0;
        for (const auto& disk : serials.diskSerials) {
            std::stringstream label;
            label << "Disk " << diskIndex++;
            printSerialWithStatus(label.str(), disk, diskChanged, hasSaved, labelWidth, valueWidth);
        }
        bool netChanged = hasSaved && changes["Network Adapters"];
        for (const auto& adapter : serials.networkAdapters)
            printSerialWithStatus(adapter.first, adapter.second, netChanged, hasSaved, labelWidth, valueWidth);

        // ----- PART 3: Security (WMI) -----
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
        // Instantly save to default file, no prompt
        std::string filename = serialsFile;
        if (saveSerials(serials, filename)) {
            ConsoleUtils::printSuccess("Serials saved successfully to " + filename);
        }
        else {
            ConsoleUtils::printError("Failed to save serials to " + filename);
        }
        // Wait for 3 seconds
        Sleep(1000);
    }

    void waitForKey() {
        ConsoleUtils::setColor(ConsoleUtils::DARK_WHITE);
        std::cout << "\nPress any key to continue...";
        ConsoleUtils::resetColor();
        clearInputBuffer();
        _getch();
        clearInputBuffer();
    }

public:
    void run() {
        ConsoleUtils::initialize();
        if (!checker.isWMIInitialized()) {
            ConsoleUtils::printError("Failed to initialize WMI. Some features may not work.");
            ConsoleUtils::printWarning("Try running as Administrator for full functionality.");
            std::cout << "\n";
        }

        char choice;
        bool running = true;
        while (running) {
            clearInputBuffer();
            displayMainMenu();
            choice = _getch();
            std::cout << choice << "\n\n";
            switch (choice) {
            case '1': showSystemSummary(); waitForKey(); break;
            case '2': clearInputBuffer(); saveCurrentSerials(); waitForKey(); break;
            case '0': running = false; ConsoleUtils::printInfo("Exiting..."); break;
            default: ConsoleUtils::printError("Invalid option. Please try again."); Sleep(1000); break;
            }
        }
    }
};

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
