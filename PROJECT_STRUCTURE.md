# ESP32 Multi-Engine Platform - Project Structure

## Pre-Build Structure

```
esp32/
├── include/                    # Header files
│   ├── CloudStorage.h         # Universal cloud storage protocols (WebDAV, S3, HTTP)
│   ├── config.h               # Operational mode configuration
│   ├── DataUploader.h         # Data upload management (header only, no implementation)
│   ├── DisplayManager.h       # OLED display interface (SSD1306)
│   ├── EmergencyRouter.h      # Emergency router engine (Engine 3)
│   ├── EngineManager.h        # Multi-engine lifecycle manager
│   ├── NetworkAnalyzer.h      # Network analyzer engine (Engine 2)
│   ├── NetworkConfig.h        # Network feature flags
│   ├── NetworkMonitor.h       # Network quality and scan detection
│   ├── PacketSniffer.h        # WiFi promiscuous mode packet capture
│   ├── RFScanner.h            # RF scanner engine (Engine 1)
│   ├── SystemLogger.h         # Multi-level logging system
│   ├── TelnetServer.h         # Remote telnet access
│   └── WebInterface.h         # Web dashboard interface
│
├── src/                       # Implementation files
│   ├── CloudStorage.cpp       # Cloud storage implementation
│   ├── DisplayManager.cpp     # OLED display rendering
│   ├── EmergencyRouter.cpp    # Router engine implementation
│   ├── EngineManager.cpp      # Engine management logic
│   ├── main.cpp               # Application entry point
│   ├── main.cpp.backup        # Backup of main.cpp
│   ├── NetworkAnalyzer.cpp    # Network analyzer implementation
│   ├── NetworkMonitor.cpp     # Network monitoring logic
│   ├── PacketSniffer.cpp      # Packet capture implementation
│   ├── RFScanner.cpp          # RF scanner implementation
│   ├── SystemLogger.cpp       # Logging system implementation
│   ├── TelnetServer.cpp       # Telnet server implementation
│   └── WebInterface.cpp       # Web server implementation
│
├── lib/                       # External libraries (empty, using PlatformIO registry)
├── .claude/                   # Claude Code configuration
│   └── settings.local.json   # Local Claude settings
├── .vscode/                   # VSCode configuration
│   └── extensions.json       # Recommended extensions
├── platformio.ini             # PlatformIO build configuration
├── .gitignore                 # Git ignore rules
├── README.md                  # Project documentation
├── PROJECT_STRUCTURE.md       # This file
└── test_i2c_scanner.cpp       # I2C device scanner utility
```

## Post-Build Structure

```
esp32/
├── [All pre-build files above]
│
├── .pio/                      # PlatformIO build system (generated)
│   ├── build/
│   │   └── esp32dev/
│   │       ├── firmware.elf      # ELF executable with debug symbols
│   │       ├── firmware.bin      # Main application binary
│   │       ├── bootloader.bin    # ESP32 bootloader
│   │       ├── partitions.bin    # Partition table
│   │       ├── src/              # Compiled object files (*.o)
│   │       ├── lib*/             # Compiled library archives
│   │       └── FrameworkArduino/ # Arduino framework objects
│   │
│   ├── libdeps/               # Downloaded library dependencies
│   │   └── esp32dev/
│   │       ├── U8g2/          # OLED display library
│   │       ├── ArduinoJson/   # JSON parsing library
│   │       └── [other libs]
│   │
│   └── penv/                  # Python virtual environment
│
└── .gitignore                 # Git ignore file (excludes .pio/)
```

## Key Files Description

### Configuration Files

**platformio.ini**
- Build system configuration
- Board: Arduino Nano ESP32
- Platform: espressif32
- Framework: Arduino
- Libraries: U8g2, ArduinoJson
- Build flags: PSRAM support, USB CDC enabled, debug level 3
- Upload: /dev/ttyACM0 at 115200 baud with custom reset flags

**include/config.h**
- Operational mode selection (Dual Engine vs Emergency Router)
- Auto-start configuration
- Boot sequence options
- POST (Power-On Self Test) enable/disable
- WiFi credentials and server configuration
- Display pin configuration (I2C: A4/A5 for Arduino Nano ESP32)
- WiFi monitoring parameters (channel hopping, scan thresholds)
- Emergency router configuration (upstream/downstream settings)

**include/NetworkConfig.h**
- Comprehensive network feature flags
- Cloud storage protocol configuration (WebDAV, S3, HTTP, FTP)
- Service enable/disable toggles (HTTP, HTTPS, Telnet, DNS, DHCP, etc.)
- Messaging protocols (MQTT, CoAP, WebSocket)
- Time and discovery services (NTP, SSDP, SNMP)
- Network analysis tools (PCAP, NetFlow, Syslog)
- Security features (Firewall, IDS, MAC filtering)
- Routing options (NAT, IP forwarding, QoS)
- Tunneling protocols (IP Tunnel, GRE, WireGuard)
- Radio protocols (Bluetooth, BLE, ESP-NOW, LoRa)

### Core System Files

**src/main.cpp**
- Application entry point
- System initialization
- Main loop coordination
- WiFi management

**include/EngineManager.h + src/EngineManager.cpp**
- Multi-engine lifecycle management
- Power-On Self Test (POST) implementation
- Auto-start logic for dual-engine or router mode
- Periodic health checks
- Engine creation and destruction

**include/SystemLogger.h + src/SystemLogger.cpp**
- Multi-level logging (INFORMATIONAL, WARNING, ERROR, CRITICAL, FLAGGED, SUCCESS)
- Engine health tracking with heartbeat monitoring
- Log filtering for OLED display
- Web dashboard log export

**include/DisplayManager.h + src/DisplayManager.cpp**
- Split-screen OLED interface (64 pixels left, 64 pixels right)
- Left: Engine health status with OK/ERR/!! indicators
- Right: Live critical logs with timestamps
- Boot sequence visualization

### Engine Files

**Engine 1: RF Scanner**
- include/RFScanner.h + src/RFScanner.cpp
- include/PacketSniffer.h + src/PacketSniffer.cpp
- Modes: Passive scan, deauth attack, beacon spam, probe flood, evil twin, PMKID capture, BLE scan
- WiFi promiscuous mode packet capture

**Engine 2: Network Analyzer**
- include/NetworkAnalyzer.h + src/NetworkAnalyzer.cpp
- include/NetworkMonitor.h + src/NetworkMonitor.cpp
- Modes: Passive monitor, DNS server, MITM proxy, traffic analysis, flow capture, network map
- ARP spoofing, DNS blocking, packet inspection

**Engine 3: Emergency Router**
- include/EmergencyRouter.h + src/EmergencyRouter.cpp
- Converts phone hotspot to WiFi router
- Dual WiFi mode (STA + AP)
- DHCP server, DNS forwarding, client tracking

### Network Services

**include/WebInterface.h + src/WebInterface.cpp**
- HTTP server on port 80
- Dashboard with engine status
- Log viewer with color-coded entries
- RESTful API for programmatic access

**include/TelnetServer.h + src/TelnetServer.cpp**
- Telnet server on port 23
- Remote command-line interface
- Commands: status, engines, start, stop, restart, help, clear, exit

**include/CloudStorage.h + src/CloudStorage.cpp**
- Universal cloud storage protocols
- WebDAV support (NextCloud, ownCloud, Google Drive, etc.)
- S3-compatible storage (AWS S3, MinIO, Wasabi, etc.)
- Generic HTTP POST/PUT for custom endpoints

### Utility Files

**include/DataUploader.h**
- Data upload management interface
- Note: Header only - no corresponding .cpp implementation exists
- Likely a planned feature not yet implemented

**test_i2c_scanner.cpp**
- I2C device scanner utility
- Used to detect OLED display address
- Standalone tool for hardware troubleshooting

## Build Artifacts

### Firmware Binaries

**firmware.bin**
- Main application code
- Contains all engine logic, network services, and UI
- Size varies based on enabled features

**bootloader.bin**
- ESP32 first-stage bootloader
- Initializes hardware and loads application

**partitions.bin**
- Partition table defining flash memory layout
- Typically includes: bootloader, app0, nvs, spiffs

### Memory Usage

**Flash Memory**
- Size depends on board (4MB minimum, 8MB recommended)
- Usage varies based on enabled features in NetworkConfig.h

**RAM Usage (at compile time)**
- Static allocation: ~15-20% of available RAM
- Dynamic allocation varies based on active engines and packet buffers
- Total: 327,680 bytes on ESP32

## Development Workflow

### Building the Project

```bash
# Clean build
~/.platformio/penv/bin/platformio run --target clean

# Build firmware
~/.platformio/penv/bin/platformio run

# Build with verbose output
~/.platformio/penv/bin/platformio run -v
```

### Flashing to Device

```bash
# Auto-detect port and flash
~/.platformio/penv/bin/platformio run --target upload

# Specify port manually
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/ttyACM0

# Flash and monitor serial output
~/.platformio/penv/bin/platformio run --target upload --target monitor
```

### Monitoring

```bash
# Serial monitor
~/.platformio/penv/bin/platformio device monitor

# Serial monitor with custom baud rate
~/.platformio/penv/bin/platformio device monitor --baud 115200
```

## File Organization Principles

### Header Files (include/)
- All class declarations
- Public API definitions
- Constant definitions
- Struct definitions

### Implementation Files (src/)
- Class method implementations
- Private helper functions
- Static member initialization

### Separation of Concerns
- Each engine is self-contained (own .h and .cpp)
- Network services are independent modules
- Configuration is centralized in config.h and NetworkConfig.h
- Logging is abstracted through SystemLogger

### Static Members
- PacketSniffer uses static members for ISR context
- Enables hardware callback access to packet processing state
- Required pattern for ESP32 WiFi promiscuous mode

## Dependencies

### PlatformIO Libraries
- U8g2 @ ^2.35.9 (OLED display driver)
- ArduinoJson @ ^6.21.3 (JSON parsing)
- WiFi (ESP32 built-in)
- HTTPClient (ESP32 built-in)
- WiFiClientSecure (ESP32 built-in)
- DNSServer (ESP32 built-in)
- ESPmDNS (ESP32 built-in)
- WebServer (ESP32 built-in)
- Wire (I2C communication)

### ESP-IDF Components
- esp_wifi (WiFi low-level API)
- esp_wifi_types (WiFi type definitions)

## Version Control

### .gitignore Recommendations
```
.pio/
.vscode/
.platformio
*.bin
*.elf
*.o
```

### Files to Track
- All source files (src/, include/)
- Configuration files (platformio.ini, config.h, NetworkConfig.h)
- Documentation (README.md, PROJECT_STRUCTURE.md)

### Files NOT to Track
- Build artifacts (.pio/build/)
- Library downloads (.pio/libdeps/)
- Python virtual environment (.pio/penv/)
- IDE settings (.vscode/, .idea/)

## Notes

### Missing Implementations
- **DataUploader.cpp**: Header file exists but no implementation
- This may be a planned feature or legacy code

### Board-Specific Configuration
- Primary target: Arduino Nano ESP32
- I2C pins use Arduino compatibility (A4/A5)
- USB CDC enabled for serial communication over USB
- PSRAM support enabled for future expansion

### Build Configuration Details
- Debug level set to 3 (verbose)
- Monitor filters include ESP32 exception decoder
- Upload uses custom reset sequence (no_reset_no_sync before, hard_reset after)
- Monitor DTR/RTS disabled for compatibility
