#ifndef SYSTEM_LOGGER_H
#define SYSTEM_LOGGER_H

#include <Arduino.h>
#include <vector>
#include <time.h>

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    INFORMATIONAL = 0,  // General info, not displayed on live log
    WARNING = 1,        // Warnings, not displayed on live log
    ERROR = 2,          // Errors, not displayed on live log
    CRITICAL = 3,       // Critical issues - SHOWN on live log
    FLAGGED = 4,        // Flagged events - SHOWN on live log
    SUCCESS = 5,        // Success/Complete/Finished - SHOWN on live log
    COMPLETE = 6,       // Alias for SUCCESS
    FINISHED = 7        // Alias for SUCCESS
};

/**
 * @brief Log entry structure
 */
struct LogEntry {
    time_t timestamp;
    LogLevel level;
    String engineName;
    String message;
    uint8_t engineColor;  // 0=red, 1=green, 2=blue, 3=yellow, etc.

    // Helper to get formatted time string
    String getTimeString() const {
        struct tm* timeinfo = localtime(&timestamp);
        char buffer[9];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
        return String(buffer);
    }

    // Helper to get level string
    String getLevelString() const {
        switch(level) {
            case LogLevel::INFORMATIONAL: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            case LogLevel::FLAGGED: return "FLAGGED";
            case LogLevel::SUCCESS:
            case LogLevel::COMPLETE:
            case LogLevel::FINISHED: return "SUCCESS";
            default: return "UNKNOWN";
        }
    }

    // Check if this should be shown on live log
    bool shouldShowOnLive() const {
        return level == LogLevel::CRITICAL ||
               level == LogLevel::FLAGGED ||
               level == LogLevel::SUCCESS ||
               level == LogLevel::COMPLETE ||
               level == LogLevel::FINISHED;
    }
};

/**
 * @brief Engine health status
 */
struct EngineHealth {
    String name;
    bool operational;
    bool responsive;
    unsigned long lastHeartbeat;
    String lastError;
    uint32_t errorCount;
    uint32_t warningCount;
    uint8_t color;
};

/**
 * @brief System-wide logging manager
 */
class SystemLogger {
public:
    SystemLogger(size_t maxLogEntries = 100);
    ~SystemLogger();

    /**
     * @brief Log a message
     */
    void log(LogLevel level, const String& engineName, const String& message, uint8_t engineColor = 1);

    // Convenience methods
    void info(const String& engineName, const String& message, uint8_t color = 1);
    void warn(const String& engineName, const String& message, uint8_t color = 3);
    void error(const String& engineName, const String& message, uint8_t color = 0);
    void critical(const String& engineName, const String& message, uint8_t color = 0);
    void flagged(const String& engineName, const String& message, uint8_t color = 3);
    void success(const String& engineName, const String& message, uint8_t color = 1);

    /**
     * @brief Get recent log entries for live display
     * @param count Number of recent entries to return
     * @return Vector of log entries (filtered for live display)
     */
    std::vector<LogEntry> getLiveLog(size_t count = 4) const;

    /**
     * @brief Get all log entries
     */
    const std::vector<LogEntry>& getAllLogs() const { return logBuffer; }

    /**
     * @brief Register an engine for health monitoring
     */
    void registerEngine(const String& name, uint8_t color);

    /**
     * @brief Update engine heartbeat
     */
    void engineHeartbeat(const String& name);

    /**
     * @brief Set engine operational status
     */
    void setEngineStatus(const String& name, bool operational, const String& errorMsg = "");

    /**
     * @brief Get engine health status
     */
    const std::vector<EngineHealth>& getEngineHealth() const { return engineHealth; }

    /**
     * @brief Check if all engines are healthy
     */
    bool isSystemHealthy() const;

    /**
     * @brief Get system health summary
     */
    String getHealthSummary() const;

    /**
     * @brief Clear all logs
     */
    void clearLogs();

    /**
     * @brief Export logs as JSON
     */
    String exportLogsJSON() const;

private:
    std::vector<LogEntry> logBuffer;
    std::vector<EngineHealth> engineHealth;
    size_t maxEntries;

    // Helper methods
    EngineHealth* findEngine(const String& name);
    void trimLogBuffer();
};

#endif // SYSTEM_LOGGER_H
