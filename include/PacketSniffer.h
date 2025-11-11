#ifndef PACKET_SNIFFER_H
#define PACKET_SNIFFER_H

#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <map>
#include <vector>

// 802.11 Frame Types
#define FRAME_TYPE_MANAGEMENT 0x00
#define FRAME_TYPE_CONTROL    0x01
#define FRAME_TYPE_DATA       0x02

// Management Frame Subtypes
#define FRAME_SUBTYPE_BEACON       0x08
#define FRAME_SUBTYPE_PROBE_REQ    0x04
#define FRAME_SUBTYPE_PROBE_RESP   0x05
#define FRAME_SUBTYPE_ASSOC_REQ    0x00
#define FRAME_SUBTYPE_ASSOC_RESP   0x01
#define FRAME_SUBTYPE_AUTH         0x0B
#define FRAME_SUBTYPE_DEAUTH       0x0C
#define FRAME_SUBTYPE_DISASSOC     0x0A

// Data Frame Subtypes
#define FRAME_SUBTYPE_DATA         0x00
#define FRAME_SUBTYPE_QOS_DATA     0x08

// Deauth reason codes
#define DEAUTH_REASON_UNSPECIFIED           1
#define DEAUTH_REASON_PREV_AUTH_NOT_VALID   2
#define DEAUTH_REASON_DEAUTH_LEAVING        3
#define DEAUTH_REASON_DISASSOC_DUE_TO_INACTIVITY 4

// 802.11 Frame Control structure
struct __attribute__((packed)) FrameControl {
    uint8_t protocol:2;
    uint8_t type:2;
    uint8_t subtype:4;
    uint8_t toDS:1;
    uint8_t fromDS:1;
    uint8_t moreFragments:1;
    uint8_t retry:1;
    uint8_t powerManagement:1;
    uint8_t moreData:1;
    uint8_t WEP:1;
    uint8_t order:1;
};

// 802.11 MAC Header
struct __attribute__((packed)) wifi_ieee80211_mac_hdr_t {
    FrameControl frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6]; // receiver address
    uint8_t addr2[6]; // sender address
    uint8_t addr3[6]; // filtering address / BSSID
    uint16_t seq_ctrl;
};

// Beacon/Probe Response payload
struct __attribute__((packed)) wifi_ieee80211_packet_t {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; // Variable length payload
};

// WiFi packet structures for tracking
struct SniffedPacket {
    uint8_t src_mac[6];
    uint8_t dst_mac[6];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    uint16_t seq_num;
    uint8_t frame_type;
    uint8_t frame_subtype;
    uint32_t timestamp;
    size_t payload_len;
};

// Device statistics
struct DeviceStats {
    uint8_t mac[6];
    String mac_str;
    uint32_t first_seen;
    uint32_t last_seen;
    uint32_t packet_count;
    int8_t avg_rssi;
    int8_t max_rssi;
    std::vector<uint8_t> channels;
    bool is_ap;
    String ssid;
    String vendor;  // OUI vendor lookup
    uint32_t beacons_sent;
    uint32_t probes_sent;
    uint32_t data_frames;
    bool has_encryption;
    uint8_t encryption_type; // 0=open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA3
};

// EAPOL Key Frame structure for parsing
struct __attribute__((packed)) eapol_key_frame_t {
    uint8_t version;           // EAPOL version
    uint8_t type;              // EAPOL type (3 = EAPOL-Key)
    uint16_t length;           // Body length
    uint8_t descriptor_type;   // Key descriptor type (2 or 254 for WPA)
    uint16_t key_info;         // Key information flags
    uint16_t key_length;       // Key length
    uint8_t replay_counter[8]; // Replay counter
    uint8_t key_nonce[32];     // Key nonce (ANonce or SNonce)
    uint8_t key_iv[16];        // EAPOL Key IV
    uint8_t key_rsc[8];        // Key Receive Sequence Counter
    uint8_t key_id[8];         // Key ID
    uint8_t key_mic[16];       // Key MIC
    uint16_t key_data_length;  // Key Data Length
    uint8_t key_data[0];       // Variable length key data
};

// Handshake tracking
struct HandshakeInfo {
    uint8_t client_mac[6];
    uint8_t ap_mac[6];
    String ssid;

    // Handshake messages captured
    bool has_m1;
    bool has_m2;
    bool has_m3;
    bool has_m4;

    // Nonces
    uint8_t anonce[32];        // Authenticator nonce (from M1)
    uint8_t snonce[32];        // Supplicant nonce (from M2)

    // MIC and other data for hashcat
    uint8_t mic[16];           // Message Integrity Code (from M2)
    uint8_t keyver;            // Key version (1=TKIP, 2=AES-CCMP, 3=AES-128-CMAC)

    // EAPOL frames (store for hashcat export)
    uint8_t eapol_m1[256];     // M1 frame
    uint16_t eapol_m1_len;
    uint8_t eapol_m2[256];     // M2 frame
    uint16_t eapol_m2_len;

    // Timestamps and state
    uint32_t timestamp;
    uint32_t last_update;
    bool is_complete;          // Has M1+M2 (minimum for cracking)
    bool is_full_handshake;    // Has all M1+M2+M3+M4
};

// Forward declaration
class CommandInterface;

class PacketSniffer {
public:
    PacketSniffer();
    void begin(uint8_t channel = 1);
    void stop();
    void setChannel(uint8_t channel);
    void channelHop(bool enable);

    // Set command interface for wireless C2
    void setCommandInterface(CommandInterface* cmd_interface);

    // Callback for packet processing
    static void IRAM_ATTR packetHandler(void* buff, wifi_promiscuous_pkt_type_t type);

    // Get statistics
    std::map<String, DeviceStats>& getDevices();
    uint32_t getTotalPackets();
    uint32_t getBeaconCount();
    uint32_t getProbeCount();
    uint32_t getDataCount();
    uint32_t getDeauthCount();
    uint32_t getHandshakeCount();
    uint8_t getCurrentChannel();

    // Get captured handshakes
    const std::vector<HandshakeInfo>& getHandshakes();

    // Export handshakes
    String exportHandshakeHashcat(const HandshakeInfo& hs);  // PMKID/EAPOL format for hashcat
    void printHandshakeSummary();

    // Deauth attacks (AUTHORIZED USE ONLY)
    void sendDeauthAttack(const uint8_t* target_mac, const uint8_t* ap_mac, uint8_t reason = DEAUTH_REASON_UNSPECIFIED);
    void sendDeauthBroadcast(const uint8_t* ap_mac, uint8_t reason = DEAUTH_REASON_UNSPECIFIED);
    void triggerHandshake(const uint8_t* ap_mac, const uint8_t* client_mac, uint8_t burst_count = 5);

    // Beacon flood attack (AUTHORIZED USE ONLY - Research/CTF)
    void startBeaconFlood(uint8_t channel = 1);
    void stopBeaconFlood();
    bool isBeaconFloodActive();
    void setBeaconFloodSSIDs(const std::vector<String>& ssids);
    void beaconFloodLoop();  // Call from main loop when active

private:
    static std::map<String, DeviceStats> devices;
    static std::vector<HandshakeInfo> handshakes;
    static uint32_t total_packets;
    static uint32_t beacon_count;
    static uint32_t probe_count;
    static uint32_t data_count;
    static uint32_t deauth_count;
    static uint8_t current_channel;
    static unsigned long last_channel_hop;
    static CommandInterface* command_interface;  // For wireless C2

    static String macToString(const uint8_t* mac);
    static void processPacket(const wifi_promiscuous_pkt_t* packet);
    static void processBeacon(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi);
    static void processProbeRequest(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi);
    static void processProbeResponse(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi);
    static void processDeauth(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi);
    static void processDataFrame(const wifi_ieee80211_packet_t* pkt, uint16_t len, int8_t rssi);
    static void processEAPOL(const uint8_t* payload, uint16_t len, const uint8_t* src, const uint8_t* dst, const uint8_t* bssid);
    static String extractSSID(const uint8_t* payload, uint16_t len);
    static uint8_t getEncryptionType(const uint8_t* payload, uint16_t len);
    static void updateDeviceStats(const uint8_t* mac, int8_t rssi, uint8_t channel, bool is_ap, const String& ssid = "");

    // Handshake tracking helpers
    static HandshakeInfo* findHandshake(const uint8_t* client_mac, const uint8_t* ap_mac);
    static void updateHandshakeState(HandshakeInfo* hs);
    static String getSSIDForBSSID(const uint8_t* bssid);

    // Deauth attack helpers
    static void buildDeauthFrame(uint8_t* frame, const uint8_t* dest, const uint8_t* src, const uint8_t* bssid, uint8_t reason);

    // Beacon flood helpers
    static void buildBeaconFrame(uint8_t* frame, const String& ssid, uint8_t channel, const uint8_t* mac);
    static void sendBeaconFrame(const String& ssid, uint8_t channel);

    // Attack tracking (prevent abuse)
    static unsigned long last_deauth_time;
    static const uint32_t DEAUTH_RATE_LIMIT_MS = 100;  // Min 100ms between attacks

    // Beacon flood state
    static bool beacon_flood_active;
    static std::vector<String> beacon_ssids;
    static uint8_t beacon_flood_channel;
    static unsigned long last_beacon_time;
    static uint32_t beacon_interval_us;  // Microseconds between beacons
    static uint32_t beacons_sent;
};

#endif
