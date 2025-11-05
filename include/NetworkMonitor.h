#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <Arduino.h>
#include <map>
#include <vector>

// Network quality metrics
struct NetworkQuality {
    int8_t rssi;
    uint32_t packet_loss;
    uint32_t successful_packets;
    float loss_percentage;
    uint32_t retransmissions;
    unsigned long last_update;
};

// Scan detection
struct ScanEvent {
    uint8_t scanner_mac[6];
    String scanner_mac_str;
    uint32_t timestamp;
    uint16_t ports_scanned;
    uint16_t rate;  // packets per second
    String scan_type;  // "PORT_SCAN", "NETWORK_SCAN", "PROBE_FLOOD"
};

// Connection event
struct ConnectionEvent {
    String device_mac;
    String event_type;  // "CONNECT", "DISCONNECT", "RECONNECT"
    uint32_t timestamp;
    int8_t rssi;
    String device_name;
};

class NetworkMonitor {
public:
    NetworkMonitor();

    // Quality monitoring
    void updateQuality(const String& mac, int8_t rssi);
    NetworkQuality getQuality(const String& mac);

    // Scan detection
    void analyzeForScans(const String& src_mac, const String& dst_mac, uint16_t seq_num);
    std::vector<ScanEvent>& getDetectedScans();

    // Connection tracking
    void recordConnection(const String& mac, const String& type, int8_t rssi);
    std::vector<ConnectionEvent>& getRecentEvents(uint32_t count = 10);

    // Anomaly detection
    bool detectAnomalies(const String& mac);

private:
    std::map<String, NetworkQuality> quality_map;
    std::vector<ScanEvent> scan_events;
    std::vector<ConnectionEvent> connection_events;

    // Scan detection state
    struct ScanDetectionState {
        std::vector<uint16_t> recent_seq_nums;
        unsigned long last_packet_time;
        uint32_t rapid_packet_count;
    };
    std::map<String, ScanDetectionState> scan_detection;

    bool isScanPattern(const ScanDetectionState& state);
};

#endif
