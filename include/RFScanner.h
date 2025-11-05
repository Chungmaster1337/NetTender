#ifndef RF_SCANNER_H
#define RF_SCANNER_H

#include "EngineManager.h"
#include "DisplayManager.h"
#include "PacketSniffer.h"
#include <Arduino.h>

/**
 * @brief RF Scanner Engine - Flipper/Marauder-like capabilities
 *
 * Features:
 * - WiFi packet sniffing and analysis
 * - Deauth attacks
 * - Beacon spam
 * - Probe flooding
 * - Evil twin AP
 * - BLE scanning
 * - PMKID capture
 * - Handshake capture
 * - Device tracking and proximity alerts
 * - Real-time statistics
 */
class RFScanner : public Engine {
public:
    RFScanner(DisplayManager* display);
    ~RFScanner();

    // Engine interface implementation
    bool begin() override;
    void loop() override;
    void stop() override;
    const char* getName() override { return "RF Scanner"; }
    void handleButton(uint8_t button) override;

private:
    DisplayManager* display;
    PacketSniffer* sniffer;

    // Scanner modes
    enum class ScanMode {
        PASSIVE_SCAN,
        DEAUTH_ATTACK,
        BEACON_SPAM,
        PROBE_FLOOD,
        EVIL_TWIN,
        PMKID_CAPTURE,
        BLE_SCAN
    };

    ScanMode currentMode;
    uint8_t menuPosition;
    bool inSubmenu;

    // Statistics
    unsigned long startTime;
    uint32_t totalPackets;
    uint32_t totalDevices;

    // Attack parameters
    uint8_t targetChannel;
    uint8_t targetMAC[6];
    String targetSSID;

    // Methods
    void showMainMenu();
    void handleModeSelection();
    void runPassiveScan();
    void runDeauthAttack();
    void runBeaconSpam();
    void runProbeFlood();
    void runEvilTwin();
    void runPMKIDCapture();
    void runBLEScan();
    void updateDisplay();
};

#endif // RF_SCANNER_H
