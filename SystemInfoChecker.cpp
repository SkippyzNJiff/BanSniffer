// systeminfochecker.cpp

#include "SystemInfoChecker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winreg.h>

#pragma comment(lib, "psapi.lib")

SystemInfoChecker::SystemInfoChecker() : pLoc(NULL), pSvc(NULL), wmiInitialized(false) {
    wmiInitialized = initializeWMI();
}

SystemInfoChecker::~SystemInfoChecker() {
    cleanupWMI();
}

bool SystemInfoChecker::initializeWMI() {
    HRESULT hres;

    // Initialize COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres) && hres != RPC_E_CHANGED_MODE) {
        return false;
    }

    // Set security levels
    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return false;
    }

    // Create WMI locator
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }

    // Connect to WMI
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    // Set proxy blanket
    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    return true;
}

void SystemInfoChecker::cleanupWMI() {
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    if (wmiInitialized) CoUninitialize();
}

std::string SystemInfoChecker::getWMIProperty(const std::string& wmiClass, const std::string& property) {
    if (!wmiInitialized) return "WMI Not Initialized";

    std::string result = "Not Available";
    IEnumWbemClassObject* pEnumerator = NULL;

    std::string query = "SELECT " + property + " FROM " + wmiClass;
    BSTR bstrQuery = SysAllocString(std::wstring(query.begin(), query.end()).c_str());

    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstrQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (SUCCEEDED(hres)) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
            VARIANT vtProp;
            std::wstring wprop(property.begin(), property.end());
            hres = pclsObj->Get(wprop.c_str(), 0, &vtProp, 0, 0);

            if (SUCCEEDED(hres)) {
                if (vtProp.vt == VT_BSTR) {
                    result = _com_util::ConvertBSTRToString(vtProp.bstrVal);
                }
                else if (vtProp.vt == VT_I4) {
                    result = std::to_string(vtProp.intVal);
                }
                else if (vtProp.vt == VT_UI4) {
                    result = std::to_string(vtProp.uintVal);
                }
            }
            VariantClear(&vtProp);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    SysFreeString(bstrQuery);
    return result;
}

std::vector<std::map<std::string, std::string>> SystemInfoChecker::getWMIMultipleProperties(
    const std::string& wmiClass, const std::vector<std::string>& properties) {

    std::vector<std::map<std::string, std::string>> results;
    if (!wmiInitialized) return results;

    IEnumWbemClassObject* pEnumerator = NULL;
    std::string query = "SELECT * FROM " + wmiClass;
    BSTR bstrQuery = SysAllocString(std::wstring(query.begin(), query.end()).c_str());

    HRESULT hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstrQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (SUCCEEDED(hres)) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) break;

            std::map<std::string, std::string> item;

            for (const auto& prop : properties) {
                VARIANT vtProp;
                std::wstring wprop(prop.begin(), prop.end());
                hr = pclsObj->Get(wprop.c_str(), 0, &vtProp, 0, 0);

                if (SUCCEEDED(hr)) {
                    if (vtProp.vt == VT_BSTR) {
                        item[prop] = _com_util::ConvertBSTRToString(vtProp.bstrVal);
                    }
                    else if (vtProp.vt == VT_I4) {
                        item[prop] = std::to_string(vtProp.intVal);
                    }
                    else if (vtProp.vt == VT_UI4) {
                        item[prop] = std::to_string(vtProp.uintVal);
                    }
                    else {
                        item[prop] = "N/A";
                    }
                }
                VariantClear(&vtProp);
            }

            results.push_back(item);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    SysFreeString(bstrQuery);
    return results;
}

SystemSerials SystemInfoChecker::getSystemSerials() {
    SystemSerials serials;

    // Get CPU ID
    serials.cpuId = getWMIProperty("Win32_Processor", "ProcessorId");

    // Get Motherboard Serial
    serials.motherboardSerial = getWMIProperty("Win32_BaseBoard", "SerialNumber");

    // Get BIOS Serial
    serials.biosSerial = getWMIProperty("Win32_BIOS", "SerialNumber");

    // Get Disk Serials
    auto disks = getWMIMultipleProperties("Win32_DiskDrive", { "SerialNumber", "Model" });
    for (const auto& disk : disks) {
        if (disk.find("SerialNumber") != disk.end()) {
            std::string serial = disk.at("SerialNumber");
            if (serial != "N/A" && !serial.empty()) {
                serials.diskSerials.push_back(serial);
            }
        }
    }

    // Get Network Adapter MACs
    ULONG bufferSize = 0;
    GetAdaptersInfo(NULL, &bufferSize);

    if (bufferSize > 0) {
        PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
        if (GetAdaptersInfo(pAdapterInfo, &bufferSize) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                std::stringstream mac;
                for (int i = 0; i < pAdapter->AddressLength; i++) {
                    if (i > 0) mac << "-";
                    mac << std::hex << std::uppercase << std::setw(2)
                        << std::setfill('0') << (int)pAdapter->Address[i];
                }
                serials.networkAdapters.push_back(
                    std::make_pair(pAdapter->Description, mac.str())
                );
                pAdapter = pAdapter->Next;
            }
        }
        free(pAdapterInfo);
    }

    // Set timestamp
    serials.timestamp = getCurrentTimestamp();

    return serials;
}

SecurityStatus SystemInfoChecker::getSecurityStatus() {
    SecurityStatus status;

    // Check Windows Defender service
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager) {
        SC_HANDLE hService = OpenService(hSCManager, TEXT("WinDefend"), SERVICE_QUERY_STATUS);

        if (hService) {
            SERVICE_STATUS_PROCESS serviceStatus;
            DWORD bytesNeeded;

            if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
                (LPBYTE)&serviceStatus, sizeof(serviceStatus), &bytesNeeded)) {

                switch (serviceStatus.dwCurrentState) {
                case SERVICE_RUNNING:
                    status.defenderServiceStatus = "Running";
                    break;
                case SERVICE_STOPPED:
                    status.defenderServiceStatus = "Stopped";
                    break;
                case SERVICE_PAUSED:
                    status.defenderServiceStatus = "Paused";
                    break;
                default:
                    status.defenderServiceStatus = "Unknown";
                }
            }
            CloseServiceHandle(hService);
        }
        else {
            status.defenderServiceStatus = "Not Found";
        }
        CloseServiceHandle(hSCManager);
    }

    // Check real-time protection
    HKEY hKey;
    status.realtimeProtectionEnabled = true; // Default to enabled
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Microsoft\\Windows Defender\\Real-Time Protection"),
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD disableRealtimeMonitoring = 0;
        DWORD dataSize = sizeof(DWORD);

        if (RegQueryValueEx(hKey, TEXT("DisableRealtimeMonitoring"),
            NULL, NULL, (LPBYTE)&disableRealtimeMonitoring, &dataSize) == ERROR_SUCCESS) {
            status.realtimeProtectionEnabled = !disableRealtimeMonitoring;
        }

        RegCloseKey(hKey);
    }

    // Check DEP
    BOOL depEnabled = FALSE;
    DWORD depFlags = 0;
    if (GetProcessDEPPolicy(GetCurrentProcess(), &depFlags, &depEnabled)) {
        status.depEnabled = depEnabled;
    }

    // Check ASLR
    status.aslrStatus = "Unknown";
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD moveImages = 0;
        DWORD dataSize = sizeof(DWORD);

        if (RegQueryValueEx(hKey, TEXT("MoveImages"),
            NULL, NULL, (LPBYTE)&moveImages, &dataSize) == ERROR_SUCCESS) {
            switch (moveImages) {
            case 0: status.aslrStatus = "Disabled"; break;
            case 1: status.aslrStatus = "Enabled for ASLR images"; break;
            case 2: status.aslrStatus = "Enabled for all images"; break;
            }
        }

        RegCloseKey(hKey);
    }

    // Check Control Flow Guard
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfgPolicy = { 0 };
    if (GetProcessMitigationPolicy(GetCurrentProcess(),
        ProcessControlFlowGuardPolicy,
        &cfgPolicy, sizeof(cfgPolicy))) {
        status.controlFlowGuardEnabled = cfgPolicy.EnableControlFlowGuard;
    }

    // Get antivirus products
    IWbemLocator* pSecLoc = NULL;
    IWbemServices* pSecSvc = NULL;

    HRESULT hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pSecLoc);

    if (SUCCEEDED(hres)) {
        hres = pSecLoc->ConnectServer(
            _bstr_t(L"ROOT\\SecurityCenter2"),
            NULL, NULL, 0, NULL, 0, 0, &pSecSvc);

        if (SUCCEEDED(hres)) {
            IEnumWbemClassObject* pEnumerator = NULL;
            hres = pSecSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t("SELECT * FROM AntivirusProduct"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator);

            if (SUCCEEDED(hres)) {
                IWbemClassObject* pclsObj = NULL;
                ULONG uReturn = 0;

                while (pEnumerator) {
                    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) break;

                    VARIANT vtProp;
                    hr = pclsObj->Get(L"displayName", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
                        status.antivirusProducts.push_back(
                            _com_util::ConvertBSTRToString(vtProp.bstrVal));
                    }
                    VariantClear(&vtProp);
                    pclsObj->Release();
                }
                pEnumerator->Release();
            }
            pSecSvc->Release();
        }
        pSecLoc->Release();
    }

    return status;
}

SystemInfo SystemInfoChecker::getSystemInfo() {
    SystemInfo info;

    // Windows version
    OSVERSIONINFOEX osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(OSVERSIONINFOEX*);
    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(
        GetModuleHandle(TEXT("ntdll.dll")), "RtlGetVersion");

    if (RtlGetVersion && RtlGetVersion(&osvi) == 0) {
        std::stringstream version;
        version << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
            << " Build " << osvi.dwBuildNumber;
        info.windowsVersion = version.str();
    }

    // Computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        info.computerName = computerName;
    }

    // Username
    char userName[UNLEN + 1];
    size = sizeof(userName);
    if (GetUserNameA(userName, &size)) {
        info.userName = userName;
    }

    // System uptime
    ULONGLONG uptime = GetTickCount64() / 1000;
    std::stringstream uptimeStr;
    uptimeStr << uptime / 86400 << " days, "
        << (uptime % 86400) / 3600 << " hours, "
        << (uptime % 3600) / 60 << " minutes";
    info.uptime = uptimeStr.str();

    // Architecture
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        info.architecture = "x64";
        break;
    case PROCESSOR_ARCHITECTURE_INTEL:
        info.architecture = "x86";
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        info.architecture = "ARM64";
        break;
    default:
        info.architecture = "Unknown";
    }

    // Total memory
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        std::stringstream memStr;
        memStr << (memStatus.ullTotalPhys / (1024 * 1024 * 1024)) << " GB";
        info.totalMemory = memStr.str();
    }

    return info;
}

bool SystemInfoChecker::saveSerials(const SystemSerials& serials, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // Write timestamp
    size_t len = serials.timestamp.length();
    file.write(reinterpret_cast<const char*>(&len), sizeof(len));
    file.write(serials.timestamp.c_str(), len);

    // Write CPU ID
    len = serials.cpuId.length();
    file.write(reinterpret_cast<const char*>(&len), sizeof(len));
    file.write(serials.cpuId.c_str(), len);

    // Write Motherboard Serial
    len = serials.motherboardSerial.length();
    file.write(reinterpret_cast<const char*>(&len), sizeof(len));
    file.write(serials.motherboardSerial.c_str(), len);

    // Write BIOS Serial
    len = serials.biosSerial.length();
    file.write(reinterpret_cast<const char*>(&len), sizeof(len));
    file.write(serials.biosSerial.c_str(), len);

    // Write Disk Serials
    size_t count = serials.diskSerials.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& disk : serials.diskSerials) {
        len = disk.length();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(disk.c_str(), len);
    }

    // Write Network Adapters
    count = serials.networkAdapters.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& adapter : serials.networkAdapters) {
        len = adapter.first.length();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(adapter.first.c_str(), len);

        len = adapter.second.length();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(adapter.second.c_str(), len);
    }

    file.close();
    return true;
}

bool SystemInfoChecker::loadSerials(SystemSerials& serials, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    try {
        size_t len;
        char buffer[4096];

        // Read timestamp
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        file.read(buffer, len);
        serials.timestamp = std::string(buffer, len);

        // Read CPU ID
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        file.read(buffer, len);
        serials.cpuId = std::string(buffer, len);

        // Read Motherboard Serial
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        file.read(buffer, len);
        serials.motherboardSerial = std::string(buffer, len);

        // Read BIOS Serial
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        file.read(buffer, len);
        serials.biosSerial = std::string(buffer, len);

        // Read Disk Serials
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        serials.diskSerials.clear();
        for (size_t i = 0; i < count; i++) {
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            file.read(buffer, len);
            serials.diskSerials.push_back(std::string(buffer, len));
        }

        // Read Network Adapters
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        serials.networkAdapters.clear();
        for (size_t i = 0; i < count; i++) {
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            file.read(buffer, len);
            std::string name(buffer, len);

            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            file.read(buffer, len);
            std::string mac(buffer, len);

            serials.networkAdapters.push_back(std::make_pair(name, mac));
        }

        file.close();
        return true;
    }
    catch (...) {
        file.close();
        return false;
    }
}

std::map<std::string, bool> SystemInfoChecker::compareSerials(
    const SystemSerials& current, const SystemSerials& saved) {

    std::map<std::string, bool> changes;

    // Compare CPU ID
    changes["CPU ID"] = (current.cpuId != saved.cpuId);

    // Compare Motherboard Serial
    changes["Motherboard Serial"] = (current.motherboardSerial != saved.motherboardSerial);

    // Compare BIOS Serial
    changes["BIOS Serial"] = (current.biosSerial != saved.biosSerial);

    // Compare Disk Serials (any change counts as changed)
    bool diskChanged = (current.diskSerials.size() != saved.diskSerials.size());
    if (!diskChanged) {
        for (size_t i = 0; i < current.diskSerials.size(); i++) {
            bool found = false;
            for (const auto& savedDisk : saved.diskSerials) {
                if (current.diskSerials[i] == savedDisk) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                diskChanged = true;
                break;
            }
        }
    }
    changes["Disk Serials"] = diskChanged;

    // Compare Network Adapters
    bool netChanged = (current.networkAdapters.size() != saved.networkAdapters.size());
    if (!netChanged) {
        for (const auto& currentAdapter : current.networkAdapters) {
            bool found = false;
            for (const auto& savedAdapter : saved.networkAdapters) {
                if (currentAdapter.second == savedAdapter.second) { // Compare by MAC
                    found = true;
                    break;
                }
            }
            if (!found) {
                netChanged = true;
                break;
            }
        }
    }
    changes["Network Adapters"] = netChanged;

    return changes;
}

std::string SystemInfoChecker::getCurrentTimestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    localtime_s(&tstruct, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}