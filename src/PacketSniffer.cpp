#include "PacketSniffer.h"
#include <esp_wifi.h>

// Initialize static members
std::map<String, DeviceStats> PacketSniffer::devices;
uint32_t PacketSniffer::total_packets = 0;
uint8_t PacketSniffer::current_channel = 1;
unsigned long PacketSniffer::last_channel_hop = 0;

PacketSniffer::PacketSniffer() {
    // Constructor
}

void PacketSniffer::begin(uint8_t channel) {
    current_channel = channel;

    // Set WiFi to promiscuous mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&PacketSniffer::packetHandler);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);

    Serial.println("[PacketSniffer] Started on channel " + String(current_channel));
}

void PacketSniffer::stop() {
    esp_wifi_set_promiscuous(false);
    Serial.println("[PacketSniffer] Stopped");
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

uint32_t PacketSniffer::getTotalPackets() {
    return total_packets;
}

std::map<String, DeviceStats>& PacketSniffer::getDevices() {
    return devices;
}

String PacketSniffer::macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void PacketSniffer::processPacket(const wifi_promiscuous_pkt_t* packet) {
    // Placeholder: Process captured packet
    // Extract MAC addresses, RSSI, channel, etc.
    total_packets++;
}

void IRAM_ATTR PacketSniffer::packetHandler(void* buf, wifi_promiscuous_pkt_type_t type) {
    // This is called from ISR context, so keep it minimal
    if (type != WIFI_PKT_MGMT) {
        return; // Only process management frames for now
    }

    const wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buf;

    // Increment packet counter
    total_packets++;

    // TODO: In a real implementation, would queue packet for processing
    // processPacket(packet);
}
