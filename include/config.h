#ifndef CONFIG_H
#define CONFIG_H

// ==================== OPERATIONAL MODE ====================
#define MODE_DUAL_ENGINE        true   // RF Scanner + Network Analyzer (concurrent)

// Boot behavior
#define AUTO_START_ON_BOOT      true   // Start engines automatically (no menu)
#define SHOW_BOOT_STATUS        true   // Display boot status log on OLED
#define BOOT_HEALTH_CHECK       true   // Perform POST (Power-On Self Test)

// ==================== WARDRIVING MODE ====================
// NOTE: Sniffy Boi operates in WiFi MONITOR mode (promiscuous)
// NO network connection is established - this is intentional!
// WiFi hardware is initialized for packet capture only

// Display Configuration (I2C pins)
// Arduino Nano ESP32 uses A4/A5 for I2C (Arduino Uno compatibility)
// Physical pins: Look for "A4" and "A5" labels on your board
// A4 = I2C SDA (data)
// A5 = I2C SCL (clock)
#define OLED_SDA A4
#define OLED_SCL A5
#define SDA_PIN A4
#define SCL_PIN A5

// WiFi Monitoring Configuration
#define START_CHANNEL 1
#define MAX_CHANNEL 13
#define CHANNEL_HOP_INTERVAL 1000  // ms between channel hops
#define ENABLE_CHANNEL_HOPPING true

// Beacon Flood Attack Configuration (AUTHORIZED USE ONLY)
#define BEACON_FLOOD_INTERVAL_US 1000  // microseconds between beacons (1000 = 1ms = 1000 beacons/sec)
#define BEACON_FLOOD_DEFAULT_CHANNEL 6  // default channel for beacon flooding
#define BEACON_FLOOD_SSID_COUNT 50      // number of fake SSIDs to cycle through

// Scan Detection Thresholds
#define SCAN_THRESHOLD_PACKETS 50     // packets per second to trigger scan alert
#define SCAN_THRESHOLD_TIME 1000      // time window in ms
#define ANOMALY_THRESHOLD_RSSI -90    // minimum RSSI for valid signal

// Display Update Interval
#define DISPLAY_UPDATE_INTERVAL 1000  // ms

// Log Buffer Size
#define MAX_EVENTS_BUFFER 100

#endif
