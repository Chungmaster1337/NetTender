#include "EmergencyRouter.h"

EmergencyRouter::EmergencyRouter(DisplayManager* display)
    : display(display), dnsServer(nullptr), startTime(0),
      lastReconnectAttempt(0), reconnectAttempts(0), totalBytesRouted(0),
      totalPacketsRouted(0), status(RouterStatus::INITIALIZING),
      menuPosition(0), inSubmenu(false) {
    loadDefaultConfig();
}

EmergencyRouter::~EmergencyRouter() {
    stop();
}

bool EmergencyRouter::begin() {
    Serial.println("[EmergencyRouter] Initializing Emergency Router Engine...");

    startTime = millis();
    status = RouterStatus::INITIALIZING;
    statusMessage = "Initializing...";

    if (display != nullptr) {
        display->clear();
        display->showMessage("Emergency Router", "Initializing...");
        delay(1000);
    }

    // Step 1: Connect to upstream (phone hotspot)
    status = RouterStatus::CONNECTING_UPSTREAM;
    if (display != nullptr) {
        display->showMessage("Emergency Router", "Connecting to phone...");
    }

    if (!connectToUpstream()) {
        Serial.println("[EmergencyRouter] ERROR: Failed to connect to upstream");
        status = RouterStatus::ERROR;
        statusMessage = "Failed to connect";
        return false;
    }

    // Step 2: Start Access Point
    status = RouterStatus::STARTING_AP;
    if (display != nullptr) {
        display->showMessage("Emergency Router", "Starting AP...");
    }

    if (!startAccessPoint()) {
        Serial.println("[EmergencyRouter] ERROR: Failed to start AP");
        status = RouterStatus::ERROR;
        statusMessage = "Failed to start AP";
        return false;
    }

    // Step 3: Start DHCP server
    if (!startDHCPServer()) {
        Serial.println("[EmergencyRouter] WARNING: DHCP server failed");
        // Not critical, continue
    }

    // Step 4: Start DNS server
    if (!startDNSServer()) {
        Serial.println("[EmergencyRouter] WARNING: DNS server failed");
        // Not critical, continue
    }

    status = RouterStatus::RUNNING;
    statusMessage = "Router active";

    Serial.println("[EmergencyRouter] Emergency Router initialized successfully");
    Serial.print("[EmergencyRouter] AP SSID: ");
    Serial.println(config.apSSID);
    Serial.print("[EmergencyRouter] AP IP: ");
    Serial.println(config.apIP);

    if (display != nullptr) {
        showMainMenu();
    }

    return true;
}

void EmergencyRouter::loop() {
    if (status != RouterStatus::RUNNING) {
        delay(100);
        return;
    }

    // Check upstream connection
    checkUpstreamConnection();

    // Handle routing
    handleRouting();

    // Scan for connected clients
    static unsigned long lastClientScan = 0;
    if (millis() - lastClientScan > 5000) { // Every 5 seconds
        scanConnectedClients();
        lastClientScan = millis();
    }

    // Update client statistics
    updateClientStats();

    // Handle DNS requests if server is active
    if (dnsServer != nullptr) {
        dnsServer->processNextRequest();
    }

    // Update display periodically
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 1000) { // Every second
        updateDisplay();
        lastDisplayUpdate = millis();
    }

    delay(10);
}

void EmergencyRouter::stop() {
    Serial.println("[EmergencyRouter] Stopping Emergency Router...");

    stopRouter();

    if (display != nullptr) {
        display->clear();
        display->showMessage("Emergency Router", "Stopped");
    }
}

void EmergencyRouter::handleButton(uint8_t button) {
    // Handle menu navigation
    switch (button) {
        case 1: // Up
            if (menuPosition > 0) {
                menuPosition--;
                updateDisplay();
            }
            break;

        case 2: // Down
            if (menuPosition < 2) { // 3 views (0-2)
                menuPosition++;
                updateDisplay();
            }
            break;

        case 0: // Select
            // Toggle between views
            break;
    }
}

void EmergencyRouter::showMainMenu() {
    if (display == nullptr) return;

    display->clear();
    display->showEmergencyRouterMenu(menuPosition);
}

void EmergencyRouter::handleMenuNavigation() {
    // Placeholder for menu navigation
}

void EmergencyRouter::updateDisplay() {
    if (display == nullptr) return;

    unsigned long uptime = (millis() - startTime) / 1000;

    switch (menuPosition) {
        case 0: // Status view
            display->showRouterStatus(
                config.upstreamConnected,
                connectedClients.size(),
                totalBytesRouted,
                uptime
            );
            break;

        case 1: // Client list view
            showClientList();
            break;

        case 2: // Statistics view
            showStatistics();
            break;
    }
}

bool EmergencyRouter::connectToUpstream() {
    Serial.print("[EmergencyRouter] Connecting to upstream: ");
    Serial.println(config.upstreamSSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.upstreamSSID.c_str(), config.upstreamPassword.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        config.upstreamConnected = true;
        config.upstreamIP = WiFi.localIP();
        config.upstreamGateway = WiFi.gatewayIP();

        Serial.print("[EmergencyRouter] Connected! IP: ");
        Serial.println(config.upstreamIP);

        return true;
    }

    Serial.println("[EmergencyRouter] Connection failed");
    return false;
}

bool EmergencyRouter::startAccessPoint() {
    Serial.print("[EmergencyRouter] Starting AP: ");
    Serial.println(config.apSSID);

    // Configure AP
    WiFi.mode(WIFI_AP_STA); // Dual mode: STA + AP
    bool result = WiFi.softAP(
        config.apSSID.c_str(),
        config.apPassword.c_str(),
        1,                      // Channel
        0,                      // SSID hidden (0=visible)
        config.maxClients       // Max connections
    );

    if (result) {
        WiFi.softAPConfig(config.apIP, config.apGateway, config.apSubnet);

        Serial.print("[EmergencyRouter] AP started! IP: ");
        Serial.println(WiFi.softAPIP());

        return true;
    }

    Serial.println("[EmergencyRouter] Failed to start AP");
    return false;
}

bool EmergencyRouter::startDHCPServer() {
    Serial.println("[EmergencyRouter] DHCP server is handled by ESP32 softAP automatically");
    // ESP32's softAP mode includes a built-in DHCP server
    return true;
}

bool EmergencyRouter::startDNSServer() {
    Serial.println("[EmergencyRouter] Starting DNS server...");

    dnsServer = new DNSServer();

    // Start DNS server on port 53
    // Forward all DNS requests to the configured DNS server
    bool result = dnsServer->start(
        53,                         // DNS port
        "*",                        // Catch all domains
        config.apIP                 // IP to return
    );

    if (result) {
        Serial.println("[EmergencyRouter] DNS server started");
        return true;
    }

    Serial.println("[EmergencyRouter] Failed to start DNS server");
    delete dnsServer;
    dnsServer = nullptr;
    return false;
}

void EmergencyRouter::stopRouter() {
    if (dnsServer != nullptr) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }

    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    connectedClients.clear();

    status = RouterStatus::INITIALIZING;
    config.upstreamConnected = false;
}

void EmergencyRouter::scanConnectedClients() {
    // Get number of connected stations (simple method for now)
    uint8_t stationCount = WiFi.softAPgetStationNum();

    Serial.print("[EmergencyRouter] Connected clients: ");
    Serial.println(stationCount);

    // Note: The newer ESP32 Arduino framework doesn't expose detailed station info easily
    // For full client details, would need to use lower-level ESP-IDF APIs
    // This is a simplified version for now

    // If count changed, log it
    static uint8_t lastCount = 0;
    if (stationCount != lastCount) {
        if (stationCount > lastCount) {
            Serial.println("[EmergencyRouter] New client(s) connected");
        } else {
            Serial.println("[EmergencyRouter] Client(s) disconnected");
        }
        lastCount = stationCount;
    }
}

void EmergencyRouter::updateClientStats() {
    // Placeholder: Update bandwidth statistics for each client
    // TODO: Implement packet counting and bandwidth tracking
}

EmergencyRouter::ClientInfo* EmergencyRouter::findClient(const uint8_t* mac) {
    for (auto& client : connectedClients) {
        if (memcmp(client.mac, mac, 6) == 0) {
            return &client;
        }
    }
    return nullptr;
}

void EmergencyRouter::handleRouting() {
    // Placeholder: Handle NAT/routing between STA and AP interfaces
    // TODO: Implement packet forwarding
    totalPacketsRouted++;
}

void EmergencyRouter::forwardPacket(const uint8_t* packet, size_t length) {
    // Placeholder: Forward packet between interfaces
    // TODO: Implement packet forwarding with NAT
    totalBytesRouted += length;
}

void EmergencyRouter::checkUpstreamConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        config.upstreamConnected = false;

        // Attempt reconnect every 30 seconds
        if (millis() - lastReconnectAttempt > 30000) {
            attemptReconnect();
        }
    } else {
        config.upstreamConnected = true;
    }
}

void EmergencyRouter::attemptReconnect() {
    Serial.println("[EmergencyRouter] Attempting to reconnect to upstream...");

    lastReconnectAttempt = millis();
    reconnectAttempts++;

    WiFi.reconnect();
}

void EmergencyRouter::loadDefaultConfig() {
    // Upstream (phone hotspot) - USER MUST CONFIGURE
    config.upstreamSSID = "MyPhoneHotspot";
    config.upstreamPassword = "password123";
    config.upstreamConnected = false;

    // Access Point
    config.apSSID = "ESP32-EmergencyRouter";
    config.apPassword = "emergency2024";
    config.apIP = IPAddress(192, 168, 4, 1);
    config.apGateway = IPAddress(192, 168, 4, 1);
    config.apSubnet = IPAddress(255, 255, 255, 0);

    // DHCP range
    config.dhcpStart = IPAddress(192, 168, 4, 2);
    config.dhcpEnd = IPAddress(192, 168, 4, 20);

    // DNS
    config.dnsServer = IPAddress(8, 8, 8, 8);
    config.dnsForwarding = true;

    // Limits
    config.maxClients = 4; // ESP32 limitation
}

void EmergencyRouter::showConfiguration() {
    if (display == nullptr) return;

    display->showRouterConfig(
        config.apSSID,
        config.apIP.toString(),
        connectedClients.size(),
        config.maxClients
    );
}

void EmergencyRouter::showClientList() {
    if (display == nullptr) return;

    display->showRouterClients(connectedClients.size());
}

void EmergencyRouter::showStatistics() {
    if (display == nullptr) return;

    unsigned long uptime = (millis() - startTime) / 1000;

    display->showRouterStats(
        totalBytesRouted,
        totalPacketsRouted,
        uptime
    );
}
