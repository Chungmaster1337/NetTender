#include "EngineManager.h"
#include "DisplayManager.h"
#include "SystemLogger.h"
#include "RFScanner.h"
#include <WiFi.h>

EngineManager::EngineManager(DisplayManager* display, SystemLogger* logger)
    : display(display), logger(logger), lastHealthCheck(0) {
}

EngineManager::~EngineManager() {
    stopAllEngines();
}

void EngineManager::begin() {
    if (logger) logger->info("System", "EngineManager initializing", 1);

    // Perform POST if enabled
#if BOOT_HEALTH_CHECK
    if (!performPOST()) {
        if (logger) logger->critical("System", "POST failed", 0);
        showBootStatus("POST", "Failed", false);
        return;
    }
#endif

    // Auto-start based on compile-time configuration
#if AUTO_START_ON_BOOT
    autoStart();
#else
    if (logger) logger->info("System", "Manual start mode - waiting for input", 1);
#endif
}

void EngineManager::loop() {
    // Run all active engines
    for (auto* engine : activeEngines) {
        if (engine != nullptr) {
            engine->loop();
        }
    }

    // Periodic health check (every 5 seconds)
    if (millis() - lastHealthCheck > 5000) {
        checkEngineHealth();
        lastHealthCheck = millis();
    }
}

bool EngineManager::performPOST() {
    if (logger) logger->info("System", "Boot check", 1);
    showBootStatus("Init", "Starting", true);

    // Test 1: Display
    showBootStatus("Display", "OK", true);
    if (display == nullptr) {
        if (logger) logger->error("System", "Display fail", 0);
        return false;
    }
    if (logger) logger->success("System", "Display ready", 1);
    delay(200);

    // Test 2: WiFi hardware
    showBootStatus("WiFi", "OK", true);
    WiFi.mode(WIFI_STA);
    if (WiFi.getMode() != WIFI_STA) {
        if (logger) logger->error("System", "WiFi fail", 0);
        return false;
    }
    if (logger) logger->success("System", "WiFi ready", 1);
    delay(200);

    // Test 3: Memory
    showBootStatus("Memory", "OK", true);
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 50000) {  // Less than 50KB is critical
        if (logger) logger->critical("System", "Low mem: " + String(freeHeap / 1024) + "KB", 0);
        return false;
    }
    if (logger) logger->success("System", "Free: " + String(freeHeap / 1024) + "KB", 1);
    delay(200);

    showBootStatus("Init", "Ready", true);
    if (logger) logger->success("System", "Boot complete", 1);

    return true;
}

bool EngineManager::isSystemHealthy() {
    if (logger) {
        return logger->isSystemHealthy();
    }

    // Fallback check
    for (auto* engine : activeEngines) {
        if (engine != nullptr && !engine->isHealthy()) {
            return false;
        }
    }
    return true;
}

EngineType EngineManager::getCurrentEngine() const {
    if (activeEngines.empty()) return EngineType::NONE;

    // Return first active engine (for compatibility)
    // In dual-engine mode, this will be RF_SCANNER
    Engine* first = activeEngines[0];

    // Use name comparison instead of dynamic_cast (RTTI disabled)
    String name = String(first->getName());
    if (name == "RF Scanner") return EngineType::RF_SCANNER;

    return EngineType::NONE;
}

// ==================== PRIVATE METHODS ====================

void EngineManager::autoStart() {
    if (logger) logger->info("System", "Auto-starting engines", 1);

#if MODE_DUAL_ENGINE
    startDualEngineMode();
#else
    if (logger) logger->error("System", "No operational mode configured", 0);
#endif
}

void EngineManager::startDualEngineMode() {
    if (logger) logger->info("System", "Starting RF Scanner", 1);
    showBootStatus("Engine", "Loading", true);

    // Start RF Scanner (single engine mode for wardriving)
    if (startEngine(EngineType::RF_SCANNER)) {
        if (logger) logger->success("System", "RF Scanner up", 2);
    } else {
        if (logger) logger->error("System", "RF Scanner fail", 0);
    }

    if (logger) {
        logger->success("System", "Sniffy ready!", 1);
    }
}

void EngineManager::showBootStatus(const String& component, const String& message, bool success) {
#if SHOW_BOOT_STATUS
    if (display != nullptr) {
        display->showBootSequence(component, message, success);
        delay(200);
    }
#endif

    Serial.print("[BOOT] ");
    Serial.print(component);
    Serial.print(": ");
    Serial.println(message);
}

Engine* EngineManager::createEngine(EngineType type) {
    switch (type) {
        case EngineType::RF_SCANNER:
            return new RFScanner(display, logger);

        default:
            if (logger) logger->error("System", "Unknown engine type", 0);
            return nullptr;
    }
}

bool EngineManager::startEngine(EngineType type) {
    Engine* engine = createEngine(type);
    if (engine == nullptr) {
        return false;
    }

    // Register with logger
    if (logger) {
        uint8_t color = 1; // Default green
        if (type == EngineType::RF_SCANNER) color = 2; // Blue
        if (type == EngineType::NETWORK_ANALYZER) color = 1; // Green

        logger->registerEngine(engine->getName(), color);
    }

    // Initialize engine
    bool success = engine->begin();

    if (success) {
        activeEngines.push_back(engine);
        if (logger) {
            logger->setEngineStatus(engine->getName(), true);
        }
        return true;
    } else {
        if (logger) {
            logger->setEngineStatus(engine->getName(), false, "Initialization failed");
        }
        delete engine;
        return false;
    }
}

bool EngineManager::loadEngine(EngineType type) {
    // Manual engine load (for web/telnet control)
    return startEngine(type);
}

void EngineManager::stopAllEngines() {
    for (auto* engine : activeEngines) {
        if (engine != nullptr) {
            if (logger) {
                logger->setEngineStatus(engine->getName(), false, "Stopped");
            }
            engine->stop();
            delete engine;
        }
    }
    activeEngines.clear();
}

void EngineManager::returnToMenu() {
    if (logger) logger->info("System", "Stopping all engines", 1);
    stopAllEngines();
}

void EngineManager::checkEngineHealth() {
    for (auto* engine : activeEngines) {
        if (engine != nullptr && logger != nullptr) {
            // Send heartbeat
            logger->engineHeartbeat(engine->getName());

            // Check if healthy
            if (!engine->isHealthy()) {
                logger->error(engine->getName(), "Health check failed", 0);
            }
        }
    }
}
