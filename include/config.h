#ifndef CONFIG_H
#define CONFIG_H

// ==================== OPERATIONAL MODE ====================
// Choose ONE mode - these are mutually exclusive
#define MODE_DUAL_ENGINE        true   // RF Scanner + Network Analyzer (concurrent)
#define MODE_EMERGENCY_ROUTER   false  // Emergency Router only (exclusive)

// Boot behavior
#define AUTO_START_ON_BOOT      true   // Start engines automatically (no menu)
#define SHOW_BOOT_STATUS        true   // Display boot status log on OLED
#define BOOT_HEALTH_CHECK       true   // Perform POST (Power-On Self Test)

// WiFi Configuration
#define WIFI_SSID "CenturyLink2121"
#define WIFI_PASSWORD "ic7iu9iq3gc6iu"

// HTTPS Server Configuration
#define SERVER_URL "https://your-server.com/api"
#define AUTH_TOKEN "your-auth-token"

// Upload interval (milliseconds)
#define UPLOAD_INTERVAL 30000  // 30 seconds

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

// Scan Detection Thresholds
#define SCAN_THRESHOLD_PACKETS 50     // packets per second to trigger scan alert
#define SCAN_THRESHOLD_TIME 1000      // time window in ms
#define ANOMALY_THRESHOLD_RSSI -90    // minimum RSSI for valid signal

// Display Update Interval
#define DISPLAY_UPDATE_INTERVAL 1000  // ms

// Log Buffer Size
#define MAX_EVENTS_BUFFER 100

// ==================== EMERGENCY ROUTER CONFIGURATION ====================
// Upstream connection (phone hotspot)
#define UPSTREAM_SSID "MyPhoneHotspot"      // Change to your phone's hotspot SSID
#define UPSTREAM_PASSWORD "password123"     // Change to your phone's hotspot password

// Access Point configuration
#define AP_SSID "ESP32-EmergencyRouter"     // SSID of the ESP32's AP
#define AP_PASSWORD "emergency2024"         // Password for ESP32's AP (min 8 chars)
#define AP_CHANNEL 1                        // WiFi channel for AP
#define AP_MAX_CLIENTS 4                    // Max simultaneous connections (ESP32 limit)

// IP Configuration
#define AP_IP_ADDR "192.168.4.1"            // ESP32 AP IP address
#define AP_GATEWAY "192.168.4.1"            // Gateway (same as AP IP)
#define AP_SUBNET "255.255.255.0"           // Subnet mask

// DHCP Range
#define DHCP_START "192.168.4.2"            // First assignable IP
#define DHCP_END "192.168.4.20"             // Last assignable IP

// DNS Configuration
#define DNS_SERVER "8.8.8.8"                // Google DNS (fallback)
#define DNS_PORT 53                         // Standard DNS port

#endif
