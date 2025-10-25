#pragma once
#include "config_types.h"
#include <string>

class ConfigManager {
public:
    explicit ConfigManager(const std::string& configFilePath);
    const LoadBalancerConfig& getConfig();

private:
    LoadBalancerConfig config;
    std::string configFilePath;
    bool loaded = false;
    const LoadBalancerConfig loadConfig();
    void validateConfig();
};