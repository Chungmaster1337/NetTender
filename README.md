# ESP32 Multi-Engine Security Platform

A sophisticated ESP32-based security research platform featuring three independent operational engines for WiFi security testing, network analysis, and emergency networking. Built on ESP32 with dual-core architecture, OLED display, and comprehensive remote access capabilities.

## Overview

This platform provides a complete security research toolkit in a portable ESP32 form factor. The system features auto-boot operation, split-screen OLED display with real-time health monitoring, multi-level logging, and remote access via web dashboard and telnet.

### Key Features

- **Tri-Engine Architecture**: Three independent operational modes
- **Dual-Engine Concurrent Operation**: Engines 1 and 2 run simultaneously
- **Auto-Boot to Operational**: No user input required, compile-time configuration
- **Split-Screen OLED Display**: Real-time health status and critical logs
- **Advanced Logging System**: 6-level classification with web dashboard export
- **Remote Access**: Web interface (HTTP) and Telnet server
- **Universal Cloud Storage**: WebDAV, S3-compatible, and HTTP protocols
- **Power-On Self Test (POST)**: Automatic hardware validation at boot

## Operational Modes

### Mode 1: Dual Engine (RF Scanner + Network Analyzer)

Default operational mode running two concurrent engines for comprehensive wireless security research.

**Engine 1: RF Scanner**
- Passive WiFi scanning and packet capture
- Deauthentication attack capabilities
- Beacon spam and probe flood
- Evil twin AP deployment
- PMKID capture for WPA2 analysis
- BLE scanning and analysis
- Device tracking and relationship mapping

**Engine 2: Network Analyzer**
- Passive network traffic monitoring
- DNS server with ad blocking (PiHole-like)
- MITM proxy with ARP spoofing
- HTTP/HTTPS traffic inspection
- Network flow analysis and capture
- Protocol detection and bandwidth monitoring
- Device relationship and dependency mapping

### Mode 2: Emergency Router (Exclusive)

Standalone mode converting a phone hotspot into a fully functional WiFi router for emergency networking scenarios.

**Features**
- Dual WiFi mode (STA + AP)
- Upstream connection to phone hotspot
- Downstream WiFi access point
- Built-in DHCP server
- DNS forwarding
- Client tracking and bandwidth monitoring
- NAT/routing between interfaces
- Connection resilience with auto-reconnect

## Hardware Requirements

### Minimum Configuration
- ESP32 Dev Module (or compatible)
- 4MB Flash minimum (8MB recommended)
- 128x64 OLED Display (SSD1306, I2C)
- USB power supply

### Tested Hardware
- Arduino Nano ESP32 (primary development target)
- ESP32 DevKit V1
- ESP32 WROOM-32

### Display Connection
- OLED Display: SSD1306 128x64
- Interface: I2C
- SDA: A4 (Arduino Nano ESP32) / GPIO 21 (ESP32 DevKit)
- SCL: A5 (Arduino Nano ESP32) / GPIO 22 (ESP32 DevKit)
- Address: 0x3C
- Note: Configure pins in [config.h](include/config.h) based on your board

## Software Architecture

### Core Components

**EngineManager**
- Multi-engine lifecycle management
- Power-On Self Test (POST) execution
- Auto-start logic based on compile-time configuration
- Periodic health monitoring (5-second intervals)
- Engine creation, initialization, and cleanup

**SystemLogger**
- 6-level logging system: INFORMATIONAL, WARNING, ERROR, CRITICAL, FLAGGED, SUCCESS
- Engine health tracking with heartbeat monitoring
- Log filtering (OLED shows only CRITICAL/FLAGGED/SUCCESS)
- Timestamped entries: HH:MM:SS format
- Web dashboard export with full history

**DisplayManager**
- Split-screen OLED interface (64px left, 64px right)
- Left panel: Engine health status (OK/ERR/!! indicators)
- Right panel: Live log stream with latest critical events
- Boot sequence visualization with progress
- U8g2 library with optimized rendering

**WebInterface**
- HTTP server on port 80
- Dashboard with engine status table
- Scrollable log viewer with color-coded severity
- JSON API at /api/logs for programmatic access
- Real-time system statistics

**TelnetServer**
- Telnet access on port 23
- Command-line interface for remote management
- Commands: status, engines, start, stop, restart, help, clear, exit
- Session management with authentication hooks

### Packet Capture System

**PacketSniffer**
- WiFi promiscuous mode operation
- ISR-safe packet handling
- Device statistics tracking
- MAC address resolution
- Channel hopping support
- Static members for ISR context

**NetworkMonitor**
- Network quality metrics (RSSI, packet loss)
- Scan detection (port scans, network probes)
- Connection event tracking
- Anomaly detection heuristics

## Configuration

### Compile-Time Mode Selection

Edit `include/config.h`:

```cpp
// Dual Engine Mode: RF Scanner + Network Analyzer (concurrent)
#define MODE_DUAL_ENGINE        true
#define MODE_EMERGENCY_ROUTER   false

// Auto-start engines at boot (no menu)
#define AUTO_START_ON_BOOT      true

// Display boot status sequence on OLED
#define SHOW_BOOT_STATUS        true

// Perform Power-On Self Test (POST)
#define BOOT_HEALTH_CHECK       true
```

### Network Features

Edit `include/NetworkConfig.h` to enable/disable network services:

```cpp
// Web and Telnet Services
#define NET_ENABLE_HTTP_SERVER      true
#define NET_ENABLE_TELNET           true

// Cloud Storage Protocols
#define NET_ENABLE_WEBDAV           false  // NextCloud, ownCloud, etc.
#define NET_ENABLE_S3               false  // AWS S3, MinIO, Wasabi, etc.
#define NET_ENABLE_HTTP_UPLOAD      false  // Custom HTTP endpoints

// DNS and Captive Portal
#define NET_ENABLE_DNS_SERVER       false
#define NET_ENABLE_CAPTIVE_PORTAL   false
```

### Emergency Router Configuration

Edit `src/EmergencyRouter.cpp` function `loadDefaultConfig()`:

```cpp
// Upstream (phone hotspot)
config.upstreamSSID = "YourPhoneHotspot";
config.upstreamPassword = "your_password";

// Access Point
config.apSSID = "ESP32-EmergencyRouter";
config.apPassword = "emergency2024";
config.maxClients = 4;  // ESP32 hardware limitation
```

## Building and Flashing

### Prerequisites

Install PlatformIO:
```bash
pip install platformio
```

### Build Firmware

```bash
cd esp32
~/.platformio/penv/bin/platformio run
```

Build output:
- `firmware.bin` - Main application (943 KB)
- `bootloader.bin` - ESP32 bootloader (17 KB)
- `partitions.bin` - Partition table (3 KB)

### Flash to Device

```bash
# Auto-detect port
~/.platformio/penv/bin/platformio run --target upload

# Specify port manually
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/ttyUSB0

# Flash and monitor serial output
~/.platformio/penv/bin/platformio run --target upload --target monitor
```

### Serial Monitor

```bash
~/.platformio/penv/bin/platformio device monitor --baud 115200
```

## Memory Usage

### Flash Memory
- Used: 937,369 bytes (71.5%)
- Available: 373,351 bytes (28.5%)
- Total: 1,310,720 bytes (1.25 MB partition)

### RAM Usage
- Compile-time: 49,968 bytes (15.2%)
- Runtime: Variable based on active engines and packet buffers
- Available: 277,712 bytes (84.8%)

## Boot Sequence

1. **Hardware Initialization**
   - Display: OLED initialization and test
   - WiFi: Hardware presence check
   - Memory: Free heap validation (minimum 50 KB)

2. **Power-On Self Test (POST)**
   - Display functional test
   - WiFi hardware validation
   - Memory availability check
   - Results logged and displayed on OLED

3. **Engine Auto-Start**
   - Dual Engine Mode: RF Scanner + Network Analyzer start concurrently
   - Emergency Router Mode: Router engine starts exclusively
   - Engine registration with SystemLogger
   - Health monitoring activation

4. **Operational**
   - Split-screen OLED shows engine health and live logs
   - Web server starts on WiFi connection
   - Telnet server available for remote access
   - Engines enter main loop execution

## Display Layout

### Boot Sequence View
```
+------------------------+
|    BOOT SEQUENCE       |
|  [Component]: Status   |
|  [========>     ] 60%  |
+------------------------+
```

### Operational View (Split-Screen)
```
+------------+------------+
| RF Scanner | 12:34:56   |
|    OK      | CRITICAL   |
| NetworkAna | RF Scanner |
|    OK      | Deauth OK  |
|            |            |
| Errors: 0  | 12:34:58   |
| Warns:  2  | SUCCESS    |
|            | NetAnalyze |
+------------+------------+
 Left: Health | Right: Logs
```

## Web Dashboard

Access at `http://<esp32-ip>/`

### Features
- Engine status table with operational state
- Error and warning counts per engine
- Scrollable log viewer with full history
- Color-coded log entries:
  - Red: ERROR, CRITICAL
  - Orange: WARNING
  - Green: SUCCESS
  - Yellow: FLAGGED
  - Blue: INFORMATIONAL

### API Endpoints

**GET /logs** - HTML log viewer
**GET /api/logs** - JSON log data
```json
{
  "engines": [
    {
      "name": "RF Scanner",
      "status": "operational",
      "errors": 0,
      "warnings": 2
    }
  ],
  "logs": [
    {
      "timestamp": "12:34:56",
      "level": "SUCCESS",
      "engine": "RF Scanner",
      "message": "Packet capture started"
    }
  ]
}
```

## Telnet Interface

Connect: `telnet <esp32-ip> 23`

### Available Commands

| Command | Description |
|---------|-------------|
| status | Show system and engine status |
| engines | List all active engines |
| start [engine] | Start specified engine |
| stop [engine] | Stop specified engine |
| restart | Restart all engines |
| help | Show command help |
| clear | Clear screen |
| exit | Disconnect session |

## Cloud Storage Integration

### WebDAV Configuration

Compatible with: NextCloud, ownCloud, Box, pCloud, Yandex, Koofr, Google Drive (via WebDAV bridge)

```cpp
#define NET_ENABLE_WEBDAV           true
#define NET_WEBDAV_URL              "https://cloud.example.com/remote.php/dav/files/user/"
#define NET_WEBDAV_USERNAME         "username"
#define NET_WEBDAV_PASSWORD         "password"
```

### S3-Compatible Configuration

Compatible with: AWS S3, MinIO, Wasabi, Backblaze B2, DigitalOcean Spaces, Cloudflare R2, Storj

```cpp
#define NET_ENABLE_S3               true
#define NET_S3_ENDPOINT             "s3.amazonaws.com"
#define NET_S3_BUCKET               "bucket-name"
#define NET_S3_ACCESS_KEY           "access_key"
#define NET_S3_SECRET_KEY           "secret_key"
#define NET_S3_REGION               "us-east-1"
```

### Generic HTTP Upload

```cpp
#define NET_ENABLE_HTTP_UPLOAD      true
#define NET_HTTP_UPLOAD_URL         "https://api.example.com/upload"
#define NET_HTTP_UPLOAD_METHOD      "POST"
#define NET_HTTP_API_KEY            "api_key"
```

## Development Status

### Implemented Features
- [x] Multi-engine architecture
- [x] Dual-engine concurrent operation
- [x] Auto-boot with compile-time configuration
- [x] Power-On Self Test (POST)
- [x] Split-screen OLED display
- [x] Multi-level logging system
- [x] Engine health monitoring
- [x] Web dashboard with log viewer
- [x] Telnet remote access
- [x] Emergency router mode
- [x] Universal cloud storage protocols
- [x] Stub implementations for RF Scanner and Network Analyzer

### In Development
- [ ] Full RF Scanner packet capture implementation
- [ ] Deauthentication attack functionality
- [ ] Beacon spam and probe flood
- [ ] PMKID capture and analysis
- [ ] Network Analyzer MITM proxy
- [ ] DNS server with ad blocking
- [ ] Traffic flow capture and analysis
- [ ] BLE scanning and device tracking

### Planned Features
- [ ] SD card logging for long-term storage
- [ ] GPS module integration for wardriving
- [ ] Battery level monitoring
- [ ] Sleep mode for power conservation
- [ ] OTA firmware updates
- [ ] SSL certificate generation for MITM
- [ ] Custom filter rules for packet capture
- [ ] Packet replay capabilities

## Security Considerations

### Authorized Use Only

This platform is designed for authorized security testing, defensive security research, CTF challenges, and educational contexts. Users must:

- Obtain explicit written authorization before testing any network
- Only use on networks you own or have permission to test
- Comply with all local laws and regulations regarding wireless security testing
- Use responsibly and ethically

### Prohibited Use

Do NOT use this platform for:
- Unauthorized network access or testing
- Disrupting network services (DoS attacks)
- Mass targeting of networks or devices
- Supply chain compromise
- Detection evasion for malicious purposes
- Any illegal activity

### Legal Notice

Misuse of wireless security tools may violate computer fraud and abuse laws, unauthorized access statutes, and wireless communication regulations. Users are solely responsible for ensuring their use complies with applicable laws.

## Troubleshooting

### Build Errors

**Error: undefined reference to class members**
- Ensure all .cpp files are in the src/ directory
- Check that static members are initialized in .cpp files
- Verify header includes are correct

**Error: macro conflicts (e.g., DNS_SERVER)**
- Check for naming collisions between enums and #define macros
- Rename enum values or macros to avoid conflicts

**Error: dynamic_cast not allowed**
- ESP32 Arduino framework may have RTTI limitations
- Use string comparison or other type identification methods instead

### Runtime Issues

**OLED display not showing**
- Check I2C connections (SDA: GPIO 21, SCL: GPIO 22)
- Verify OLED address (typically 0x3C)
- Check power supply voltage (3.3V or 5V depending on module)

**POST failures**
- Display not initialized: Check I2C connections
- WiFi hardware fault: Verify ESP32 module
- Low memory: Disable unused features in NetworkConfig.h

**Web dashboard not accessible**
- Check WiFi connection status in serial monitor
- Verify ESP32 IP address (printed at boot)
- Ensure NET_ENABLE_HTTP_SERVER is true in NetworkConfig.h

**Emergency Router not connecting to upstream**
- Verify phone hotspot credentials in EmergencyRouter.cpp
- Check phone hotspot is active and visible
- Monitor serial output for connection status

## Contributing

Contributions are welcome! Areas of interest:

- Full implementation of RF Scanner packet capture
- Network Analyzer MITM and DNS features
- Additional cloud storage protocols
- Performance optimizations
- Documentation improvements
- Bug fixes and testing

### Development Guidelines

- Follow existing code structure and naming conventions
- Use static members only when required (ISR context)
- Document all public APIs with comments
- Test on real hardware before submitting
- Keep memory usage minimal (target <80% flash, <30% RAM)

## License

This project is provided as-is for educational and authorized security research purposes. Users assume all responsibility for compliance with applicable laws.

## References

### ESP32 Documentation
- ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/
- Arduino-ESP32 Reference: https://docs.espressif.com/projects/arduino-esp32/

### Libraries
- U8g2 OLED Library: https://github.com/olikraus/u8g2
- ArduinoJson: https://arduinojson.org/
- PlatformIO: https://platformio.org/

### Wireless Security
- WiFi Promiscuous Mode: ESP32 Technical Reference Manual
- 802.11 Frame Types: IEEE 802.11 Standard
- WPA2 Security: Wi-Fi Alliance Documentation

## Project Structure

See `PROJECT_STRUCTURE.md` for detailed file organization and build artifact documentation.

## Contact

For questions, issues, or contributions, please use the GitHub issue tracker.

---

**Disclaimer**: This tool is for educational and authorized security testing only. Unauthorized network access or disruption is illegal and unethical. Always obtain proper authorization before conducting security research.
