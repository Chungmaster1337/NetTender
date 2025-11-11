# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based WiFi security research platform ("Sniffy Boi") focused on wardriving and WPA2 attacks. Features auto-boot operation, split-screen OLED display, complete handshake capture, PMKID extraction, and deauthentication attacks.

**Target Hardware:** Arduino Nano ESP32 (primary), ESP32 DevKit V1
**Framework:** Arduino-ESP32 (via PlatformIO)
**Attack Vectors:** Handshake capture, PMKID (clientless), Deauth injection

## Build Commands

### Development Workflow

```bash
# Build firmware (from /home/user/esp32)
~/.platformio/penv/bin/platformio run

# Flash to device (auto-detect port)
~/.platformio/penv/bin/platformio run --target upload

# Flash and monitor serial output
~/.platformio/penv/bin/platformio run --target upload --target monitor

# Serial monitor only (115200 baud)
~/.platformio/penv/bin/platformio device monitor --baud 115200
```

### Port Configuration

Default upload port is `/dev/ttyACM0` (Arduino Nano ESP32). Override with:
```bash
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/ttyUSB0
```

### Dependencies

**Minimal library footprint for maximum flash efficiency:**

```ini
lib_deps =
    olikraus/U8g2@^2.35.9  # OLED display driver (SSD1306 128x64)
```

**Removed dependencies** (after transformation to wardriving platform):
- ~~ArduinoJson~~ - No longer needed after removing WebInterface, TelnetServer, CloudStorage
- All remote access features removed to focus on packet capture performance

**Rationale:**
- U8g2 (48MB source, ~30KB in firmware): Required for "Sniffy Boi" boot screen and operational display
- No JSON library needed: Hashcat exports use simple string formatting
- No HTTP/WebSocket: OLED + serial interface only
- ESP32 native libraries handle all WiFi operations: `esp_wifi.h`, `esp_wifi_types.h`

## Core Architecture

### Engine System

The platform uses a simplified **single-engine architecture** focused exclusively on WiFi packet capture and attacks. The `EngineManager` handles lifecycle management and health monitoring for the RF Scanner engine.

**Key Design:** After removing NetworkAnalyzer, the system is streamlined for wardriving performance. The `Engine` base class pattern is retained for future extensibility (GPS logger, SD card writer, etc.).

#### Engine Lifecycle

1. **Registration:** `EngineManager::createEngine(EngineType)` instantiates the RFScanner
2. **Initialization:** Engine's `begin()` sets up hardware (WiFi promiscuous mode, packet buffers, etc.)
3. **Execution:** `EngineManager::loop()` calls the engine's `loop()` continuously
4. **Health Monitoring:** Periodic heartbeat checks via `SystemLogger` (every 5 seconds)
5. **Cleanup:** `stop()` releases resources, disables promiscuous mode, frees memory

**Current Engine:**
- **RFScanner** (`src/RFScanner.cpp`): WiFi packet capture, handshake extraction, PMKID capture, deauth injection
  - Uses **PacketSniffer** for 802.11 frame parsing
  - Uses **PMKIDCapture** for clientless PMKID attacks
  - Exports to Hashcat mode 22000 format

### Logging System

**SystemLogger** (`src/SystemLogger.cpp`) implements a 6-level severity system with intelligent filtering:

- **INFORMATIONAL/WARNING/ERROR:** Logged but NOT shown on OLED
- **CRITICAL/FLAGGED/SUCCESS:** Logged AND displayed on OLED right panel

This design prevents OLED spam while maintaining full log history for web dashboard export. Each log entry is timestamped (`HH:MM:SS`) and includes engine name for correlation.

**Engine Health Tracking:**
- Engines call `logger->registerEngine(name, color)` during `begin()`
- `engineHeartbeat(name)` updates lastHeartbeat timestamp
- `setEngineStatus(name, operational, errorMsg)` marks failures
- Health status appears in left panel: `OK`, `ERR`, or `!!` indicators

### Display Architecture

**DisplayManager** (`src/DisplayManager.cpp`) handles 128x64 SSD1306 OLED with split-screen layout:

**Left Panel (64px):** Engine health dashboard
```
RF Scanner
   OK
NetworkAna
   OK

Errors: 0
Warns:  2
```

**Right Panel (64px):** Live log stream (last 3-4 critical events)
```
12:34:56
CRITICAL
RF Scanner
Deauth OK
```

**Display Modes:**
- **Boot Sequence:** Progress bar with component status (`showBootStatus()`)
- **WiFi Status:** Connection progress with SSID (`showWiFiStatus()`)
- **Operational View:** Split-screen health + logs (`showOperationalView()`)

The display updates at 1Hz in main loop to balance responsiveness and power consumption.

### Configuration System

**Single configuration file:**

**`include/config.h`**: WiFi credentials, boot behavior, I2C pins, channel hopping settings

**Compile-time mode selection** eliminates runtime menu systems:
```cpp
#define MODE_DUAL_ENGINE        true   // Now runs RF Scanner only (NetworkAnalyzer removed)
#define AUTO_START_ON_BOOT      true   // No user input required
#define BOOT_HEALTH_CHECK       true   // POST validation
#define ENABLE_CHANNEL_HOPPING  true   // Scan all 13 channels
#define CHANNEL_HOP_INTERVAL    1000   // 1 second per channel
```

**Removed:** `NetworkConfig.h` - All networking protocols removed (HTTP, telnet, WebDAV, S3, MQTT, etc.)

This approach optimizes memory usage—unused features are excluded from the final binary.

## Packet Capture System

### WiFi Promiscuous Mode

**PacketSniffer** (`src/PacketSniffer.cpp`) operates in ESP32's promiscuous mode, which bypasses standard WiFi stack to capture raw 802.11 frames.

**Critical Pattern - ISR Safety:**
```cpp
class PacketSniffer {
    static PacketSniffer* instance;  // Static for ISR context
    static void IRAM_ATTR promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type);
};
```

The promiscuous callback runs in **Interrupt Service Routine (ISR) context**, which prohibits:
- Dynamic memory allocation (`new`, `malloc`)
- String operations (they allocate internally)
- Serial/display output
- FreeRTOS synchronization primitives

**Solution:** Use static members and defer processing to main loop. The ISR captures raw data, stores in fixed buffers, and sets flags for processing in `loop()`.

### Channel Hopping

Channel hopping (`esp_wifi_set_channel()`) scans all 13 2.4GHz WiFi channels to detect hidden networks and monitor entire spectrum:

```cpp
// config.h
#define START_CHANNEL 1
#define MAX_CHANNEL 13
#define CHANNEL_HOP_INTERVAL 1000  // ms between hops
```

**Trade-off:** Hopping increases coverage but may miss packets during channel switches. Disable hopping (`ENABLE_CHANNEL_HOPPING false`) for targeted capture.

## User Interface

### Serial Console

Primary interface at **115200 baud** via USB:

**RFScanner outputs:**
- Device discovery (beacons, probes, data frames)
- EAPOL handshake capture progress (M1-M4)
- PMKID extraction from M1 frames
- Deauth attack status
- Hashcat export strings (mode 22000)

**SystemLogger outputs:**
- Engine health heartbeats
- Critical events (handshake complete, PMKID captured)
- Error diagnostics

### OLED Display

**128x64 SSD1306 display** with split-screen layout:

**Boot sequence:**
- "Sniffy Boi..." splash screen (large font)
- POST status with progress bar

**Operational view:**
- Left panel: Engine health dashboard
- Right panel: Live critical event stream (last 3-4 events)
- Updates at 1Hz

**Removed features:** Web dashboard and telnet server removed to minimize attack surface and maximize packet capture performance.

## Memory Constraints

**Flash Usage:** 775KB / 3145KB (24.6%) - **ample headroom** for SD logging, GPS, menu system
**RAM Usage:** 45KB / 327KB (13.7%) - packet buffers, log history, handshake storage

**Optimization Guidelines:**
1. Use `F("string")` macros for PROGMEM storage of string literals
2. Minimize String objects in hot loops (prefer char arrays)
3. Limit `SystemLogger` maxEntries (currently 100) for memory-constrained devices
4. Disable unused protocols in `NetworkConfig.h`
5. Monitor `ESP.getFreeHeap()` during development

**Static Member Warning:** Only use static members when ISR context requires it. Static members increase binary size and can cause initialization order issues.

## Boot Sequence

1. **Hardware Init** (main.cpp:27-52)
   - Serial @ 115200 baud
   - DisplayManager (I2C on A4/A5 for Nano ESP32)
   - SystemLogger (100 entry ring buffer)
   - EngineManager

2. **Power-On Self Test** (EngineManager::performPOST)
   - Display functional test
   - WiFi hardware validation (esp_wifi_init)
   - Memory check (minimum 50KB free heap)
   - Results logged and displayed on OLED

3. **WiFi Connection** (main.cpp:78-175)
   - 30-second timeout with progress bar
   - Specific error handling for WL_NO_SSID_AVAIL, WL_CONNECT_FAILED
   - IP address logged and displayed
   - Auto-reconnect enabled

4. **Auto-Start Engine** (EngineManager::autoStart)
   - Single-engine mode: RF Scanner launches automatically
   - Engine calls `begin()` to enable WiFi promiscuous mode
   - PacketSniffer and PMKIDCapture modules initialized
   - Engine registers with logger for health tracking

5. **Operational Loop** (main.cpp)
   - RF Scanner's `loop()` method executed continuously
   - Packet capture and processing (beacons, handshakes, PMKIDs)
   - Display updates at 1Hz
   - Serial output for handshake/PMKID captures

## Security Context

**Authorization Required:** This platform is for authorized security testing, defensive research, CTF competitions, and educational use only.

**Prohibited Activities:**
- Unauthorized network access or testing
- DoS attacks or mass targeting
- Supply chain compromise
- Detection evasion for malicious purposes

**Legal Compliance:** Users must obtain written authorization and comply with local laws (Computer Fraud and Abuse Act, unauthorized access statutes, wireless communication regulations).

## Development Guidelines

### Adding a New Engine

1. Create `include/NewEngine.h` and `src/NewEngine.cpp`
2. Inherit from `Engine` base class (EngineManager.h:24)
3. Implement required methods: `begin()`, `loop()`, `stop()`, `getName()`
4. Add enum value to `EngineType` (EngineManager.h:15)
5. Update `EngineManager::createEngine()` to instantiate your engine
6. Register with logger: `logger->registerEngine(getName(), colorCode)`
7. Call `logger->engineHeartbeat(getName())` in `loop()` (every iteration or periodic)
8. Use `logger->critical/flagged/success()` for OLED-visible events

### ISR-Safe Packet Handling

When working with WiFi promiscuous callbacks:
```cpp
// WRONG - will crash
static void IRAM_ATTR callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    String ssid = extractSSID(buf);  // String allocation in ISR!
    Serial.println(ssid);            // Serial I/O in ISR!
}

// CORRECT
static volatile bool packetReady = false;
static uint8_t packetBuffer[2048];

static void IRAM_ATTR callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    memcpy(packetBuffer, buf, len);  // Copy to static buffer
    packetReady = true;              // Set flag
}

void loop() {
    if (packetReady) {
        processPacket(packetBuffer);  // Process in main loop
        packetReady = false;
    }
}
```

### Naming Conflicts

Avoid enum values that collide with `#define` macros in config files. Historical example:
```cpp
// BAD: conflicts with #define DNS_SERVER in config
enum class AnalyzerMode { DNS_SERVER };

// GOOD: uses descriptive name to avoid collision
enum class AnalyzerMode { DNS_MODE };
```

**Current codebase** has minimal risk since networking features were removed, but watch for collisions with ESP32 SDK macros (WIFI_*, ESP_*, etc.).

### Testing Without Hardware

Use `test_i2c_scanner.cpp` to detect OLED I2C address (typically 0x3C). If display fails POST:
1. Check I2C connections (SDA=A4/GPIO21, SCL=A5/GPIO22)
2. Verify OLED voltage (3.3V or 5V depending on module)
3. Run I2C scanner: compile test_i2c_scanner.cpp alone

## Troubleshooting

### Build Errors

**"undefined reference" errors:** Ensure all .cpp files are in `src/` directory and static members are initialized in .cpp (not just declared in .h)

**Macro collision errors:** Check for enum/class names matching `#define` macros (use namespaced enums or rename)

**`dynamic_cast` not allowed:** ESP32 Arduino has limited RTTI. Use string comparison or type IDs instead.

### Runtime Issues

**OLED blank:** Check I2C address (run I2C scanner), verify connections, check power supply voltage

**POST failures:** Display issues (check I2C connections A4/A5), WiFi hardware fault (verify ESP32 module), low memory (reduce SystemLogger maxEntries)

**Engine not starting:** Check serial output for `begin()` failures, verify free heap > 50KB, ensure promiscuous mode not already enabled

**No packets captured:** Verify WiFi channel matches target AP, disable channel hopping for targeted capture, check antenna connection

## Project Status

**Implemented (Sniffy Boi v1.0):**
- ✅ Single-engine architecture optimized for packet capture (24.6% flash)
- ✅ Auto-boot and Power-On Self Test
- ✅ "Sniffy Boi" OLED display with split-screen health monitoring
- ✅ Complete 802.11 frame parser (beacons, probes, deauth, data)
- ✅ Full WPA/WPA2 handshake capture (M1-M4 with nonce extraction)
- ✅ PMKID capture module (passive + clientless attack)
- ✅ Deauthentication attack (targeted, broadcast, handshake trigger)
- ✅ Hashcat mode 22000 export for both handshakes and PMKIDs
- ✅ Device tracking with statistics (RSSI, channels, encryption)
- ✅ Comprehensive attack guides (3 markdown documents)

**Next Priorities:**
- SD card logging for PCAP export and long-term storage
- GPS integration for wardriving coordinates (lat/lon/timestamp)
- OLED menu system for attack mode selection
- Auto-attack mode (deauth all clients on channel)
- WPA3 detection and flagging

**Removed Features:**
- ❌ NetworkAnalyzer, WebInterface, TelnetServer, CloudStorage
- ❌ All networking protocols (HTTP, WebDAV, S3, MQTT, FTP)
- ❌ ArduinoJson dependency
- **Result:** Freed 162KB flash, 4KB RAM
