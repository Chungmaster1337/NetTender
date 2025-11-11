# Miscellaneous Files (misc)

This folder contains utility files and testing tools that support the Sniffy Boi project but aren't part of the core build.

## 📁 Contents

### **beacon_broadcaster/**
WiFi Testing Module - Python-based beacon broadcaster for testing ESP32 packet sniffing

**Purpose:**
- Broadcast WiFi beacon frames to test Sniffy's detection capabilities
- Three test modes: simple, flood (unique MACs), integrated (with serial monitoring)
- Monitor ESP32 serial output in real-time

**Quick Start:**
```bash
cd beacon_broadcaster
./install.sh                                # One-time setup
sudo ./monitor_mode.sh wlp0s20f3 setup      # Enable monitor mode
sudo python3 sniffy_tester.py -s "TestAP" -i wlp0s20f3 --monitor
```

**Test Modes:**
- **Simple:** Continuous broadcast with single MAC
- **Flood:** 20 beacons with unique MACs (triggers [NEW DEVICE] on ESP32)
- **Integrated:** Broadcast + live ESP32 serial monitoring

See `beacon_broadcaster/README.md` for full documentation.

---

### **sniffy_ios_control.html**
iOS-optimized wireless command interface (web app)

**Purpose:**
- Send wireless commands to Sniffy Boi via WiFi SSID magic packets
- Beautiful mobile-first UI with haptic feedback
- Clipboard integration for easy command injection

**Usage:**
1. Open in Safari on iOS
2. Tap a command button to copy
3. Go to Settings → WiFi → Other...
4. Paste command as network name
5. Select "None" for security and tap "Join"

**Supported Commands:**
- SCAN, STATUS, EXPORT
- HOPPING:ON / HOPPING:OFF
- CHANNEL:N (1-13)
- ATTACK:MAC
- PMKID:MAC

---

### **test_i2c_scanner.cpp**
I2C bus scanner for OLED display debugging

**Purpose:**
- Scan I2C bus (addresses 0x01-0x7F)
- Detect OLED display address (typically 0x3C)
- Validate SDA/SCL connections

**Usage:**
```bash
# Replace src/main.cpp temporarily
cp misc/test_i2c_scanner.cpp src/main.cpp

# Build and upload
platformio run --target upload --target monitor

# Restore original main
git checkout src/main.cpp
```

**Expected Output:**
```
I2C Scanner
Scanning...
Found device at 0x3C (OLED SSD1306)
Scan complete!
```

---

## 🔧 Testing Tools

When troubleshooting hardware issues:
1. Run I2C scanner first to verify display detection
2. Check pin connections (A4=SDA, A5=SCL for Nano ESP32)
3. Verify OLED voltage (3.3V or 5V depending on module)

---

**Last Updated:** 2025-11-10
