# Sniffy Boi - Quick Start Guide

## ⚡ How to Use (It's Passive!)

**IMPORTANT**: Sniffy Boi is a **passive capture tool**. You don't type commands - it automatically captures and displays handshakes as they're detected.

---

## Step 1: Flash Firmware

```bash
cd /home/user/esp32
~/.platformio/penv/bin/platformio run --target upload
```

Wait for: `Hard resetting via RTS pin...`

---

## Step 2: Open Serial Monitor

```bash
~/.platformio/penv/bin/platformio device monitor --baud 115200
```

**What you'll see immediately:**

```
╔════════════════════════════════════════════════════════════╗
║                   SNIFFY BOI v1.0                        ║
║              Wardriving & WPA2 Attack Platform           ║
╚════════════════════════════════════════════════════════════╝

  Mode:        Monitor (Standalone)
  Attacks:     Handshake | PMKID | Deauth
  Output:      Hashcat mode 22000
  Network:     DISCONNECTED (no AP connection required)

════════════════════════════════════════════════════════════

[Main] Initializing display...
[Main] Initializing logger...
[Main] Initializing Sniffy Boi...
[Main] Sniffy Boi ready!
========================================

[Main] WiFi configured for monitor mode (no network connection)
[Main] Setup complete. Entering main loop...
========================================

[RFScanner] Starting in passive mode...
[RFScanner] Channel hopping enabled
[RFScanner] Monitoring all channels (1-13)
```

---

## Step 3: Wait for Automatic Captures

**The device will AUTOMATICALLY print when it finds:**

### Beacon Frames (APs discovered)
```
[BEACON] AA:BB:CC:DD:EE:FF | "HomeNetwork" | Ch:6 | WPA2 | -45dBm
[BEACON] 11:22:33:44:55:66 | "NeighborWiFi" | Ch:11 | WPA2 | -62dBm
```

### EAPOL Handshakes (Automatic!)
```
[EAPOL] Client: 11:22:33:44:55:66 <-> AP: AA:BB:CC:DD:EE:FF [M1]
[EAPOL] Client: 11:22:33:44:55:66 <-> AP: AA:BB:CC:DD:EE:FF [M2]

╔════════════════════════════════════════════════════════════╗
║          ★★★ COMPLETE HANDSHAKE CAPTURED! ★★★            ║
╚════════════════════════════════════════════════════════════╝
  SSID:     HomeNetwork
  AP:       AA:BB:CC:DD:EE:FF
  Client:   11:22:33:44:55:66
  Channel:  6
  RSSI:     -45 dBm
  Messages: M1=Y M2=Y M3=N M4=N
  Key Ver:  2 (AES-CCMP)

✓ Ready for hashcat cracking!

Hashcat 22000 format:
WPA*02*AABBCCDDEEFF*112233445566*486F6D654E6574776F726B*...
════════════════════════════════════════════════════════════
```

### PMKID Captures (Automatic!)
```
╔════════════════════════════════════════════════════════════╗
║            ★★★ PMKID CAPTURED! ★★★                       ║
╚════════════════════════════════════════════════════════════╝
  SSID:     GuestNetwork
  AP:       11:22:33:44:55:66
  Client:   AA:BB:CC:DD:EE:FF (fake/clientless)
  Channel:  11
  RSSI:     -55 dBm
  PMKID:    1A2B3C4D5E6F7A8B9C0D1E2F3A4B5C6D

✓ Ready for hashcat cracking (mode 22000)
════════════════════════════════════════════════════════════
```

---

## What You're Doing Wrong

❌ **DON'T DO THIS:**
```
# In serial monitor, typing:
sniffer->exportHandshakeHashcat(handshake);
sniffer->printHandshakeSummary();
```

This doesn't work because **there's no command interpreter**. Those are C++ function calls, not commands!

✅ **DO THIS INSTEAD:**
1. Flash firmware
2. Open serial monitor
3. **WAIT** - device automatically captures and prints
4. When you see "COMPLETE HANDSHAKE CAPTURED", copy the hashcat line

---

## How to Manually Trigger Captures

Since handshakes only happen when clients **connect** to APs, you have options:

### Option 1: Passive (Wait for Natural Reconnections)
- Client device goes to sleep
- Client moves out of range then back
- Router reboots
- Wait 10-30 minutes near busy WiFi

### Option 2: Active (Deauth Attack - NOT YET IMPLEMENTED IN UI)

**Currently you'd need to modify RFScanner.cpp to call:**
```cpp
// In src/RFScanner.cpp loop() function, add:
uint8_t target_ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
uint8_t target_client[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
sniffer->triggerHandshake(target_ap, target_client, 5);
```

But this requires recompiling. **We need to build a menu system next!**

---

## Expected Output Timeline

**First 10 seconds:**
- Boot messages
- Engine initialization
- "Setup complete"

**10-60 seconds:**
- Beacon frames appear (every AP in range)
- Channel hopping messages
- Device discovery

**1-10 minutes:**
- EAPOL frames (if clients connecting nearby)
- PMKID captures (if APs support it)
- Deauth frames (if attacks happening nearby)

**Nothing happening?**
- Move near a busy coffee shop / airport / apartment complex
- Or implement deauth attacks to force handshakes
- Or trigger reconnections (unplug your router, replug it)

---

## Common Issues

### "I only see hex codes like 03"
- **Cause**: You're typing in the serial monitor
- **Fix**: Don't type anything! Just watch the output

### "No handshakes after 10 minutes"
- **Cause**: No clients connecting to APs
- **Fix**: Use deauth attack (requires menu system) or move to busier location

### "Device keeps rebooting"
- **Cause**: Power supply insufficient (ESP32 draws 200-500mA)
- **Fix**: Use USB 2.0/3.0 port, not USB hub

### "Only seeing beacons, no EAPOL"
- **Normal**: Handshakes only happen during client connections (rare event)
- **Solution**: Implement deauth attack menu (next feature)

---

## Next Steps

**What we need to build:**
1. **Serial command interface** - Type commands like:
   ```
   > scan                    # Show discovered APs
   > attack 0 5              # Deauth AP #0, 5 bursts
   > pmkid 0                 # Clientless PMKID attack on AP #0
   > export                  # Print all hashcat hashes
   ```

2. **OLED menu system** - Navigate with buttons (if you add them)

3. **SD card logging** - Auto-save captures to files

**Want me to build the serial command interface now?** This would let you:
- Type `scan` to list APs
- Type `attack <ap_id>` to deauth and capture handshakes
- Type `export` to dump all captures
- Type `stats` to see capture counts
