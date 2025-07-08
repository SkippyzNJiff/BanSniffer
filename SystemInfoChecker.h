#pragma once
#ifndef SYSTEM_INFO_CHECKER_H
#define SYSTEM_INFO_CHECKER_H

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <comdef.h>
#include <Wbemidl.h>
#include <sstream>  // add if not present
#include <vector>
#include <map>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

struct SystemSerials {
    std::string cpuId;
    std::string motherboardSerial;
    std::string biosSerial;
    std::vector<std::string> diskSerials;
    std::vector<std::pair<std::string, std::string>> networkAdapters; // name, MAC
    std::string timestamp;
};

struct SecurityStatus {
    std::string defenderServiceStatus;
    bool realtimeProtectionEnabled;
    bool depEnabled;
    std::string aslrStatus;
    bool controlFlowGuardEnabled;
    std::vector<std::string> antivirusProducts;
};

struct SystemInfo {
    std::string windowsVersion;
    std::string computerName;
    std::string userName;
    std::string uptime;
    std::string architecture;
    std::string totalMemory;
};

class SystemInfoChecker {
private:
    IWbemLocator* pLoc;
    IWbemServices* pSvc;
    bool wmiInitialized;

    bool initializeWMI();
    void cleanupWMI();
    std::string getWMIProperty(const std::string& wmiClass, const std::string& property);
    std::vector<std::map<std::string, std::string>> getWMIMultipleProperties(
        const std::string& wmiClass, const std::vector<std::string>& properties);

public:
    SystemInfoChecker();
    ~SystemInfoChecker();

    SystemSerials getSystemSerials();
    SecurityStatus getSecurityStatus();
    SystemInfo getSystemInfo();

    bool saveSerials(const SystemSerials& serials, const std::string& filename = "serials.dat");
    bool loadSerials(SystemSerials& serials, const std::string& filename = "serials.dat");
    std::map<std::string, bool> compareSerials(const SystemSerials& current, const SystemSerials& saved);

    static std::string getCurrentTimestamp();
    bool isWMIInitialized() const { return wmiInitialized; }
};

#endif // SYSTEM_INFO_CHECKER_H
