#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
#include "interfaces/ILogger.h"
#include "config_types.h"
enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error
};

class Logger : public ILogger{
public:
    explicit Logger(LogLevel level = LogLevel::Info,
                    bool toFile = false,
                    const std::string& filePath = "");
    Logger(const LoggingConfig& config);
    void logDebug(const std::string& msg);
    void logError(const std::string& msg);
    void logWarn(const std::string& msg);
    void logInfo(const std::string& msg);

private:
    void log(LogLevel level, const std::string& msg);
    std::string formatMessage(LogLevel level, const std::string& msg);

    LogLevel m_Level;
    bool m_ToFile;
    std::ofstream m_FileStream;
    std::mutex m_Mutex;
};
