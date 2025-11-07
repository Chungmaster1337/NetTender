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
    Serial.println("ESP32 Dual-Engine Platform v1.0");
    Serial.println("========================================");

#if MODE_DUAL_ENGINE
    Serial.println("Mode: DUAL ENGINE (RF Scanner + Network Analyzer)");
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

    // ==================== WiFi Connection ====================
    Serial.println("[Main] Starting WiFi connection...");
    Serial.print("[Main] SSID: ");
    Serial.println(WIFI_SSID);

    // Show WiFi connection starting on OLED
    if (display) {
        display->showWiFiStatus("Connecting...", String("SSID: ") + WIFI_SSID, 10);
    }

    // Configure WiFi
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection with timeout and progress updates
    int attempts = 0;
    const int maxAttempts = 30;  // 30 seconds timeout

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(1000);
        attempts++;

        // Calculate progress (0-80% during connection)
        int progress = (attempts * 80) / maxAttempts;

        // Show progress on OLED
        if (display) {
            String detail = String("SSID: ") + WIFI_SSID;
            display->showWiFiStatus("Connecting...", detail, progress);
        }

        // Serial debug
        Serial.print(".");
        if (attempts % 10 == 0) {
            Serial.print(" [");
            Serial.print(attempts);
            Serial.println("s]");
        }

        // Check specific connection status
        wl_status_t status = WiFi.status();
        if (status == WL_NO_SSID_AVAIL) {
            Serial.println("\n[Main] ERROR: SSID not found!");
            if (logger) logger->error("WiFi", "SSID not found", 5);
            if (display) display->showWiFiStatus("FAILED", "SSID Not Found", 0);
            delay(5000);
            break;
        } else if (status == WL_CONNECT_FAILED) {
            Serial.println("\n[Main] ERROR: Connection failed (wrong password?)");
            if (logger) logger->error("WiFi", "Auth failed", 5);
            if (display) display->showWiFiStatus("FAILED", "Wrong Password?", 0);
            delay(5000);
            break;
        }
    }

    // Connection result
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Main] WiFi Connected!");
        Serial.print("[Main] IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[Main] Signal Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("[Main] Gateway: ");
        Serial.println(WiFi.gatewayIP());

        // Show success on OLED
        if (display) {
            display->showWiFiStatus("CONNECTED", WiFi.localIP().toString(), 100);
        }

        // Log success
        if (logger) {
            logger->success("WiFi", "Connected: " + WiFi.localIP().toString(), 1);
            logger->info("WiFi", "RSSI: " + String(WiFi.RSSI()) + "dBm", 2);
        }

        delay(3000);  // Show connection success for 3 seconds

    } else {
        Serial.println("\n[Main] WiFi connection timeout!");
        Serial.println("[Main] System will continue without network features");

        if (display) {
            display->showWiFiStatus("TIMEOUT", "No Network", 0);
        }

        if (logger) {
            logger->warn("WiFi", "Connection timeout", 3);
        }

        delay(3000);  // Show error for 3 seconds
    }

    Serial.println("[Main] Setup complete. Entering main loop...");
    Serial.println("========================================\n");
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
