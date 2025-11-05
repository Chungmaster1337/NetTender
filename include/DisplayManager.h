#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <vector>

// Forward declaration
class SystemLogger;

class DisplayManager {
public:
    DisplayManager(uint8_t sda_pin = 21, uint8_t scl_pin = 22);
    void begin();
    void update();

    // Display modes
    void showStats(uint32_t devices, uint32_t packets, uint32_t scans);
    void showConnectionEvent(const String& mac, const String& event, int8_t rssi);
    void addLogEntry(const String& entry);
    void showScanAlert(const String& scanner_mac, const String& scan_type);
    void showNetworkQuality(const String& mac, int8_t rssi, float loss);

    // Boot menu and engine-specific displays
    void showBootMenu(uint8_t selection);
    void showMessage(const String& title, const String& message);

    // RF Scanner displays
    void showRFScannerMenu(uint8_t selection);
    void showRFScanStats(uint32_t packets, uint32_t devices, uint8_t channel, unsigned long runtime);

    // Network Analyzer displays
    void showNetworkAnalyzerMenu(uint8_t selection);
    void showDNSStats(uint32_t queriesHandled, uint32_t queriesBlocked, unsigned long runtime);
    void showMITMStats(uint64_t bytesProcessed, uint32_t connections, unsigned long runtime);

    // Emergency Router displays
    void showEmergencyRouterMenu(uint8_t selection);
    void showRouterStatus(bool upstreamConnected, size_t clientCount, uint64_t bytesRouted, unsigned long uptime);
    void showRouterConfig(const String& ssid, const String& ip, size_t clients, uint8_t maxClients);
    void showRouterClients(size_t count);
    void showRouterStats(uint64_t bytesRouted, uint32_t packetsRouted, unsigned long uptime);

    // Split-screen operational display
    void showOperationalView(SystemLogger* logger);
    void showBootSequence(const String& component, const String& message, bool success);

    // WiFi connection status display
    void showWiFiStatus(const String& status, const String& detail = "", int progress = -1);

    // Screen management
    void clear();
    void setMode(uint8_t mode);  // 0=log, 1=stats, 2=alerts

private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C* display;
    uint8_t current_mode;
    std::vector<String> log_buffer;
    const uint8_t MAX_LOG_ENTRIES = 5;

    void drawHeader();
    void drawLogMode();
    void drawStatsMode();
    void drawAlertMode();

    uint32_t device_count;
    uint32_t packet_count;
    uint32_t scan_count;
    String last_event;
    unsigned long last_update;
};

#endif
