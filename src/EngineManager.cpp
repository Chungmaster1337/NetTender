#include "EngineManager.h"
#include "DisplayManager.h"
#include "SystemLogger.h"
#include "RFScanner.h"
#include "NetworkAnalyzer.h"
#include "EmergencyRouter.h"

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
    if (logger) logger->info("System", "Running POST", 1);
    showBootStatus("POST", "Starting tests", true);

    // Test 1: Display
    showBootStatus("Display", "Checking", true);
    if (display == nullptr) {
        if (logger) logger->error("System", "Display not initialized", 0);
        return false;
    }
    if (logger) logger->success("System", "Display OK", 1);
    delay(300);

    // Test 2: WiFi hardware
    showBootStatus("WiFi", "Checking hardware", true);
    WiFi.mode(WIFI_STA);
    if (WiFi.getMode() != WIFI_STA) {
        if (logger) logger->error("System", "WiFi hardware fault", 0);
        return false;
    }
    if (logger) logger->success("System", "WiFi OK", 1);
    delay(300);

    // Test 3: Memory
    showBootStatus("Memory", "Checking", true);
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 50000) {  // Less than 50KB is critical
        if (logger) logger->critical("System", "Low memory: " + String(freeHeap), 0);
        return false;
    }
    if (logger) logger->success("System", "Memory OK: " + String(freeHeap / 1024) + "KB", 1);
    delay(300);

    showBootStatus("POST", "Complete", true);
    if (logger) logger->success("System", "POST passed", 1);

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
    if (name == "Network Analyzer") return EngineType::NETWORK_ANALYZER;
    if (name == "Emergency Router") return EngineType::EMERGENCY_ROUTER;

    return EngineType::NONE;
}

// ==================== PRIVATE METHODS ====================

void EngineManager::autoStart() {
    if (logger) logger->info("System", "Auto-starting engines", 1);

#if MODE_DUAL_ENGINE
    startDualEngineMode();
#elif MODE_EMERGENCY_ROUTER
    startEmergencyRouterMode();
#else
    if (logger) logger->error("System", "No operational mode configured", 0);
#endif
}

void EngineManager::startDualEngineMode() {
    if (logger) logger->info("System", "Starting DUAL ENGINE mode", 1);
    showBootStatus("Mode", "Dual Engine", true);

    // Start RF Scanner
    if (startEngine(EngineType::RF_SCANNER)) {
        if (logger) logger->success("System", "RF Scanner started", 2);
    } else {
        if (logger) logger->error("System", "RF Scanner failed to start", 0);
    }

    delay(500);

    // Start Network Analyzer
    if (startEngine(EngineType::NETWORK_ANALYZER)) {
        if (logger) logger->success("System", "Network Analyzer started", 1);
    } else {
        if (logger) logger->error("System", "Network Analyzer failed to start", 0);
    }

    if (logger) {
        logger->success("System", "Dual engine mode operational", 1);
    }
}

void EngineManager::startEmergencyRouterMode() {
    if (logger) logger->info("System", "Starting EMERGENCY ROUTER mode", 1);
    showBootStatus("Mode", "Emergency Router", true);

    if (startEngine(EngineType::EMERGENCY_ROUTER)) {
        if (logger) logger->success("System", "Emergency Router started", 3);
    } else {
        if (logger) logger->critical("System", "Emergency Router failed to start", 0);
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
            return new RFScanner(display);

        case EngineType::NETWORK_ANALYZER:
            return new NetworkAnalyzer(display);

        case EngineType::EMERGENCY_ROUTER:
            return new EmergencyRouter(display);

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
        if (type == EngineType::EMERGENCY_ROUTER) color = 3; // Yellow

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
