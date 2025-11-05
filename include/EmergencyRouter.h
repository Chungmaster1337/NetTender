#ifndef EMERGENCY_ROUTER_H
#define EMERGENCY_ROUTER_H

#include "EngineManager.h"
#include "DisplayManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

/**
 * @brief Emergency Router Engine - Phone hotspot to WiFi router
 *
 * Features:
 * - Dual WiFi mode (STA + AP simultaneously)
 * - DHCP server for connected clients
 * - DNS forwarding
 * - NAT/routing between phone and clients
 * - Firewall and packet filtering
 * - QoS and bandwidth management
 * - Connection monitoring
 * - Auto-reconnect to phone hotspot
 * - Web configuration interface
 * - Power management
 */
class EmergencyRouter : public Engine {
public:
    EmergencyRouter(DisplayManager* display);
    ~EmergencyRouter();

    // Engine interface implementation
    bool begin() override;
    void loop() override;
    void stop() override;
    const char* getName() override { return "Emergency Router"; }
    void handleButton(uint8_t button) override;

private:
    DisplayManager* display;

    // Router configuration
    struct RouterConfig {
        // Upstream (phone hotspot) configuration
        String upstreamSSID;
        String upstreamPassword;
        bool upstreamConnected;
        IPAddress upstreamIP;
        IPAddress upstreamGateway;

        // Access Point configuration
        String apSSID;
        String apPassword;
        IPAddress apIP;
        IPAddress apGateway;
        IPAddress apSubnet;

        // DHCP range
        IPAddress dhcpStart;
        IPAddress dhcpEnd;

        // DNS configuration
        IPAddress dnsServer;
        bool dnsForwarding;

        // Limits
        uint8_t maxClients;
    };

    RouterConfig config;
    DNSServer* dnsServer;

    // Connected clients tracking
    struct ClientInfo {
        uint8_t mac[6];
        IPAddress ip;
        String hostname;
        unsigned long connectTime;
        uint64_t bytesRx;
        uint64_t bytesTx;
        uint32_t packetsRx;
        uint32_t packetsTx;
    };

    std::vector<ClientInfo> connectedClients;

    // Statistics
    unsigned long startTime;
    unsigned long lastReconnectAttempt;
    uint32_t reconnectAttempts;
    uint64_t totalBytesRouted;
    uint32_t totalPacketsRouted;

    // Status
    enum class RouterStatus {
        INITIALIZING,
        CONNECTING_UPSTREAM,
        STARTING_AP,
        RUNNING,
        ERROR
    };

    RouterStatus status;
    String statusMessage;

    // Menu
    uint8_t menuPosition;
    bool inSubmenu;

    // Methods
    void showMainMenu();
    void handleMenuNavigation();
    void updateDisplay();

    // Router operations
    bool connectToUpstream();
    bool startAccessPoint();
    bool startDHCPServer();
    bool startDNSServer();
    void stopRouter();

    // Client management
    void scanConnectedClients();
    void updateClientStats();
    ClientInfo* findClient(const uint8_t* mac);

    // NAT/Routing
    void handleRouting();
    void forwardPacket(const uint8_t* packet, size_t length);

    // Auto-reconnect
    void checkUpstreamConnection();
    void attemptReconnect();

    // Configuration
    void loadDefaultConfig();
    void showConfiguration();
    void showClientList();
    void showStatistics();
};

#endif // EMERGENCY_ROUTER_H
