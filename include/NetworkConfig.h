#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

/**
 * @file NetworkConfig.h
 * @brief Comprehensive networking configuration flags
 *
 * Enable/disable individual networking features to optimize
 * memory usage and functionality for your specific use case.
 */

// ==================== CORE PROTOCOLS ====================

// HTTP/Web Server
#define NET_ENABLE_HTTP_SERVER          true
#define NET_HTTP_PORT                   80
#define NET_HTTP_MAX_CLIENTS            4

// HTTPS/SSL
#define NET_ENABLE_HTTPS                false
#define NET_HTTPS_PORT                  443

// Telnet Server
#define NET_ENABLE_TELNET               true
#define NET_TELNET_PORT                 23

// mDNS (local .local hostname resolution)
#define NET_ENABLE_MDNS                 false
#define NET_MDNS_HOSTNAME               "esp32"

// DNS Server
#define NET_ENABLE_DNS_SERVER           true
#define NET_DNS_PORT                    53

// DHCP Server
#define NET_ENABLE_DHCP_SERVER          true

// ==================== MESSAGING PROTOCOLS ====================

// MQTT
#define NET_ENABLE_MQTT                 false
#define NET_MQTT_BROKER                 "broker.hivemq.com"
#define NET_MQTT_PORT                   1883

// CoAP
#define NET_ENABLE_COAP                 false
#define NET_COAP_PORT                   5683

// WebSocket
#define NET_ENABLE_WEBSOCKET            false
#define NET_WEBSOCKET_PORT              8080

// ==================== TIME & DISCOVERY ====================

// NTP (time sync)
#define NET_ENABLE_NTP                  true
#define NET_NTP_SERVER                  "pool.ntp.org"

// SSDP/UPnP
#define NET_ENABLE_SSDP                 false

// SNMP
#define NET_ENABLE_SNMP                 false

// ==================== CLOUD STORAGE (UNIVERSAL PROTOCOLS) ====================

// WebDAV (Works with: NextCloud, ownCloud, Box, pCloud, Yandex, Koofr, Google Drive, etc.)
#define NET_ENABLE_WEBDAV               false
#define NET_WEBDAV_URL                  "https://cloud.example.com/remote.php/dav/files/username/"
#define NET_WEBDAV_USERNAME             "username"
#define NET_WEBDAV_PASSWORD             "password"

// S3-Compatible API (Works with: AWS S3, MinIO, Wasabi, Backblaze B2, DigitalOcean Spaces, Cloudflare R2, Storj)
#define NET_ENABLE_S3                   false
#define NET_S3_ENDPOINT                 "s3.amazonaws.com"
#define NET_S3_BUCKET                   "your-bucket-name"
#define NET_S3_REGION                   "us-east-1"
#define NET_S3_ACCESS_KEY               "your-access-key"
#define NET_S3_SECRET_KEY               "your-secret-key"
#define NET_S3_USE_PATH_STYLE           false  // true for MinIO, false for AWS

// Generic HTTP POST/PUT (Works with any custom endpoint)
#define NET_ENABLE_HTTP_UPLOAD          false
#define NET_HTTP_UPLOAD_URL             "https://your-server.com/upload"
#define NET_HTTP_UPLOAD_METHOD          "POST"  // POST or PUT
#define NET_HTTP_UPLOAD_AUTH_HEADER     "Authorization"
#define NET_HTTP_UPLOAD_AUTH_VALUE      "Bearer your-token"

// FTP Upload (legacy, works with any FTP server)
#define NET_ENABLE_FTP_UPLOAD           false
#define NET_FTP_SERVER                  "ftp.example.com"
#define NET_FTP_PORT                    21
#define NET_FTP_USERNAME                "username"
#define NET_FTP_PASSWORD                "password"
#define NET_FTP_BASE_PATH               "/uploads/"

// Cloud upload behavior
#define NET_CLOUD_AUTO_UPLOAD           false  // Auto-upload on capture complete
#define NET_CLOUD_UPLOAD_INTERVAL       3600   // seconds between scheduled uploads
#define NET_CLOUD_UPLOAD_ON_REBOOT      false  // Upload pending data on reboot
#define NET_CLOUD_RETRY_COUNT           3      // Number of upload retries
#define NET_CLOUD_RETRY_DELAY           5000   // ms between retries

// ==================== NETWORK ANALYSIS TOOLS ====================

// Packet Capture (PCAP)
#define NET_ENABLE_PCAP                 true
#define NET_PCAP_BUFFER_SIZE            8192

// NetFlow
#define NET_ENABLE_NETFLOW              false
#define NET_NETFLOW_COLLECTOR_IP        "192.168.1.100"

// Syslog
#define NET_ENABLE_SYSLOG               false
#define NET_SYSLOG_SERVER               "192.168.1.100"

// Port Mirroring
#define NET_ENABLE_PORT_MIRROR          false

// ==================== SECURITY ====================

// Firewall
#define NET_ENABLE_FIREWALL             true

// IDS/IPS
#define NET_ENABLE_IDS                  false

// MAC Filtering
#define NET_ENABLE_MAC_FILTER           false

// Rate Limiting
#define NET_ENABLE_RATE_LIMIT           false

// ==================== ROUTING ====================

// NAT
#define NET_ENABLE_NAT                  true

// IP Forwarding
#define NET_ENABLE_IP_FORWARD           true

// QoS
#define NET_ENABLE_QOS                  false

// Load Balancing
#define NET_ENABLE_LOAD_BALANCE         false

// ==================== TUNNELING ====================

// IP Tunnel
#define NET_ENABLE_IP_TUNNEL            false

// GRE
#define NET_ENABLE_GRE                  false

// WireGuard VPN
#define NET_ENABLE_WIREGUARD            false

// ==================== FILE SHARING ====================

// FTP Server
#define NET_ENABLE_FTP_SERVER           false
#define NET_FTP_PORT                    21

// TFTP
#define NET_ENABLE_TFTP                 false

// SMB/CIFS
#define NET_ENABLE_SMB                  false

// ==================== API ====================

// REST API
#define NET_ENABLE_REST_API             true

// GraphQL
#define NET_ENABLE_GRAPHQL              false

// Prometheus Metrics
#define NET_ENABLE_PROMETHEUS           false

// ==================== RADIO PROTOCOLS ====================

// Bluetooth Serial
#define NET_ENABLE_BT_SERIAL            false

// BLE GATT
#define NET_ENABLE_BLE_GATT             false

// ESP-NOW
#define NET_ENABLE_ESP_NOW              false

// LoRa
#define NET_ENABLE_LORA                 false

// ==================== DEBUGGING ====================

// Packet logging
#define NET_DEBUG_PACKET_LOG            false

// Statistics
#define NET_ENABLE_STATS                true

// Traffic graphs
#define NET_ENABLE_GRAPHS               false

// ==================== STORAGE ====================

// Config persistence
#define NET_ENABLE_CONFIG_STORAGE       true

// SD card logging
#define NET_ENABLE_SD_LOGGING           false

// PCAP to SD
#define NET_ENABLE_SD_PCAP              false

// ==================== ADVANCED ====================

// IPv6
#define NET_ENABLE_IPV6                 false

// Multicast
#define NET_ENABLE_MULTICAST            false

// IGMP
#define NET_ENABLE_IGMP                 false

#endif // NETWORK_CONFIG_H
