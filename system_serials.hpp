#pragma once
#include <string>
#include <vector>
#include <map>

struct SystemSerials {
    std::string cpuId;
    std::string motherboardSerial;
    std::string biosSerial;
    std::vector<std::string> diskSerials;
    std::vector<std::pair<std::string, std::string>> networkAdapters; // name, MAC
    std::string timestamp;
};

SystemSerials getSystemSerials();