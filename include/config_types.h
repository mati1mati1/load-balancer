#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace std;

struct ListenConfig {
    std::string host;
    uint16_t port;
    int backlog = 128;
};

struct BackendConfig {
    std::string host;
    uint16_t port;
};

struct LoggingConfig {
    std::string level;  
    std::string mode;  
    std::string filePath;
};

struct ReactorConfig {
    int threads = 0;               
    size_t connectionReadBuffer = 65536;
    size_t connectionWriteBuffer = 65536;
};

struct ShutdownConfig {
    int drainSeconds = 10;
};

struct LoadBalancerConfig {
    ListenConfig listen;
    std::vector<BackendConfig> backends;
    LoggingConfig logging;
    ReactorConfig reactor;
    ShutdownConfig shutdown;
};

inline void from_json(const json& j, ListenConfig& c) {
    j.at("host").get_to(c.host);

    int port;
    j.at("port").get_to(port);
    if (port < 1 || port > 65535)
        throw runtime_error("Configuration error: Listen port must be between 1 and 65535.");

    c.port = static_cast<uint16_t>(port);

    if (j.contains("backlog")) j.at("backlog").get_to(c.backlog);
}

inline void from_json(const json& j, BackendConfig& c) {
    j.at("host").get_to(c.host);

    int port;
    j.at("port").get_to(port);
    if (port < 1 || port > 65535)
        throw runtime_error("Configuration error: Backend port must be between 1 and 65535.");

    c.port = static_cast<uint16_t>(port);
}

inline void from_json(const json& j, LoggingConfig& c) {
    j.at("level").get_to(c.level);
    j.at("mode").get_to(c.mode);
    if (j.contains("filePath"))
        j.at("filePath").get_to(c.filePath);
    else
        c.filePath = "";
    }

inline void from_json(const json& j, ReactorConfig& c) {
    if (j.contains("threads")) j.at("threads").get_to(c.threads);
    if (j.contains("connectionReadBuffer")) j.at("connectionReadBuffer").get_to(c.connectionReadBuffer);
    if (j.contains("connectionWriteBuffer")) j.at("connectionWriteBuffer").get_to(c.connectionWriteBuffer);
}

inline void from_json(const json& j, ShutdownConfig& c) {
    if (j.contains("drainSeconds")) j.at("drainSeconds").get_to(c.drainSeconds);
}

inline void from_json(const json& j, LoadBalancerConfig& c) {
    j.at("listen").get_to(c.listen);
    j.at("backends").get_to(c.backends);
    if (j.contains("logging"))  j.at("logging").get_to(c.logging);
    if (j.contains("reactor"))  j.at("reactor").get_to(c.reactor);
    if (j.contains("shutdown")) j.at("shutdown").get_to(c.shutdown);
}
