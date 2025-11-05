#ifndef NETWORK_ANALYZER_H
#define NETWORK_ANALYZER_H

#include "EngineManager.h"
#include "DisplayManager.h"
#include "NetworkMonitor.h"
#include <Arduino.h>
#include <WiFi.h>

/**
 * @brief Network Analyzer Engine - MITM/DNS/PiHole capabilities
 *
 * Features:
 * - MITM proxy (ARP spoofing)
 * - DNS server with ad blocking (PiHole-like)
 * - HTTP/HTTPS traffic inspection
 * - Network flow analysis
 * - Protocol detection
 * - Bandwidth monitoring
 * - Device relationship mapping
 * - SSL/TLS analysis
 * - Custom filtering rules
 */
class NetworkAnalyzer : public Engine {
public:
    NetworkAnalyzer(DisplayManager* display);
    ~NetworkAnalyzer();

    // Engine interface implementation
    bool begin() override;
    void loop() override;
    void stop() override;
    const char* getName() override { return "Network Analyzer"; }
    void handleButton(uint8_t button) override;

private:
    DisplayManager* display;
    NetworkMonitor* monitor;

    // Analyzer modes
    enum class AnalyzerMode {
        PASSIVE_MONITOR,
        DNS_MODE,  // Renamed from DNS_SERVER to avoid config.h collision
        MITM_PROXY,
        TRAFFIC_ANALYSIS,
        FLOW_CAPTURE,
        NETWORK_MAP
    };

    AnalyzerMode currentMode;
    uint8_t menuPosition;
    bool inSubmenu;

    // DNS Server configuration
    bool dnsServerActive;
    uint32_t dnsQueriesHandled;
    uint32_t dnsQueriesBlocked;
    std::vector<String> blocklist;

    // MITM configuration
    bool mitmActive;
    uint8_t gatewayMAC[6];
    uint8_t targetDeviceMAC[6];
    IPAddress gatewayIP;
    IPAddress targetIP;

    // Statistics
    unsigned long startTime;
    uint64_t bytesProcessed;
    uint32_t connectionsTracked;
    uint32_t totalDevices;

    // Methods
    void showMainMenu();
    void handleModeSelection();
    void runPassiveMonitor();
    void runDNSServer();
    void runMITMProxy();
    void runTrafficAnalysis();
    void runFlowCapture();
    void runNetworkMap();
    void updateDisplay();

    // DNS methods
    void handleDNSRequest();
    bool isBlocked(const String& domain);
    void loadBlocklist();

    // MITM methods
    void startARPSpoof();
    void stopARPSpoof();
    void sendARPReply(const uint8_t* targetMAC, IPAddress targetIP, IPAddress spoofIP);
};

#endif // NETWORK_ANALYZER_H
