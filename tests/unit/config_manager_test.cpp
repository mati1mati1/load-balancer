#include <gtest/gtest.h>
#include <fstream>
#include <stdexcept>
#include "config_manager.h"

using namespace std;

static void writeConfigFile(const string& path, const string& content) {
    ofstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Failed to create test config file: " + path);
    }
    file << content;
    file.close();
}


TEST(ConfigManagerTest, LoadValidConfig) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 8080 },
        "backends": [
            { "host": "127.0.0.1", "port": 9001 },
            { "host": "127.0.0.1", "port": 9002 }
        ],
        "logging": { "level": "info", "mode": "stdout" },
        "reactor": {
            "threads": 4,
            "connectionReadBuffer": 65536,
            "connectionWriteBuffer": 65536
        },
        "shutdown": { "drainSeconds": 10 }
    })";

    string filePath = "temp_valid_config.json";
    writeConfigFile(filePath, jsonContent);

    ConfigManager manager(filePath);
    const LoadBalancerConfig& cfg = manager.getConfig();

    EXPECT_EQ(cfg.listen.host, "0.0.0.0");
    EXPECT_EQ(cfg.listen.port, 8080);

    ASSERT_EQ(cfg.backends.size(), 2);
    EXPECT_EQ(cfg.backends[0].host, "127.0.0.1");
    EXPECT_EQ(cfg.backends[0].port, 9001);
    EXPECT_EQ(cfg.backends[1].host, "127.0.0.1");
    EXPECT_EQ(cfg.backends[1].port, 9002);

    EXPECT_EQ(cfg.logging.level, "info");
    EXPECT_EQ(cfg.logging.mode, "stdout");

    EXPECT_EQ(cfg.reactor.threads, 4);
    EXPECT_EQ(cfg.reactor.connectionReadBuffer, 65536);
    EXPECT_EQ(cfg.reactor.connectionWriteBuffer, 65536);

    EXPECT_EQ(cfg.shutdown.drainSeconds, 10);
}

TEST(ConfigManagerTest, ThrowsIfFileNotFound) {
    ConfigManager manager("non_existent_file.json");
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigManagerTest, ThrowsIfInvalidJson) {
    string invalidJson = R"({ "listen": { "host": "0.0.0.0" } })";  
    string filePath = "temp_invalid_config.json";
    writeConfigFile(filePath, invalidJson);
    ConfigManager manager(filePath);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigManagerTest, ThrowsIfInvalidJsonFormat) {
    string invalidJson = R"({ "listen": { "host": "0.0.0.0" )";  
    string filePath = "temp_invalid_config.json";
    writeConfigFile(filePath, invalidJson);
    ConfigManager manager(filePath);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigValidationTest, ThrowsIfListenPortOutOfRange) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 80000 },
        "backends": [{ "host": "127.0.0.1", "port": 9001 }],
        "logging": { "level": "info", "mode": "stdout" },
        "reactor": { "threads": 2 },
        "shutdown": { "drainSeconds": 5 }
    })";
    string path = "temp_invalid_port.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}
TEST(ConfigValidationTest, ThrowsIfListenHostMissing) {
    string jsonContent = R"({
        "listen": { "host": "", "port": 80000 },
        "backends": [{ "host": "127.0.0.1", "port": 9001 }],
        "logging": { "level": "info", "mode": "stdout" },
        "reactor": { "threads": 2 },
        "shutdown": { "drainSeconds": 5 }
    })";
    string path = "temp_invalid_port.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigValidationTest, ThrowsIfNoBackends) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 8080 },
        "backends": [],
        "logging": { "level": "info", "mode": "stdout" },
        "reactor": { "threads": 2 },
        "shutdown": { "drainSeconds": 5 }
    })";
    string path = "temp_no_backends.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigValidationTest, ThrowsIfBackendHostEmpty) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 8080 },
        "backends": [{ "host": "", "port": 9001 }],
        "logging": { "level": "info", "mode": "stdout" },
        "reactor": { "threads": 2 },
        "shutdown": { "drainSeconds": 5 }
    })";
    string path = "temp_empty_host.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigValidationTest, ThrowsIfBackendPortInvalid) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 8080 },
        "backends": [{ "host": "127.0.0.1", "port": 99999 }],
        "logging": { "level": "info", "mode": "stdout" },
        "reactor": { "threads": 2 },
        "shutdown": { "drainSeconds": 5 }
    })";
    string path = "temp_invalid_backend_port.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigValidationTest, ThrowsIfLoggingLevelInvalid) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 8080 },
        "backends": [{ "host": "127.0.0.1", "port": 9001 }],
        "logging": { "level": "verbose", "mode": "stdout" },
        "reactor": { "threads": 2 },
        "shutdown": { "drainSeconds": 5 }
    })";
    string path = "temp_invalid_logging.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_THROW({
        manager.getConfig();
    }, runtime_error);
}

TEST(ConfigValidationTest, PassesWithValidConfig) {
    string jsonContent = R"({
        "listen": { "host": "0.0.0.0", "port": 8080 },
        "backends": [{ "host": "127.0.0.1", "port": 9001 }],
        "logging": { "level": "debug", "mode": "stdout" },
        "reactor": { "threads": 4 },
        "shutdown": { "drainSeconds": 10 }
    })";
    string path = "temp_valid_config.json";
    writeConfigFile(path, jsonContent);
    ConfigManager manager(path);
    EXPECT_NO_THROW({
        manager.getConfig();
    });
}
