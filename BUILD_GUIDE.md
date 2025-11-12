# Sniffy Boi - Build & Upload Guide

Quick reference for building and uploading different versions of Sniffy Boi firmware.

---

## Available Versions

### v1.1 (Current - Optimized) ⭐ **RECOMMENDED**
Performance-optimized build with:
- ✅ Font caching (33% faster display rendering)
- ✅ Unified MAC address utilities
- ✅ Named constants (improved code readability)
- ✅ Zero behavioral changes from v1.0
- ✅ Fully backward compatible

### v1.1-debug
Debug build of v1.1 with:
- Full debug symbols
- Verbose logging (CORE_DEBUG_LEVEL=5)
- Larger binary size
- Use for development and troubleshooting

### v1.0 (Legacy)
Original release before optimizations. Use only if:
- You need to compare behavior with v1.1
- You're troubleshooting optimization-related issues
- You want the pre-optimization baseline

**Note:** To build v1.0, you must checkout the v1.0 git tag first:
```bash
git checkout v1.0
pio run -e v1_0 --target upload
git checkout main  # Return to current version
```

---

## Quick Commands

### Build Only (No Upload)

```bash
# Build v1.1 (current)
pio run -e v1_1

# Build v1.1 with debug symbols
pio run -e v1_1_debug

# Build v1.0 (requires git checkout first)
git checkout v1.0
pio run -e v1_0
git checkout main
```

### Build & Upload

```bash
# Upload v1.1 to device
pio run -e v1_1 --target upload

# Upload v1.1-debug (for development)
pio run -e v1_1_debug --target upload

# Upload v1.0 (legacy)
git checkout v1.0
pio run -e v1_0 --target upload
git checkout main
```

### Build, Upload & Monitor

```bash
# Complete workflow: build, flash, and open serial monitor
pio run -e v1_1 --target upload --target monitor
```

---

## Default Build

If you run `pio run` without specifying an environment, it defaults to **v1_1**:

```bash
# These are equivalent:
pio run
pio run -e v1_1
```

This is configured in `platformio.ini`:
```ini
[platformio]
default_envs = v1_1
```

---

## Environment Details

### [env:v1_1]
```ini
build_flags =
    -DCORE_DEBUG_LEVEL=1          # Minimal logging
    -DCONFIG_BT_ENABLED=0          # Bluetooth disabled (saves 145KB)
    -DCONFIG_NIMBLE_ENABLED=0      # BLE disabled
    -DBOARD_HAS_PSRAM              # External RAM support
    -DARDUINO_USB_MODE=1           # USB CDC enabled
    -DARDUINO_USB_CDC_ON_BOOT=1    # Serial over USB
    -DVERSION_1_1=1                # Version flag
```

**Memory Usage:**
- RAM: 13.7% (45 KB / 327 KB)
- Flash: 43.3% (852 KB / 1966 KB)

### [env:v1_1_debug]
```ini
build_flags =
    ${env:v1_1.build_flags}        # Inherits v1_1 flags
    -DDEBUG_BUILD=1                 # Enables DEBUG_LOG() macros
    -DCORE_DEBUG_LEVEL=5            # Maximum ESP32 logging
build_type = debug                  # Debug symbols included
```

**Use When:**
- Tracking down crashes or hangs
- Need ESP32 internal logging
- Debugging new features
- Analyzing stack traces

---

## Upload Port Configuration

Default upload port is `/dev/ttyACM0` (Arduino Nano ESP32).

### Override Upload Port

```bash
# Specify different port
pio run -e v1_1 --target upload --upload-port /dev/ttyUSB0

# Auto-detect port
pio device list
pio run -e v1_1 --target upload --upload-port /dev/ttyACM1
```

### Permanently Change Port

Edit `platformio.ini`:
```ini
[env]
upload_port = /dev/ttyUSB0  # Your port here
```

---

## Monitoring Serial Output

### During Upload
```bash
# Upload and immediately start monitoring
pio run -e v1_1 --target upload --target monitor
```

### Monitor Only (No Upload)
```bash
# Open serial monitor at 115200 baud
pio device monitor --baud 115200

# With exception decoder (shows stack traces)
pio device monitor --baud 115200 --filter esp32_exception_decoder
```

### Exit Monitor
Press `Ctrl+C` to exit the serial monitor.

---

## Version Information at Boot

When Sniffy Boi boots, it displays version information:

```
╔════════════════════════════════════════════════════════════╗
║                   SNIFFY BOI v1.1.0                        ║
║              Wardriving & WPA2 Attack Platform           ║
╚════════════════════════════════════════════════════════════╝

  Version:     v1.1.0
  Build:       Nov 11 2025 15:30:45
  Platform:    Arduino Nano ESP32
  Chip:        ESP32-S3

  Features:
    - Handshake Capture (WPA/WPA2)
    - PMKID Extraction (clientless)
    - Deauth Attacks (targeted & broadcast)
    - Beacon Flooding
    - Wireless C2 (magic packet commands)
    - Interactive Serial CLI
    - OLED Status Display
    - Hashcat Mode 22000 Export

  Optimizations (v1.1):
    ✓ Font caching (33% faster display)
    ✓ Unified MAC utilities
    ✓ Named constants (improved readability)

  Output:      Hashcat mode 22000
  Network:     Monitor mode (standalone operation)
```

---

## Troubleshooting

### Build Fails
```bash
# Clean build directory and rebuild
pio run -e v1_1 --target clean
pio run -e v1_1
```

### Upload Fails
```bash
# Check device connection
pio device list

# Try manual reset:
# 1. Hold BOOT button on device
# 2. Press and release RESET button
# 3. Release BOOT button
# 4. Run upload command
pio run -e v1_1 --target upload
```

### Serial Monitor Shows Garbage
```bash
# Ensure baud rate matches (115200)
pio device monitor --baud 115200

# If still broken, try:
pio device monitor --baud 115200 --eol LF
```

### Out of Memory During Build
```bash
# Free up space
rm -rf .pio/build

# Build again
pio run -e v1_1
```

---

## Build Size Comparison

| Version | Flash Used | RAM Used | Notes |
|---------|------------|----------|-------|
| v1.0    | 851.3 KB   | 45.0 KB  | Original baseline |
| v1.1    | 852.0 KB   | 45.0 KB  | +0.7 KB (version strings) |
| v1.1-debug | ~900 KB | ~50 KB   | +Debug symbols |

**Note:** v1.1 is slightly larger due to version info and optimization code, but runs faster.

---

## Comparing Versions Side-by-Side

### Build Both Versions
```bash
# Build current (v1.1)
pio run -e v1_1

# Build legacy (requires checkout)
git stash  # Save current changes
git checkout v1.0
pio run -e v1_0
git checkout main
git stash pop
```

### Flash Different Versions
```bash
# Flash v1.1 to one device
pio run -e v1_1 --target upload --upload-port /dev/ttyACM0

# Flash v1.0 to another device (after git checkout)
git checkout v1.0
pio run -e v1_0 --target upload --upload-port /dev/ttyACM1
git checkout main
```

---

## CI/CD Integration

### GitHub Actions Example
```yaml
- name: Build v1.1
  run: |
    pip install platformio
    pio run -e v1_1

- name: Upload Firmware Artifact
  uses: actions/upload-artifact@v3
  with:
    name: sniffy-boi-v1.1-firmware
    path: .pio/build/v1_1/firmware.bin
```

---

## Firmware Binary Location

After building, firmware binaries are located at:

```
.pio/build/v1_1/firmware.bin          # Main firmware
.pio/build/v1_1/firmware.elf          # ELF file (for debugging)
.pio/build/v1_1/bootloader.bin        # Bootloader
.pio/build/v1_1/partitions.bin        # Partition table
```

### Manual Flash (without PlatformIO)
```bash
esptool.py --chip esp32s3 \
  --port /dev/ttyACM0 \
  --baud 460800 \
  write_flash \
  0x0 .pio/build/v1_1/bootloader.bin \
  0x8000 .pio/build/v1_1/partitions.bin \
  0x10000 .pio/build/v1_1/firmware.bin
```

---

## Quick Reference Card

| Task | Command |
|------|---------|
| **Build current version** | `pio run` |
| **Upload to device** | `pio run --target upload` |
| **Upload + Monitor** | `pio run --target upload --target monitor` |
| **Build v1.1** | `pio run -e v1_1` |
| **Build v1.1-debug** | `pio run -e v1_1_debug` |
| **Monitor serial** | `pio device monitor` |
| **Clean build** | `pio run --target clean` |
| **List devices** | `pio device list` |

---

**Last Updated:** November 11, 2025
**Current Version:** v1.1.0
**Recommended Build:** `v1_1`
