#ifndef PACKET_SNIFFER_H
#define PACKET_SNIFFER_H

#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <map>
#include <vector>

// WiFi packet structures
struct SniffedPacket {
    uint8_t src_mac[6];
    uint8_t dst_mac[6];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    uint16_t seq_num;
    uint8_t protocol;
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
    std::vector<uint8_t> channels;
    bool is_ap;
    String ssid;
    uint32_t data_sent;
    uint32_t data_received;
};

class PacketSniffer {
public:
    PacketSniffer();
    void begin(uint8_t channel = 1);
    void stop();
    void setChannel(uint8_t channel);
    void channelHop(bool enable);

    // Callback for packet processing
    static void packetHandler(void* buff, wifi_promiscuous_pkt_type_t type);

    // Get statistics
    std::map<String, DeviceStats>& getDevices();
    uint32_t getTotalPackets();

private:
    static std::map<String, DeviceStats> devices;
    static uint32_t total_packets;
    static uint8_t current_channel;
    static unsigned long last_channel_hop;

    static String macToString(const uint8_t* mac);
    static void processPacket(const wifi_promiscuous_pkt_t* packet);
};

#endif
