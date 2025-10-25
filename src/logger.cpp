#include "logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger(LogLevel level, bool toFile, const std::string& filePath)
    : m_Level(level), m_ToFile(toFile)
{
    if (m_ToFile) {
        m_FileStream.open(filePath, std::ios::app);
        if (!m_FileStream.is_open()) {
            std::cerr << "Failed to open log file: " << filePath << std::endl;
            m_ToFile = false;
        }
    }
}
Logger::Logger(const LoggingConfig& config)
    : Logger(
        config.level == "debug" ? LogLevel::Debug :
        config.level == "warn"  ? LogLevel::Warn  :
        config.level == "error" ? LogLevel::Error : LogLevel::Info,
        config.mode == "file",
        config.filePath)
{}
void Logger::logDebug(const std::string& msg) { log(LogLevel::Debug, msg); }
void Logger::logInfo(const std::string& msg)  { log(LogLevel::Info, msg); }
void Logger::logWarn(const std::string& msg)  { log(LogLevel::Warn, msg); }
void Logger::logError(const std::string& msg) { log(LogLevel::Error, msg); }

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < m_Level)
        return; 

    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string formatted = formatMessage(level, msg);

    if (m_ToFile && m_FileStream.is_open()) {
        m_FileStream << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << "[" << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "]";

    switch (level) {
        case LogLevel::Debug: oss << "[DEBUG] "; break;
        case LogLevel::Info:  oss << "[INFO]  "; break;
        case LogLevel::Warn:  oss << "[WARN]  "; break;
        case LogLevel::Error: oss << "[ERROR] "; break;
    }

    oss << msg;
    return oss.str();
}
