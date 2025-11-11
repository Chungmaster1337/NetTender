#include "DisplayManager.h"
#include "SystemLogger.h"
#include <WiFi.h>
#include <esp_wifi.h>

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

    // Splash screen: "Sniffy Boi..."
    display->clearBuffer();
    display->setFont(u8g2_font_logisoso20_tf);  // Large 20px font

    // Line 1: "Sniffy"
    const char* line1 = "Sniffy";
    int16_t line1Width = display->getStrWidth(line1);
    int16_t x1 = (128 - line1Width) / 2;
    display->setCursor(x1, 15);
    display->print(line1);

    // Line 2: "Boi..."
    const char* line2 = "Boi...";
    int16_t line2Width = display->getStrWidth(line2);
    int16_t x2 = (128 - line2Width) / 2;
    display->setCursor(x2, 42);
    display->print(line2);

    display->sendBuffer();
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

    // Menu items - only show engines that exist
    const char* engines[] = {
        "1. RF Scanner",
        "2. Network Analyzer"
    };

    for (uint8_t i = 0; i < 2; i++) {
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

    // Line 2: Uptime and Mode
    display->setCursor(0, y);
    unsigned long uptimeSec = millis() / 1000;
    uint8_t hours = (uptimeSec / 3600) % 24;
    uint8_t mins = (uptimeSec / 60) % 60;
    uint8_t secs = uptimeSec % 60;
    char uptimeStr[10];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02d%02d%02d", hours, mins, secs);
    display->print("UP:");
    display->print(uptimeStr);

    // Show wardriving mode instead of IP
    display->print(" MODE:WARDRIVE");
    y += lineHeight;

    // Line 3: Memory and Channel
    display->setCursor(0, y);
    display->print("MEM:");
    display->print(ESP.getFreeHeap() / 1024);
    display->print("KB");

    display->print(" CH:");
    uint8_t channel;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&channel, &second);
    display->print(channel);
    y += lineHeight;

    // Line 4: Attack capabilities
    display->setCursor(0, y);
    display->print("ATTACKS: HS|PMKID|DEAUTH");
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

// ==================== Interactive Command Interface Displays ====================

void DisplayManager::showCommandExecuting(const String& command, int timeout_remaining, int progress_percent) {
    display->clearBuffer();

    // Title (large font)
    display->setFont(u8g2_font_9x15_tf);
    const char* title = command.c_str();
    int16_t titleWidth = display->getStrWidth(title);
    int16_t titleX = (128 - titleWidth) / 2;
    display->setCursor(titleX, 2);
    display->print(title);

    // Line under title
    display->drawLine(0, 18, 128, 18);

    // Session timeout countdown (top right)
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 22);
    display->print("Timeout: ");
    display->print(timeout_remaining);
    display->print("s");

    // Progress bar for operation (0-100%)
    display->setCursor(0, 35);
    display->print("Progress: ");
    display->print(progress_percent);
    display->print("%");

    // Visual progress bar
    int barWidth = 120;
    int barX = 4;
    int barY = 48;
    int barHeight = 12;

    // Draw progress bar background
    display->drawFrame(barX, barY, barWidth, barHeight);

    // Fill progress
    int fillWidth = (barWidth - 2) * progress_percent / 100;
    if (fillWidth > 0) {
        display->drawBox(barX + 1, barY + 1, fillWidth, barHeight - 2);
    }

    display->sendBuffer();
}

void DisplayManager::showCommandResult(const String& command, bool success, const String& message, int items_found) {
    display->clearBuffer();

    // Success/Failure indicator (large)
    display->setFont(u8g2_font_9x15_tf);
    const char* status = success ? "SUCCESS" : "FAILED";
    int16_t statusWidth = display->getStrWidth(status);
    int16_t statusX = (128 - statusWidth) / 2;
    display->setCursor(statusX, 2);
    display->print(status);

    // Command name
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 20);
    display->print("Cmd: ");
    display->print(command);

    // Message
    display->setCursor(0, 32);
    display->print(message);

    // Items found (if applicable)
    if (items_found > 0) {
        display->setCursor(0, 44);
        display->print("Found: ");
        display->print(items_found);
        display->print(" items");
    }

    display->sendBuffer();
}

void DisplayManager::showConfigComparison(const String& setting, const String& old_value, const String& new_value, int countdown) {
    display->clearBuffer();

    // Title
    display->setFont(u8g2_font_9x15_tf);
    const char* title = setting.c_str();
    int16_t titleWidth = display->getStrWidth(title);
    int16_t titleX = (128 - titleWidth) / 2;
    display->setCursor(titleX, 2);
    display->print(title);

    display->drawLine(0, 18, 128, 18);

    // OLD value
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 24);
    display->print("OLD: ");
    display->print(old_value);

    // NEW value (highlighted)
    display->setCursor(0, 38);
    display->print("NEW: ");
    display->setFont(u8g2_font_9x15_tf);
    display->print(new_value);

    // Countdown
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 55);
    display->print("Returning in ");
    display->print(countdown);
    display->print("s");

    display->sendBuffer();
}

void DisplayManager::showCooldownResults(const String& title, const std::vector<String>& results, int countdown) {
    display->clearBuffer();

    // Title
    display->setFont(u8g2_font_7x13_tf);
    display->setCursor(0, 0);
    display->print(title);

    display->drawLine(0, 12, 128, 12);

    // Results (up to 3 lines)
    display->setFont(u8g2_font_6x10_tf);
    int y = 16;
    int max_lines = 3;
    for (int i = 0; i < results.size() && i < max_lines; i++) {
        display->setCursor(0, y);
        display->print(results[i]);
        y += 10;
    }

    if (results.size() > max_lines) {
        display->setCursor(0, y);
        display->print("... ");
        display->print(results.size() - max_lines);
        display->print(" more");
    }

    // Cooldown countdown at bottom
    display->drawLine(0, 50, 128, 50);
    display->setCursor(0, 54);
    display->print("Cooldown: ");
    display->print(countdown);
    display->print("s");

    display->sendBuffer();
}

void DisplayManager::showErrorMessage(const String& error, const String& detail, int countdown) {
    display->clearBuffer();

    // ERROR header (large, centered)
    display->setFont(u8g2_font_9x15_tf);
    const char* errText = "ERROR";
    int16_t errWidth = display->getStrWidth(errText);
    int16_t errX = (128 - errWidth) / 2;
    display->setCursor(errX, 2);
    display->print(errText);

    display->drawLine(0, 18, 128, 18);

    // Error type
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 24);
    display->print(error);

    // Detail (word wrap if needed)
    display->setCursor(0, 36);
    if (detail.length() > 21) {
        display->print(detail.substring(0, 21));
        display->setCursor(0, 46);
        display->print(detail.substring(21));
    } else {
        display->print(detail);
    }

    // Countdown
    display->drawLine(0, 50, 128, 50);
    display->setCursor(0, 54);
    display->print("Reset in ");
    display->print(countdown);
    display->print("s");

    display->sendBuffer();
}

void DisplayManager::showAwaitingValue(const String& setting, const String& current_value, const String& valid_range) {
    display->clearBuffer();

    // Title
    display->setFont(u8g2_font_9x15_tf);
    const char* title = setting.c_str();
    int16_t titleWidth = display->getStrWidth(title);
    int16_t titleX = (128 - titleWidth) / 2;
    display->setCursor(titleX, 2);
    display->print(title);

    display->drawLine(0, 18, 128, 18);

    // Current value
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 24);
    display->print("Current: ");
    display->print(current_value);

    // Valid range
    display->setCursor(0, 36);
    display->print("Valid: ");
    display->print(valid_range);

    // Instructions
    display->setCursor(0, 50);
    display->print("Send new value");

    display->sendBuffer();
}

void DisplayManager::showSessionLocked(const uint8_t* authorized_mac) {
    display->clearBuffer();

    // Warning header
    display->setFont(u8g2_font_9x15_tf);
    const char* warning = "LOCKED";
    int16_t warnWidth = display->getStrWidth(warning);
    int16_t warnX = (128 - warnWidth) / 2;
    display->setCursor(warnX, 2);
    display->print(warning);

    display->drawLine(0, 18, 128, 18);

    // Message
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 24);
    display->print("Session active");

    display->setCursor(0, 36);
    display->print("Authorized MAC:");

    // MAC address
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             authorized_mac[0], authorized_mac[1], authorized_mac[2],
             authorized_mac[3], authorized_mac[4], authorized_mac[5]);
    display->setCursor(0, 48);
    display->print(mac_str);

    display->sendBuffer();
}

void DisplayManager::showCommandMenu() {
    display->clearBuffer();

    // Header with blinking prompt
    display->setFont(u8g2_font_6x10_tf);
    display->setCursor(0, 0);
    display->print("SNIFFY:COMMAND");

    // Show blinking cursor effect (blink every 500ms)
    if ((millis() / 500) % 2 == 0) {
        display->print("_");
    }

    display->drawLine(0, 12, 128, 12);

    // Available commands (line by line)
    int y = 16;
    const int lineHeight = 9;  // Reduced from 10 to fit more

    display->setCursor(0, y);
    display->print("SCAN - Scan APs");
    y += lineHeight;

    display->setCursor(0, y);
    display->print("ATTACK <MAC> - Deauth");
    y += lineHeight;

    display->setCursor(0, y);
    display->print("PMKID <MAC> - PMKID");
    y += lineHeight;

    display->setCursor(0, y);
    display->print("BEACON [CH] - Flood");
    y += lineHeight;

    display->setCursor(0, y);
    display->print("CHANNEL [N] - Ch cfg");
    y += lineHeight;

    display->setCursor(0, y);
    display->print("HOPPING [ON/OFF]");

    display->sendBuffer();
}
