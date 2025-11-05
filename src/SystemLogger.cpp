#include "SystemLogger.h"

SystemLogger::SystemLogger(size_t maxLogEntries)
    : maxEntries(maxLogEntries) {
}

SystemLogger::~SystemLogger() {
}

void SystemLogger::log(LogLevel level, const String& engineName, const String& message, uint8_t engineColor) {
    LogEntry entry;
    entry.timestamp = time(nullptr);
    entry.level = level;
    entry.engineName = engineName;
    entry.message = message;
    entry.engineColor = engineColor;

    logBuffer.push_back(entry);
    trimLogBuffer();

    // Print to serial
    Serial.print("[");
    Serial.print(entry.getTimeString());
    Serial.print("] [");
    Serial.print(entry.getLevelString());
    Serial.print("] [");
    Serial.print(engineName);
    Serial.print("] ");
    Serial.println(message);

    // Update engine error/warning counts
    EngineHealth* engine = findEngine(engineName);
    if (engine != nullptr) {
        if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
            engine->errorCount++;
            engine->lastError = message;
        } else if (level == LogLevel::WARNING) {
            engine->warningCount++;
        }
    }
}

void SystemLogger::info(const String& engineName, const String& message, uint8_t color) {
    log(LogLevel::INFORMATIONAL, engineName, message, color);
}

void SystemLogger::warn(const String& engineName, const String& message, uint8_t color) {
    log(LogLevel::WARNING, engineName, message, color);
}

void SystemLogger::error(const String& engineName, const String& message, uint8_t color) {
    log(LogLevel::ERROR, engineName, message, color);
}

void SystemLogger::critical(const String& engineName, const String& message, uint8_t color) {
    log(LogLevel::CRITICAL, engineName, message, color);
}

void SystemLogger::flagged(const String& engineName, const String& message, uint8_t color) {
    log(LogLevel::FLAGGED, engineName, message, color);
}

void SystemLogger::success(const String& engineName, const String& message, uint8_t color) {
    log(LogLevel::SUCCESS, engineName, message, color);
}

std::vector<LogEntry> SystemLogger::getLiveLog(size_t count) const {
    std::vector<LogEntry> liveLog;

    // Filter for live-displayable entries
    for (auto it = logBuffer.rbegin(); it != logBuffer.rend(); ++it) {
        if (it->shouldShowOnLive()) {
            liveLog.push_back(*it);
            if (liveLog.size() >= count) break;
        }
    }

    // Reverse to get chronological order (oldest first)
    std::reverse(liveLog.begin(), liveLog.end());

    return liveLog;
}

void SystemLogger::registerEngine(const String& name, uint8_t color) {
    EngineHealth health;
    health.name = name;
    health.operational = false;
    health.responsive = true;
    health.lastHeartbeat = millis();
    health.errorCount = 0;
    health.warningCount = 0;
    health.color = color;

    engineHealth.push_back(health);

    info("System", name + " registered", 1);
}

void SystemLogger::engineHeartbeat(const String& name) {
    EngineHealth* engine = findEngine(name);
    if (engine != nullptr) {
        engine->lastHeartbeat = millis();
        engine->responsive = true;
    }
}

void SystemLogger::setEngineStatus(const String& name, bool operational, const String& errorMsg) {
    EngineHealth* engine = findEngine(name);
    if (engine != nullptr) {
        bool wasOperational = engine->operational;
        engine->operational = operational;

        if (operational && !wasOperational) {
            success(name, "Engine started", engine->color);
        } else if (!operational && wasOperational) {
            error(name, "Engine stopped: " + errorMsg, engine->color);
            engine->lastError = errorMsg;
        }
    }
}

bool SystemLogger::isSystemHealthy() const {
    for (const auto& engine : engineHealth) {
        if (engine.operational) {
            // Check if responsive (heartbeat within last 5 seconds)
            if (millis() - engine.lastHeartbeat > 5000) {
                return false;  // Unresponsive
            }
            if (engine.errorCount > 0) {
                return false;  // Has errors
            }
        }
    }
    return true;
}

String SystemLogger::getHealthSummary() const {
    int operational = 0;
    int total = 0;
    int errors = 0;

    for (const auto& engine : engineHealth) {
        total++;
        if (engine.operational) operational++;
        errors += engine.errorCount;
    }

    return String(operational) + "/" + String(total) + " OK, " + String(errors) + " errs";
}

void SystemLogger::clearLogs() {
    logBuffer.clear();
    info("System", "Logs cleared", 1);
}

String SystemLogger::exportLogsJSON() const {
    String json = "[";
    bool first = true;

    for (const auto& entry : logBuffer) {
        if (!first) json += ",";
        first = false;

        json += "{";
        json += "\"timestamp\":\"" + entry.getTimeString() + "\",";
        json += "\"level\":\"" + entry.getLevelString() + "\",";
        json += "\"engine\":\"" + entry.engineName + "\",";
        json += "\"message\":\"" + entry.message + "\"";
        json += "}";
    }

    json += "]";
    return json;
}

// Private methods

EngineHealth* SystemLogger::findEngine(const String& name) {
    for (auto& engine : engineHealth) {
        if (engine.name == name) {
            return &engine;
        }
    }
    return nullptr;
}

void SystemLogger::trimLogBuffer() {
    while (logBuffer.size() > maxEntries) {
        logBuffer.erase(logBuffer.begin());
    }
}
