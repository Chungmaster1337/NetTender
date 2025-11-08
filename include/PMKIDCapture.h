#ifndef PMKID_CAPTURE_H
#define PMKID_CAPTURE_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

/**
 * PMKID Attack Module
 *
 * PMKID is a newer WPA2 attack that requires only the FIRST EAPOL message (M1).
 * It's faster and simpler than full 4-way handshake capture.
 *
 * PMKID = HMAC-SHA1-128(PMK, "PMK Name" | MAC_AP | MAC_STA)
 *
 * The PMKID is included in the RSN Information Element of the M1 frame.
 * Hashcat can crack PMKID just like handshakes (mode 22000).
 *
 * Advantages over handshake capture:
 * - Only needs M1 (not M1+M2+M3+M4)
 * - Can work without any connected clients (clientless attack)
 * - Faster capture (no waiting for reconnection)
 * - Works on same networks as handshake capture
 *
 * AUTHORIZED USE ONLY - Requires written permission to test networks
 */

// PMKID Information
struct PMKIDInfo {
    uint8_t ap_mac[6];          // Access Point BSSID
    uint8_t sta_mac[6];         // Station MAC (client or fake)
    uint8_t pmkid[16];          // PMKID (16 bytes)
    String ssid;                // Network name
    uint8_t channel;            // WiFi channel
    int8_t rssi;                // Signal strength
    uint32_t timestamp;         // When captured (millis)
    bool is_valid;              // Successfully extracted PMKID
    bool is_clientless;         // Captured via fake association
};

class PMKIDCapture {
public:
    PMKIDCapture();

    /**
     * @brief Start PMKID capture (passive mode)
     * Monitors EAPOL M1 frames for PMKID in RSN IE
     */
    void beginPassive();

    /**
     * @brief Stop PMKID capture
     */
    void stop();

    /**
     * @brief Clientless attack - send association request to AP
     * This triggers the AP to send M1 with PMKID, no real client needed!
     *
     * @param ap_mac Target AP BSSID
     * @param ssid Target SSID (optional, can be empty for broadcast)
     */
    void sendAssociationRequest(const uint8_t* ap_mac, const String& ssid = "");

    /**
     * @brief Attack all APs on current channel (clientless)
     * Sends association requests to all discovered APs
     */
    void attackAllAPs();

    /**
     * @brief Get captured PMKIDs
     */
    const std::vector<PMKIDInfo>& getPMKIDs() const { return pmkids; }
    uint32_t getPMKIDCount() const { return pmkids.size(); }

    /**
     * @brief Export PMKID in hashcat format
     * Format: WPA*01*PMKID*AP_MAC*STA_MAC*SSID
     */
    String exportHashcat(const PMKIDInfo& pmkid);

    /**
     * @brief Print summary of captured PMKIDs
     */
    void printSummary();

    /**
     * @brief Process EAPOL frame (called by packet sniffer)
     * Extracts PMKID from M1 if present
     */
    static void processEAPOL_M1(const uint8_t* eapol_data, uint16_t len,
                                const uint8_t* ap_mac, const uint8_t* sta_mac,
                                int8_t rssi, uint8_t channel, const String& ssid);

private:
    static std::vector<PMKIDInfo> pmkids;
    static uint32_t association_sent_count;
    static unsigned long last_association_time;

    // Fake MAC for clientless attacks
    static uint8_t fake_sta_mac[6];

    /**
     * @brief Extract PMKID from RSN Information Element
     *
     * RSN IE format in EAPOL M1:
     * - Tag: 0xDD (Vendor Specific)
     * - Length: variable
     * - OUI: 0x00 0x0F 0xAC (WiFi Alliance)
     * - Type: 0x04 (PMKID KDE)
     * - PMKID: 16 bytes
     */
    static bool extractPMKID(const uint8_t* eapol_key_data, uint16_t data_len, uint8_t* pmkid_out);

    /**
     * @brief Build association request frame
     */
    static void buildAssociationRequest(uint8_t* frame, size_t* frame_len,
                                        const uint8_t* ap_mac, const uint8_t* sta_mac,
                                        const String& ssid);

    /**
     * @brief Helper to convert MAC to string
     */
    static String macToString(const uint8_t* mac);

    /**
     * @brief Generate random MAC address for clientless attacks
     */
    static void generateFakeMAC();
};

#endif // PMKID_CAPTURE_H
