#include "DisplayManager.h"
#include "SystemLogger.h"
#include <WiFi.h>

DisplayManager::DisplayManager(uint8_t sda_pin, uint8_t scl_pin)
    : current_mode(0), device_count(0), packet_count(0), scan_count(0), last_update(0) {
    // Initialize I2C with specific pins for Arduino Nano ESP32
    Wire.begin(sda_pin, scl_pin);

    // Use software I2C for better control on ESP32-S3
    // SW_I2C: (rotation, clock, data, reset)
    display = new U8G2_SSD1306_128X64_NONAME_F_SW_I2C(U8G2_R0, scl_pin, sda_pin, U8X8_PIN_NONE);
}

void DisplayManager::begin() {
    display->begin();
    display->setFont(u8g2_font_6x10_tf);
    display->setFontRefHeightExtendedText();
    display->setDrawColor(1);
    display->setFontPosTop();
    display->setFontDirection(0);

    clear();
    showMessage("ESP32", "Tri-Engine Platform");
    delay(2000);
}

void DisplayManager::update() {
    // Periodic update if needed
    unsigned long now = millis();
    if (now - last_update < 1000) return;
    last_update = now;
}

void DisplayManager::clear() {
    display->clearBuffer();
    display->sendBuffer();
}

void DisplayManager::setMode(uint8_t mode) {
    current_mode = mode;
}

void DisplayManager::showStats(uint32_t devices, uint32_t packets, uint32_t scans) {
    device_count = devices;
    packet_count = packets;
    scan_count = scans;

    display->clearBuffer();
    drawHeader();

    display->setCursor(0, 20);
    display->print("Devices: ");
    display->println(devices);

    display->setCursor(0, 30);
    display->print("Packets: ");
    display->println(packets);

    display->setCursor(0, 40);
    display->print("Scans: ");
    display->println(scans);

    display->sendBuffer();
}

void DisplayManager::showConnectionEvent(const String& mac, const String& event, int8_t rssi) {
    display->clearBuffer();
    drawHeader();

    display->setCursor(0, 20);
    display->print("Event: ");
    display->println(event);

    display->setCursor(0, 30);
    display->print("MAC: ");
    display->println(mac);

    display->setCursor(0, 40);
    display->print("RSSI: ");
    display->print(rssi);
    display->println(" dBm");

    display->sendBuffer();
}

void DisplayManager::addLogEntry(const String& entry) {
    log_buffer.push_back(entry);
    if (log_buffer.size() > MAX_LOG_ENTRIES) {
        log_buffer.erase(log_buffer.begin());
    }
}

void DisplayManager::showScanAlert(const String& scanner_mac, const String& scan_type) {
    display->clearBuffer();
    drawHeader();

    display->setCursor(0, 20);
    display->println("SCAN DETECTED!");

    display->setCursor(0, 30);
    display->print("Type: ");
    display->println(scan_type);

    display->setCursor(0, 40);
    display->print("From: ");
    display->println(scanner_mac);

    display->sendBuffer();
}

void DisplayManager::showNetworkQuality(const String& mac, int8_t rssi, float loss) {
    display->clearBuffer();
    drawHeader();

    display->setCursor(0, 20);
    display->print("MAC: ");
    display->println(mac);

    display->setCursor(0, 30);
    display->print("RSSI: ");
    display->print(rssi);
    display->println(" dBm");

    display->setCursor(0, 40);
    display->print("Loss: ");
    display->print(loss, 1);
    display->println("%");

    display->sendBuffer();
}

// ==================== BOOT MENU ====================

void DisplayManager::showBootMenu(uint8_t selection) {
    display->clearBuffer();

    // Title
    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(15, 0);
    display->print("SELECT ENGINE");

    display->setFont(u8g2_font_6x10_tf);

    // Menu items
    const char* engines[] = {
        "1. RF Scanner",
        "2. Network Analyzer",
        "3. Emergency Router"
    };

    for (uint8_t i = 0; i < 3; i++) {
        uint8_t y = 20 + (i * 14);

        // Highlight selected item
        if (i == selection) {
            display->drawBox(0, y - 2, 128, 12);
            display->setDrawColor(0); // Inverted text
        } else {
            display->setDrawColor(1); // Normal text
        }

        display->setCursor(5, y);
        display->print(engines[i]);

        display->setDrawColor(1); // Reset to normal
    }

    display->sendBuffer();
}

void DisplayManager::showMessage(const String& title, const String& message) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->println(title);

    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 20);
    display->println(message);

    display->sendBuffer();
}

// ==================== RF SCANNER ====================

void DisplayManager::showRFScannerMenu(uint8_t selection) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(5, 0);
    display->print("RF SCANNER");

    display->setFont(u8g2_font_6x10_tf);

    const char* modes[] = {
        "Passive Scan",
        "Deauth Attack",
        "Beacon Spam",
        "Probe Flood",
        "Evil Twin",
        "PMKID Capture",
        "BLE Scan"
    };

    // Show 5 items at a time
    uint8_t startIdx = selection > 2 ? selection - 2 : 0;
    uint8_t endIdx = startIdx + 5;
    if (endIdx > 7) endIdx = 7;

    uint8_t y = 15;
    for (uint8_t i = startIdx; i < endIdx; i++) {
        if (i == selection) {
            display->drawBox(0, y - 2, 128, 11);
            display->setDrawColor(0);
        } else {
            display->setDrawColor(1);
        }

        display->setCursor(3, y);
        display->print(modes[i]);

        display->setDrawColor(1);
        y += 10;
    }

    display->sendBuffer();
}

void DisplayManager::showRFScanStats(uint32_t packets, uint32_t devices, uint8_t channel, unsigned long runtime) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("RF SCAN");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 18);
    display->print("Packets: ");
    display->println(packets);

    display->setCursor(0, 28);
    display->print("Devices: ");
    display->println(devices);

    display->setCursor(0, 38);
    display->print("Channel: ");
    display->println(channel);

    display->setCursor(0, 48);
    display->print("Runtime: ");
    display->print(runtime);
    display->println("s");

    display->sendBuffer();
}

// ==================== NETWORK ANALYZER ====================

void DisplayManager::showNetworkAnalyzerMenu(uint8_t selection) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("NET ANALYZER");

    display->setFont(u8g2_font_6x10_tf);

    const char* modes[] = {
        "Passive Monitor",
        "DNS Server",
        "MITM Proxy",
        "Traffic Analysis",
        "Flow Capture",
        "Network Map"
    };

    uint8_t y = 15;
    for (uint8_t i = 0; i < 6; i++) {
        if (i == selection) {
            display->drawBox(0, y - 2, 128, 11);
            display->setDrawColor(0);
        } else {
            display->setDrawColor(1);
        }

        display->setCursor(3, y);
        display->print(modes[i]);

        display->setDrawColor(1);
        y += 10;
    }

    display->sendBuffer();
}

void DisplayManager::showDNSStats(uint32_t queriesHandled, uint32_t queriesBlocked, unsigned long runtime) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("DNS SERVER");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 18);
    display->print("Queries: ");
    display->println(queriesHandled);

    display->setCursor(0, 28);
    display->print("Blocked: ");
    display->println(queriesBlocked);

    uint32_t blockRate = queriesHandled > 0 ? (queriesBlocked * 100 / queriesHandled) : 0;
    display->setCursor(0, 38);
    display->print("Rate: ");
    display->print(blockRate);
    display->println("%");

    display->setCursor(0, 48);
    display->print("Runtime: ");
    display->print(runtime);
    display->println("s");

    display->sendBuffer();
}

void DisplayManager::showMITMStats(uint64_t bytesProcessed, uint32_t connections, unsigned long runtime) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("MITM PROXY");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 18);
    display->print("Bytes: ");
    if (bytesProcessed > 1024 * 1024) {
        display->print(bytesProcessed / (1024 * 1024));
        display->println(" MB");
    } else if (bytesProcessed > 1024) {
        display->print(bytesProcessed / 1024);
        display->println(" KB");
    } else {
        display->print(bytesProcessed);
        display->println(" B");
    }

    display->setCursor(0, 28);
    display->print("Conns: ");
    display->println(connections);

    display->setCursor(0, 38);
    display->print("Runtime: ");
    display->print(runtime);
    display->println("s");

    display->sendBuffer();
}

// ==================== EMERGENCY ROUTER ====================

void DisplayManager::showEmergencyRouterMenu(uint8_t selection) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("EMERG ROUTER");

    display->setFont(u8g2_font_6x10_tf);

    const char* views[] = {
        "Status",
        "Clients",
        "Statistics"
    };

    uint8_t y = 20;
    for (uint8_t i = 0; i < 3; i++) {
        if (i == selection) {
            display->drawBox(0, y - 2, 128, 11);
            display->setDrawColor(0);
        } else {
            display->setDrawColor(1);
        }

        display->setCursor(5, y);
        display->print(views[i]);

        display->setDrawColor(1);
        y += 14;
    }

    display->sendBuffer();
}

void DisplayManager::showRouterStatus(bool upstreamConnected, size_t clientCount, uint64_t bytesRouted, unsigned long uptime) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("ROUTER STATUS");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 18);
    display->print("Upstream: ");
    display->println(upstreamConnected ? "OK" : "DOWN");

    display->setCursor(0, 28);
    display->print("Clients: ");
    display->println(clientCount);

    display->setCursor(0, 38);
    display->print("Routed: ");
    if (bytesRouted > 1024 * 1024) {
        display->print(bytesRouted / (1024 * 1024));
        display->println(" MB");
    } else if (bytesRouted > 1024) {
        display->print(bytesRouted / 1024);
        display->println(" KB");
    } else {
        display->print(bytesRouted);
        display->println(" B");
    }

    display->setCursor(0, 48);
    display->print("Uptime: ");
    display->print(uptime / 60);
    display->print("m ");
    display->print(uptime % 60);
    display->println("s");

    display->sendBuffer();
}

void DisplayManager::showRouterConfig(const String& ssid, const String& ip, size_t clients, uint8_t maxClients) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("CONFIG");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 18);
    display->print("SSID: ");
    display->println(ssid);

    display->setCursor(0, 28);
    display->print("IP: ");
    display->println(ip);

    display->setCursor(0, 38);
    display->print("Clients: ");
    display->print(clients);
    display->print("/");
    display->println(maxClients);

    display->sendBuffer();
}

void DisplayManager::showRouterClients(size_t count) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("CLIENTS");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 20);
    display->print("Connected: ");
    display->println(count);

    display->setCursor(0, 32);
    display->println("TODO: Show client");
    display->setCursor(0, 42);
    display->println("details with MAC");
    display->setCursor(0, 52);
    display->println("and bandwidth");

    display->sendBuffer();
}

void DisplayManager::showRouterStats(uint64_t bytesRouted, uint32_t packetsRouted, unsigned long uptime) {
    display->clearBuffer();

    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("STATISTICS");

    display->setFont(u8g2_font_6x10_tf);

    display->setCursor(0, 18);
    display->print("Bytes: ");
    if (bytesRouted > 1024 * 1024) {
        display->print(bytesRouted / (1024 * 1024));
        display->println(" MB");
    } else {
        display->print(bytesRouted / 1024);
        display->println(" KB");
    }

    display->setCursor(0, 28);
    display->print("Packets: ");
    display->println(packetsRouted);

    display->setCursor(0, 38);
    display->print("Uptime: ");
    unsigned long hours = uptime / 3600;
    unsigned long mins = (uptime % 3600) / 60;
    display->print(hours);
    display->print("h ");
    display->print(mins);
    display->println("m");

    display->sendBuffer();
}

// ==================== COMPACT DASHBOARD VIEW ====================

void DisplayManager::showOperationalView(SystemLogger* logger) {
    if (logger == nullptr) return;

    display->clearBuffer();

    // Use ultra-small font for maximum information density
    display->setFont(u8g2_font_tom_thumb_4x6_tf);

    uint8_t y = 0;
    const uint8_t lineHeight = 6;

    // Line 1: MAC address
    display->setCursor(0, y);
    display->print("M:");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    display->print(macStr);
    y += lineHeight;

    // Line 2: Uptime and IPv4
    display->setCursor(0, y);
    unsigned long uptimeSec = millis() / 1000;
    uint8_t hours = (uptimeSec / 3600) % 24;
    uint8_t mins = (uptimeSec / 60) % 60;
    uint8_t secs = uptimeSec % 60;
    char uptimeStr[10];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02d%02d%02d", hours, mins, secs);
    display->print("UP:");
    display->print(uptimeStr);

    display->print(" IP:");
    if (WiFi.status() == WL_CONNECTED) {
        display->print(WiFi.localIP().toString());
    } else {
        display->print("---");
    }
    y += lineHeight;

    // Line 3: Gateway and Subnet
    display->setCursor(0, y);
    display->print("GW:");
    if (WiFi.status() == WL_CONNECTED) {
        display->print(WiFi.gatewayIP().toString());
    } else {
        display->print("---");
    }

    display->print(" SN:");
    if (WiFi.status() == WL_CONNECTED) {
        display->print(WiFi.subnetMask().toString());
    } else {
        display->print("---");
    }
    y += lineHeight;

    // Line 4: SSID
    display->setCursor(0, y);
    display->print("SSID:");
    if (WiFi.status() == WL_CONNECTED) {
        String ssid = WiFi.SSID();
        if (ssid.length() > 21) ssid = ssid.substring(0, 21);
        display->print(ssid);
    } else {
        display->print("DISCONNECTED");
    }
    y += lineHeight;

    // Line 5: Log status and path
    display->setCursor(0, y);
    display->print("LOG:");
    // Check if web server or telnet is running
    bool logServerRunning = (WiFi.status() == WL_CONNECTED);
    display->print(logServerRunning ? "UP" : "DOWN");

    display->print(" PATH:");
    if (logServerRunning) {
        display->print(WiFi.localIP().toString());
        display->print(":80");
    } else {
        display->print("---");
    }
    y += lineHeight;

    // Line 6: Engine statuses
    display->setCursor(0, y);
    const auto& engineHealth = logger->getEngineHealth();

    uint8_t engineNum = 1;
    for (const auto& engine : engineHealth) {
        if (engineNum > 2) break;  // Only show first 2 engines

        display->print("E");
        display->print(engineNum);
        display->print(":");

        if (!engine.operational) {
            display->print("DOWN");
        } else if (!engine.responsive || (millis() - engine.lastHeartbeat > 5000)) {
            display->print("STALL");
        } else if (engine.errorCount > 0) {
            display->print("ERR");
        } else {
            display->print("UP");
        }

        if (engineNum == 1) display->print(" ");
        engineNum++;
    }

    // If no engines registered
    if (engineHealth.empty()) {
        display->print("E1:--- E2:---");
    } else if (engineHealth.size() == 1) {
        display->print(" E2:---");
    }
    y += lineHeight;

    // Line 7: Free heap memory
    display->setCursor(0, y);
    display->print("HEAP:");
    display->print(ESP.getFreeHeap() / 1024);
    display->print("KB");
    y += lineHeight;

    // Line 8: Health summary
    display->setCursor(0, y);
    String summary = logger->getHealthSummary();
    if (summary.length() > 31) summary = summary.substring(0, 31);
    display->print(summary);
    y += lineHeight;

    // Line 9: Most recent log entry
    display->setCursor(0, y);
    auto liveLogs = logger->getLiveLog(1);
    if (!liveLogs.empty()) {
        const LogEntry& entry = liveLogs.back();
        String msg = entry.engineName + ":" + entry.message;
        if (msg.length() > 31) msg = msg.substring(0, 31);
        display->print(msg);
    } else {
        display->print("No events");
    }

    display->sendBuffer();
}

void DisplayManager::showBootSequence(const String& component, const String& message, bool success) {
    display->clearBuffer();

    // Boot header
    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 10);
    display->print("BOOT");

    // Component name
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 25);
    display->print(component);

    // Status indicator
    display->setCursor(0, 40);
    if (success) {
        display->print("[OK] ");
    } else {
        display->print("[ERR] ");
    }

    // Message (truncated)
    String msg = message;
    if (msg.length() > 15) msg = msg.substring(0, 15);
    display->print(msg);

    // Progress bar at bottom
    static uint8_t bootProgress = 0;
    bootProgress += 10;
    if (bootProgress > 128) bootProgress = 0;

    display->drawFrame(0, 55, 128, 8);
    display->drawBox(2, 57, bootProgress, 4);

    display->sendBuffer();
}

void DisplayManager::showWiFiStatus(const String& status, const String& detail, int progress) {
    display->clearBuffer();

    // Title
    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 10);
    display->print("WiFi Setup");

    // Status line
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 25);
    display->print("Status: ");
    display->print(status);

    // Detail line (SSID, IP, etc.)
    if (detail.length() > 0) {
        display->setCursor(0, 38);
        String truncated = detail;
        if (truncated.length() > 21) {
            truncated = truncated.substring(0, 21);
        }
        display->print(truncated);
    }

    // Progress bar (if progress >= 0)
    if (progress >= 0) {
        display->drawFrame(0, 50, 128, 10);
        int progressWidth = (progress * 124) / 100;
        display->drawBox(2, 52, progressWidth, 6);

        // Progress percentage
        display->setCursor(110, 52);
        display->print(progress);
        display->print("%");
    }

    display->sendBuffer();
}

// ==================== PRIVATE METHODS ====================

void DisplayManager::drawHeader() {
    display->setFont(u8g2_font_7x13B_tf);
    display->setCursor(0, 0);
    display->print("ESP32");
    display->setFont(u8g2_font_6x10_tf);
    display->drawLine(0, 12, 128, 12);
}

void DisplayManager::drawLogMode() {
    display->clearBuffer();
    drawHeader();

    uint8_t y = 20;
    for (const auto& entry : log_buffer) {
        display->setCursor(0, y);
        display->println(entry);
        y += 10;
    }

    display->sendBuffer();
}

void DisplayManager::drawStatsMode() {
    showStats(device_count, packet_count, scan_count);
}

void DisplayManager::drawAlertMode() {
    display->clearBuffer();
    drawHeader();

    display->setCursor(0, 20);
    display->println("Alert Mode");
    display->setCursor(0, 30);
    display->println(last_event);

    display->sendBuffer();
}
