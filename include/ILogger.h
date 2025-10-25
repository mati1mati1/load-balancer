#pragma once
#include <string>

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void logInfo(const std::string&) = 0;
    virtual void logDebug(const std::string&) = 0;
    virtual void logError(const std::string&) = 0;
};
