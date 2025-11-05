#include <Arduino.h>
#include <WiFi.h>
#include "DisplayManager.h"
#include "EngineManager.h"
#include "SystemLogger.h"
#include "WebInterface.h"
#include "TelnetServer.h"
#include "config.h"

// Global objects
DisplayManager* display = nullptr;
SystemLogger* logger = nullptr;
EngineManager* engineManager = nullptr;
WebServerManager* webServer = nullptr;
TelnetServer* telnetServer = nullptr;

// Configuration flags (can be overridden in config.h)
bool enableWebServer = true;
bool enableTelnet = true;

// Button pins (optional - for future hardware buttons)
#define BUTTON_SELECT 0
#define BUTTON_UP 35
#define BUTTON_DOWN 34
#define BUTTON_BACK 39

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n========================================");
    Serial.println("ESP32 Tri-Engine Platform v1.0");
    Serial.println("========================================");

#if MODE_DUAL_ENGINE
    Serial.println("Mode: DUAL ENGINE (RF Scanner + Network Analyzer)");
#elif MODE_EMERGENCY_ROUTER
    Serial.println("Mode: EMERGENCY ROUTER");
#else
    Serial.println("Mode: UNCONFIGURED");
#endif

    Serial.println("========================================\n");

    // Initialize display
    Serial.println("[Main] Initializing display...");
    display = new DisplayManager(SDA_PIN, SCL_PIN);
    display->begin();

    // Initialize system logger
    Serial.println("[Main] Initializing logger...");
    logger = new SystemLogger(100); // Keep last 100 log entries

    // Initialize engine manager
    Serial.println("[Main] Initializing engine manager...");
    engineManager = new EngineManager(display, logger);
    engineManager->begin();

    // Initialize remote access servers (will start when WiFi connects)
    if (enableWebServer) {
        webServer = new WebServerManager(engineManager, logger, 80);
        Serial.println("[Main] Web server ready (starts when WiFi connected)");
    }

    if (enableTelnet) {
        telnetServer = new TelnetServer(engineManager, 23);
        Serial.println("[Main] Telnet server ready (starts when WiFi connected)");
    }

    Serial.println("[Main] Initialization complete!");
    Serial.println("========================================\n");

    // Log system start
    if (logger) {
        logger->success("System", "Boot complete", 1);
        logger->info("System", "Free heap: " + String(ESP.getFreeHeap() / 1024) + "KB", 1);
    }
}

void loop() {
    // Run engine manager (executes all active engines)
    if (engineManager != nullptr) {
        engineManager->loop();
    }

    // Check WiFi status and manage remote access servers
    static bool serversStarted = false;
    static unsigned long lastWiFiCheck = 0;

    if (millis() - lastWiFiCheck > 5000) {  // Check every 5 seconds
        lastWiFiCheck = millis();

        bool wifiConnected = (WiFi.status() == WL_CONNECTED);

        if (wifiConnected && !serversStarted) {
            Serial.println("[Main] WiFi connected! Starting remote access...");
            Serial.print("[Main] IP Address: ");
            Serial.println(WiFi.localIP());

            if (webServer != nullptr) {
                webServer->begin();
            }

            if (telnetServer != nullptr) {
                telnetServer->begin();
            }

            if (logger) {
                logger->success("Network", "WiFi connected: " + WiFi.localIP().toString(), 1);
            }

            serversStarted = true;

        } else if (!wifiConnected && serversStarted) {
            Serial.println("[Main] WiFi disconnected. Stopping servers...");

            if (webServer != nullptr) {
                webServer->stop();
            }

            if (telnetServer != nullptr) {
                telnetServer->stop();
            }

            if (logger) {
                logger->warn("Network", "WiFi disconnected", 3);
            }

            serversStarted = false;
        }
    }

    // Handle web server requests
    if (webServer != nullptr && webServer->isRunning()) {
        webServer->handleClient();
    }

    // Handle telnet connections
    if (telnetServer != nullptr && telnetServer->isRunning()) {
        telnetServer->loop();
    }

    // Update operational display every second
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 1000) {
        if (display != nullptr && logger != nullptr) {
            display->showOperationalView(logger);
        }
        lastDisplayUpdate = millis();
    }

    // Small delay
    delay(10);
}
