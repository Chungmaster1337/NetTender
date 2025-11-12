#include <Arduino.h>
#include <WiFi.h>
#include "version.h"
#include "DisplayManager.h"
#include "EngineManager.h"
#include "SystemLogger.h"
#include "RFScanner.h"
#include "CommandInterface.h"
#include "CommandLedger.h"
#include "config.h"

// Global objects
DisplayManager* display = nullptr;
SystemLogger* logger = nullptr;
EngineManager* engineManager = nullptr;

// Button pins (optional - for future hardware buttons)
#define BUTTON_SELECT 0
#define BUTTON_UP 35
#define BUTTON_DOWN 34
#define BUTTON_BACK 39

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);

    // Print version information
    printVersionInfo();

    // Initialize display
    Serial.println("[Main] Initializing display...");
    display = new DisplayManager(SDA_PIN, SCL_PIN);
    display->begin();

    // Initialize system logger
    Serial.println("[Main] Initializing logger...");
    logger = new SystemLogger(100); // Keep last 100 log entries

    // Initialize engine manager
    Serial.println("[Main] Initializing Sniffy Boi...");
    engineManager = new EngineManager(display, logger);
    engineManager->begin();

    Serial.println("[Main] Sniffy Boi ready!");
    Serial.println("========================================\n");

    // Log system start
    if (logger) {
        logger->success("System", "Boot complete", 1);
        logger->info("System", "Free heap: " + String(ESP.getFreeHeap() / 1024) + "KB", 1);
    }

    // ==================== WiFi Initialization (Monitor Mode Only) ====================
    // NOTE: We do NOT connect to any network for wardriving
    // WiFi is initialized by EngineManager->RFScanner for promiscuous mode
    Serial.println("[Main] WiFi configured for monitor mode (no network connection)");
    if (logger) {
        logger->info("WiFi", "Monitor mode - standalone operation", 1);
    }

    Serial.println("[Main] Setup complete. Entering main loop...");
    Serial.println("========================================\n");
}

void loop() {
    // Run engine manager (executes RF Scanner)
    if (engineManager != nullptr) {
        engineManager->loop();
    }

    // Update display every second
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 1000) {
        if (display != nullptr && logger != nullptr) {
            // Get RFScanner to check command state
            Engine* activeEngine = engineManager->getActiveEngine();
            RFScanner* rfScanner = static_cast<RFScanner*>(activeEngine);

            if (rfScanner != nullptr) {
                CommandInterface* cmdInterface = rfScanner->getCommandInterface();
                if (cmdInterface != nullptr) {
                    CommandLedger* ledger = cmdInterface->getLedger();
                    if (ledger != nullptr && ledger->getState() == CommandState::IDLE) {
                        // Show command menu when in IDLE state
                        display->showCommandMenu();
                    } else {
                        // Show operational view for all other states
                        display->showOperationalView(logger);
                    }
                } else {
                    display->showOperationalView(logger);
                }
            } else {
                display->showOperationalView(logger);
            }
        }
        lastDisplayUpdate = millis();
    }

    // Small delay
    delay(10);
}
