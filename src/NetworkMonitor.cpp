#include "NetworkMonitor.h"

NetworkMonitor::NetworkMonitor() {
    // Initialize monitoring
}

void NetworkMonitor::updateQuality(const String& mac, int8_t rssi) {
    NetworkQuality& quality = quality_map[mac];
    quality.rssi = rssi;
    quality.last_update = millis();

    // Placeholder: Update packet statistics
    quality.successful_packets++;

    // Calculate loss percentage
    uint32_t total = quality.successful_packets + quality.packet_loss;
    if (total > 0) {
        quality.loss_percentage = (float)quality.packet_loss / (float)total * 100.0f;
    }
}

NetworkQuality NetworkMonitor::getQuality(const String& mac) {
    if (quality_map.find(mac) != quality_map.end()) {
        return quality_map[mac];
    }

    // Return default quality metrics if not found
    NetworkQuality default_quality;
    default_quality.rssi = -100;
    default_quality.packet_loss = 0;
    default_quality.successful_packets = 0;
    default_quality.loss_percentage = 0.0f;
    default_quality.retransmissions = 0;
    default_quality.last_update = 0;

    return default_quality;
}

void NetworkMonitor::analyzeForScans(const String& src_mac, const String& dst_mac, uint16_t seq_num) {
    // Placeholder: Analyze packet patterns for scan detection
    ScanDetectionState& state = scan_detection[src_mac];

    state.recent_seq_nums.push_back(seq_num);
    state.last_packet_time = millis();
    state.rapid_packet_count++;

    // Keep only recent sequence numbers (last 100)
    if (state.recent_seq_nums.size() > 100) {
        state.recent_seq_nums.erase(state.recent_seq_nums.begin());
    }

    // Check for scan pattern
    if (isScanPattern(state)) {
        // Create scan event
        ScanEvent event;
        // Parse MAC address
        sscanf(src_mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &event.scanner_mac[0], &event.scanner_mac[1], &event.scanner_mac[2],
               &event.scanner_mac[3], &event.scanner_mac[4], &event.scanner_mac[5]);
        event.scanner_mac_str = src_mac;
        event.timestamp = millis();
        event.ports_scanned = state.recent_seq_nums.size();
        event.rate = state.rapid_packet_count;
        event.scan_type = "NETWORK_SCAN";

        scan_events.push_back(event);

        // Reset state
        state.rapid_packet_count = 0;
    }
}

std::vector<ScanEvent>& NetworkMonitor::getDetectedScans() {
    return scan_events;
}

void NetworkMonitor::recordConnection(const String& mac, const String& type, int8_t rssi) {
    ConnectionEvent event;
    event.device_mac = mac;
    event.event_type = type;
    event.timestamp = millis();
    event.rssi = rssi;
    event.device_name = "Unknown";

    connection_events.push_back(event);

    // Keep only recent events (last 100)
    if (connection_events.size() > 100) {
        connection_events.erase(connection_events.begin());
    }
}

std::vector<ConnectionEvent>& NetworkMonitor::getRecentEvents(uint32_t count) {
    // Return all events if count is larger than available
    // In a real implementation, would return only the last 'count' events
    return connection_events;
}

bool NetworkMonitor::detectAnomalies(const String& mac) {
    // Placeholder: Anomaly detection logic
    if (quality_map.find(mac) == quality_map.end()) {
        return false;
    }

    NetworkQuality& quality = quality_map[mac];

    // Simple anomaly detection: high packet loss or very low RSSI
    if (quality.loss_percentage > 50.0f) {
        return true;
    }

    if (quality.rssi < -90) {
        return true;
    }

    return false;
}

bool NetworkMonitor::isScanPattern(const ScanDetectionState& state) {
    // Placeholder: Simple heuristic for scan detection
    // Real implementation would analyze sequence numbers, timing, etc.

    // Consider it a scan if:
    // 1. More than 50 packets in rapid succession
    // 2. Within a short time window (e.g., 5 seconds)

    if (state.rapid_packet_count > 50) {
        return true;
    }

    return false;
}
