# Sniffy Boi v1.0 - Project Organization

**Generated:** 2025-11-10

---

## 📁 Current Project Structure

```
/home/user/Projects/esp32/
│
├── 📂 src/                      # Source implementation files
│   ├── main.cpp                 # Entry point (113 lines)
│   ├── CommandInterface.cpp     # Command processor (999 lines)
│   ├── CommandLedger.cpp        # State persistence (380 lines)
│   ├── DisplayManager.cpp       # OLED controller (926 lines)
│   ├── EngineManager.cpp        # Engine orchestration (238 lines)
│   ├── PacketSniffer.cpp        # WiFi capture (847 lines)
│   ├── PMKIDCapture.cpp         # PMKID attacks (375 lines)
│   ├── RFScanner.cpp            # RF engine (445 lines)
│   └── SystemLogger.cpp         # Logging system (191 lines)
│
├── 📂 include/                  # Header files
│   ├── CommandInterface.h       # Command API (139 lines)
│   ├── CommandLedger.h          # State structures (172 lines)
│   ├── config.h                 # Configuration (44 lines)
│   ├── DisplayManager.h         # Display API (77 lines)
│   ├── EngineManager.h          # Engine base (116 lines)
│   ├── NetworkConfig.h          # Legacy config (230 lines)
│   ├── PacketSniffer.h          # Frame structures (226 lines)
│   ├── PMKIDCapture.h           # PMKID structures (136 lines)
│   ├── RFScanner.h              # Scanner API (85 lines)
│   └── SystemLogger.h           # Logger API (162 lines)
│
├── 📂 Docz/                     # 📚 ALL DOCUMENTATION
│   ├── README.md                # Documentation index
│   ├── LANGUAGE_ARCHITECTURE_REPORT.md  # 🆕 Comprehensive analysis
│   ├── CLAUDE.md                # AI development guide
│   ├── COMMAND_INTERFACE_GUIDE.md
│   ├── DEAUTH_ATTACK_GUIDE.md
│   ├── HANDSHAKE_CAPTURE_GUIDE.md
│   ├── INTERACTIVE_COMMAND_FLOW.md
│   ├── PMKID_ATTACK_GUIDE.md
│   ├── PROJECT_STRUCTURE.md
│   └── QUICK_START.md
│
├── 📂 misc/                     # 🔧 UTILITY FILES
│   ├── README.md                # Misc files index
│   ├── sniffy_ios_control.html  # iOS wireless command UI
│   └── test_i2c_scanner.cpp     # OLED debugging tool
│
├── 📄 platformio.ini            # 🔧 Build configuration (OPTIMIZED)
├── 📄 partitions.csv            # Flash partition table
├── 📄 README.md                 # Project overview
└── 📄 PROJECT_ORGANIZATION.md   # 🆕 This file

Total: 5,901 lines of code across 19 files
```

---

## ✨ Recent Changes (2025-11-10)

### 1. ⚡ Firmware Optimizations Applied

**File:** `platformio.ini`

**Changes:**
```ini
# Before
-DCORE_DEBUG_LEVEL=3

# After
-DCORE_DEBUG_LEVEL=1          # Reduced logging
-DCONFIG_BT_ENABLED=0         # Disabled Bluetooth
-DCONFIG_NIMBLE_ENABLED=0     # Disabled NimBLE
```

**Results:**
- **Before:** 856 KB (43.5%)
- **After:** 845 KB (43.0%)
- **Savings:** 11 KB
- **Remaining:** 1,121 KB available for features

---

### 2. 📚 Documentation Reorganization

**Created `Docz/` folder** containing:
- 9 markdown documentation files
- 1 comprehensive architecture report (new)
- README.md index

**Benefits:**
- Cleaner root directory
- Centralized documentation
- Easier navigation

---

### 3. 🔧 Utilities Consolidation

**Created `misc/` folder** containing:
- iOS wireless command interface (HTML)
- I2C scanner testing tool
- README.md with usage instructions

**Benefits:**
- Separated tools from source code
- Preserved testing utilities
- Clear organization

---

## 📊 Current Build Status

```
╔══════════════════════════════════════════════════╗
║        Sniffy Boi v1.0 - Build Metrics          ║
╠══════════════════════════════════════════════════╣
║ Firmware Size:    845 KB / 1,966 KB (43.0%)     ║
║ RAM Usage:        45 KB / 327 KB (13.7%)        ║
║ Flash Available:  1,121 KB                      ║
║                                                  ║
║ Code Files:       19 files                      ║
║ Total Lines:      5,901 LOC                     ║
║ Language:         C++17                         ║
║                                                  ║
║ Build Status:     ✅ SUCCESS                     ║
║ Optimizations:    ✅ APPLIED                     ║
╚══════════════════════════════════════════════════╝
```

---

## 🎯 Development Workflow

### Quick Commands

```bash
# Build firmware
platformio run

# Flash to device
platformio run --target upload

# Serial monitor
platformio device monitor --baud 115200

# View documentation
cd Docz && cat README.md

# Run I2C test
cp misc/test_i2c_scanner.cpp src/main.cpp
platformio run --target upload
```

---

## 📖 Documentation Quick Reference

| Document | Purpose | Location |
|----------|---------|----------|
| Quick Start | Setup guide | Docz/QUICK_START.md |
| Architecture | Code analysis | Docz/LANGUAGE_ARCHITECTURE_REPORT.md |
| Command Guide | API reference | Docz/COMMAND_INTERFACE_GUIDE.md |
| Attack Guides | Tutorials | Docz/*_ATTACK_GUIDE.md |
| iOS Control | Mobile UI | misc/sniffy_ios_control.html |

---

## 🔮 Future Enhancements (Roadmap)

**Available Flash Budget:** 1,121 KB

| Feature | Est. Size | Priority | Effort |
|---------|-----------|----------|--------|
| SD card PCAP logging | 30 KB | High | Medium |
| GPS integration | 50 KB | High | Medium |
| OLED menu system | 20 KB | Medium | Low |
| Auto-attack mode | 15 KB | Medium | Low |
| WPA3 detection | 25 KB | Low | High |

**Total Estimated:** ~140 KB (leaves 981 KB buffer)

---

## 🛠️ Maintenance Notes

### When Adding Features:
1. Implement in `src/` and `include/`
2. Update relevant `Docz/` documentation
3. Test with `platformio run`
4. Check firmware size stays < 1.5 MB

### When Testing Hardware:
1. Use `misc/test_i2c_scanner.cpp` for display issues
2. Check pin connections (A4=SDA, A5=SCL)
3. Verify I2C address (0x3C typical)

### When Debugging:
1. Increase debug level in `platformio.ini`
2. Monitor serial output @ 115200 baud
3. Check OLED for error messages

---

## 📝 Version History

**v1.0 (2025-11-10):**
- ✅ Wardriving platform complete
- ✅ Command interface with state machine
- ✅ Handshake, PMKID, Deauth attacks
- ✅ LittleFS state persistence
- ✅ OLED command menu (IDLE state)
- ✅ Firmware optimizations applied
- ✅ Documentation reorganized

---

## 🔐 Security & Legal

**Authorized Use Only:**
- Security research (with written authorization)
- CTF competitions
- Educational purposes
- Defensive testing

**Prohibited:**
- Unauthorized network access
- Malicious attacks
- Mass targeting
- Detection evasion

**Compliance:** CFAA, local laws, wireless regulations

---

**End of Document**
