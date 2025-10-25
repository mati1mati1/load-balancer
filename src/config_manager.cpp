#include "config_manager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
using json = nlohmann::json;
using namespace std;

ConfigManager::ConfigManager(const std::string& configFilePath) {
    this->configFilePath = configFilePath;
}


const LoadBalancerConfig ConfigManager::loadConfig() {
    std::ifstream file(this->configFilePath);
    if (!file.is_open()) {
        throw runtime_error("Could not open config file: " + this->configFilePath);
    }

    try {
        json j;
        file >> j;
        this->config = j.get<LoadBalancerConfig>();
    } catch (const json::exception& e) {
        throw runtime_error("Failed to parse config file: " + string(e.what()));
    }

    validateConfig();
    this->loaded = true;   // mark as loaded
    return config;
}

const LoadBalancerConfig& ConfigManager::getConfig() {   
    if (!this->loaded) {
        loadConfig();
    }
    return this->config;
}


void ConfigManager::validateConfig() {
    if (config.backends.empty()) {
        throw runtime_error("Configuration error: At least one backend must be specified.");
    }
    if (config.listen.port == 0 || config.listen.port > 65535) {
        throw runtime_error("Configuration error: Listen port must be between 1 and 65535.");
    }
    if (config.listen.host.empty()) {
        throw runtime_error("Configuration error: Listen host cannot be empty.");
    }
    for (const auto& backend : config.backends) {
        if (backend.port == 0 || backend.port > 65535) {
            throw runtime_error("Configuration error: Backend port must be between 1 and 65535.");
        }
        if (backend.host.empty()) {
            throw runtime_error("Configuration error: Backend host cannot be empty.");
        }
    }
    if (config.reactor.threads < 0) {
        throw runtime_error("Configuration error: Reactor threads cannot be negative.");
    }
    if (config.logging.level != "debug" &&
        config.logging.level != "info" &&
        config.logging.level != "warn" &&
        config.logging.level != "error") {
        throw runtime_error("Configuration error: Invalid logging level specified.");
    }

}