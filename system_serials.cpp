#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <ws2def.h>
#include <ws2ipdef.h> 
#include "system_serials.hpp"
#include <intrin.h>
#include <winioctl.h>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// cpu id
static std::string getCPUID() {
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    std::ostringstream oss;
    oss << std::hex << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    return oss.str();
}

// Helper: Get BIOS serial from registry
static std::string getBiosSerial() {
    HKEY hKey;
    char value[128] = { 0 };
    DWORD value_length = sizeof(value);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "SystemSerialNumber", nullptr, nullptr, (LPBYTE)value, &value_length) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (strlen(value) > 0)
                return std::string(value);
        }
        RegCloseKey(hKey);
    }
    return "Not Available";
}

// Helper: Get Motherboard serial via SMBIOS (try registry, fallback to BIOS)
static std::string getMotherboardSerial() {
    HKEY hKey;
    char value[128] = { 0 };
    DWORD value_length = sizeof(value);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "BaseBoardSerialNumber", nullptr, nullptr, (LPBYTE)value, &value_length) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (strlen(value) > 0)
                return std::string(value);
        }
        RegCloseKey(hKey);
    }
    // fallback to bios serial if nothing
    return getBiosSerial();
}

// Helper: Get disk serials using DeviceIoControl
static std::vector<std::string> getDiskSerials() {
    std::vector<std::string> out;
    char driveName[32];
    for (int i = 0; i < 16; ++i) {
        sprintf_s(driveName, "\\\\.\\PhysicalDrive%d", i);
        HANDLE hDevice = CreateFileA(driveName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hDevice == INVALID_HANDLE_VALUE)
            continue;
        STORAGE_PROPERTY_QUERY query = {};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        BYTE buffer[1024] = {};
        DWORD bytesReturned = 0;
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer), &bytesReturned, nullptr)) {
            auto desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
            if (desc->SerialNumberOffset && desc->SerialNumberOffset < bytesReturned) {
                const char* serial = (const char*)buffer + desc->SerialNumberOffset;
                out.push_back(std::string(serial));
            }
            else {
                out.push_back("Not Available");
            }
        }
        else {
            out.push_back("Not Available");
        }
        CloseHandle(hDevice);
    }
    // Remove empty or "Not Available" duplicates
    out.erase(std::remove_if(out.begin(), out.end(), [](const std::string& s) { return s.empty() || s == "Not Available"; }), out.end());
    return out;
}

// Helper: Get MAC addresses
static std::map<std::string, std::string> getNetworkAdapters() {
    std::map<std::string, std::string> result;
    ULONG buflen = 0;
    // First call to get buffer length needed
    if (GetAdaptersAddresses(AF_UNSPEC, 0, 0, nullptr, &buflen) != ERROR_BUFFER_OVERFLOW)
        return result;

    auto buf = (PIP_ADAPTER_ADDRESSES)malloc(buflen);
    if (!buf) return result;

    if (GetAdaptersAddresses(AF_UNSPEC, 0, 0, buf, &buflen) == NO_ERROR) {
        for (auto a = buf; a; a = a->Next) {
            if (a->PhysicalAddressLength == 6) {
                std::ostringstream oss;
                for (ULONG i = 0; i < 6; ++i)
                    oss << std::setfill('0') << std::setw(2) << std::hex << (int)a->PhysicalAddress[i] << (i < 5 ? "-" : "");

                std::string name = "Unknown";
                if (a->FriendlyName) {
                    // WideCharToMultiByte for proper Unicode conversion:
                    int len = WideCharToMultiByte(CP_UTF8, 0, a->FriendlyName, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 1) {
                        std::vector<char> utf8(len);
                        WideCharToMultiByte(CP_UTF8, 0, a->FriendlyName, -1, utf8.data(), len, nullptr, nullptr);
                        name = std::string(utf8.data());
                    }
                }
                result[name] = oss.str();
            }
        }
    }
    free(buf);
    return result;
}

