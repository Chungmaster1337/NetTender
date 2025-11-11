# Sniffy Boi - WPA Handshake Capture Guide

## What Is This?

Sniffy Boi now includes **complete WPA/WPA2 handshake capture** with automatic nonce extraction, ready for offline password cracking with hashcat.

## Features

### Automatic Handshake Tracking
- **Full 4-way handshake detection** (M1, M2, M3, M4)
- **ANonce extraction** from Message 1 (AP → Client)
- **SNonce extraction** from Message 2 (Client → AP)
- **MIC extraction** from Message 2 for validation
- **EAPOL frame storage** for hashcat export
- **State machine tracking** - knows exactly which messages are captured
- **Multiple concurrent handshakes** - tracks unlimited AP/Client pairs

### Smart Handshake Completion
**Minimum viable handshake:** M1 + M2 (sufficient for cracking)
**Full handshake:** M1 + M2 + M3 + M4 (optimal)

Sniffy marks handshakes as "complete" when:
- You have M1 + M2, OR
- You have M2 + M3

Either combination contains all data needed for offline cracking.

## How Handshake Capture Works

### Passive Capture
Just turn on Sniffy Boi and wait. When a client connects or reconnects to a WPA2 network, the 4-way handshake happens automatically:

1. **M1**: AP sends challenge (ANonce) to client
2. **M2**: Client responds with its nonce (SNonce) + MIC
3. **M3**: AP confirms and sends GTK
4. **M4**: Client acknowledges

Sniffy captures these in real-time and extracts:
- SSID (network name)
- AP MAC address
- Client MAC address
- ANonce (32 bytes)
- SNonce (32 bytes)
- MIC (16 bytes)
- Key version (TKIP=1, AES-CCMP=2)

### Triggered Capture (Advanced)
**Deauth attack** (not yet implemented) forces clients to reconnect, triggering handshake:
```
Client connected → Send deauth → Client reconnects → Capture handshake
```

Current implementation is **passive only** - you wait for natural reconnections.

## Serial Output

### When Handshake Messages Detected
```
[EAPOL] AA:BB:CC:DD:EE:FF <-> 11:22:33:44:55:66 [M1] [NEW HANDSHAKE: MyWiFi] [ANonce extracted]
[EAPOL] 11:22:33:44:55:66 <-> AA:BB:CC:DD:EE:FF [M2] [SNonce + MIC extracted]

╔════════════════════════════════════════════════════════════╗
║          ★★★ COMPLETE HANDSHAKE CAPTURED! ★★★            ║
╚════════════════════════════════════════════════════════════╝
  SSID: MyWiFi
  AP:   AA:BB:CC:DD:EE:FF
  Client: 11:22:33:44:55:66
  Messages: M1=Y M2=Y M3=N M4=N
  Key Version: 2 (AES-CCMP)
════════════════════════════════════════════════════════════
```

### View All Captured Handshakes
Call `sniffer->printHandshakeSummary()` to see:
```
╔═══════════════════════════════════════════════════════════════╗
║  CAPTURED HANDSHAKES: 3
╚═══════════════════════════════════════════════════════════════╝

[1] ✓ COMPLETE
    SSID:   MyHomeWiFi
    AP:     AA:BB:CC:DD:EE:FF
    Client: 11:22:33:44:55:66
    M1:✓ M2:✓ M3:✓ M4:✓ | KeyVer:2 | Age:45s
    [Ready for hashcat export]

[2] ✗ INCOMPLETE
    SSID:   NeighborNet
    AP:     FF:EE:DD:CC:BB:AA
    Client: 66:55:44:33:22:11
    M1:✓ M2:✗ M3:✗ M4:✗ | KeyVer:2 | Age:120s

[3] ✓ COMPLETE
    SSID:   CoffeeShop_5G
    AP:     12:34:56:78:9A:BC
    Client: AA:11:BB:22:CC:33
    M1:✓ M2:✓ M3:✗ M4:✗ | KeyVer:2 | Age:12s
    [Ready for hashcat export]
```

## 🔓 Exporting for Hashcat

### Export Format
Sniffy uses **hashcat mode 22000** format (EAPOL):

```cpp
sniffer->exportHandshakeHashcat(handshake);
```

Output format:
```
WPA*02*455353494448455845*AABBCCDDEEFF*112233445566*ANONCE_HEX*EAPOL_M2_HEX*SNONCE_HEX
```

Where:
- **02** = WPA2 (01 = WPA)
- **455353494448455845** = SSID in hex ("ESSIDHEXE")
- **AABBCCDDEEFF** = AP MAC
- **112233445566** = Client MAC
- **ANONCE_HEX** = 32-byte authenticator nonce
- **EAPOL_M2_HEX** = Full EAPOL M2 frame
- **SNONCE_HEX** = 32-byte supplicant nonce

### Using with Hashcat

1. **Export handshake to file:**
   ```bash
   # From serial monitor, copy the exported hash
   echo "WPA*02*..." > captured.22000
   ```

2. **Crack with hashcat:**
   ```bash
   # Mode 22000 = WPA-PBKDF2-PMKID+EAPOL
   hashcat -m 22000 captured.22000 wordlist.txt

   # With rules
   hashcat -m 22000 captured.22000 rockyou.txt -r best64.rule

   # Brute force 8-digit numeric
   hashcat -m 22000 captured.22000 -a 3 ?d?d?d?d?d?d?d?d
   ```

3. **GPU acceleration:**
   ```bash
   # Show available OpenCL devices
   hashcat -I

   # Use specific GPU
   hashcat -m 22000 -d 1 captured.22000 wordlist.txt
   ```

## 📊 Technical Details

### EAPOL Frame Structure
```
Version:           1 byte
Type:              1 byte  (3 = EAPOL-Key)
Length:            2 bytes
Descriptor Type:   1 byte  (254 or 2 = WPA)
Key Info:          2 bytes (flags: pairwise, MIC, ACK, install)
Key Length:        2 bytes
Replay Counter:    8 bytes
Key Nonce:         32 bytes ← ANonce (M1) or SNonce (M2)
Key IV:            16 bytes
Key RSC:           8 bytes
Key ID:            8 bytes
Key MIC:           16 bytes ← Used for verification
Key Data Length:   2 bytes
Key Data:          variable
```

### Key Info Flags
- **Bit 3**: Pairwise key (1 = pairwise handshake)
- **Bit 6**: Install flag (1 = M3)
- **Bit 7**: ACK (1 = M1/M3, 0 = M2/M4)
- **Bit 8**: MIC (1 = M2/M3/M4)

### Message Identification
```
M1: pairwise=1, ACK=1, MIC=0, install=0
M2: pairwise=1, ACK=0, MIC=1, install=0
M3: pairwise=1, ACK=1, MIC=1, install=1
M4: pairwise=1, ACK=0, MIC=1, install=0
```

## 🔧 Memory Usage

Each handshake consumes:
- **~600 bytes** (MACs, nonces, MIC, EAPOL frames, SSID, metadata)

With **283KB free RAM**, you can store:
- **~470 complete handshakes** before memory issues
- Practically unlimited for typical wardriving sessions

## 🚀 Optimization Tips

### Maximize Handshake Capture Rate

1. **Channel hopping:** Rotate through 1, 6, 11 (least overlap)
   ```cpp
   sniffer->setChannel(1);  // Manually lock to busy channel
   ```

2. **Target busy networks:** More clients = more reconnections = more handshakes

3. **Monitor at peak times:**
   - Morning: 7-9 AM (people arriving at work)
   - Evening: 5-7 PM (people coming home)
   - Lunch: 12-1 PM (mobile devices reconnecting)

4. **Deauth attacks** (when implemented): Force handshakes on demand

### Storage Strategy

**Current:** In-RAM storage (volatile - lost on reboot)

**Next steps:**
- SD card logging (PCAP format)
- Periodic export to serial/SD
- Automatic hashcat file generation

## 📱 Integration with Other Tools

### Airodump-ng Comparison
```
Airodump-ng:  PCAP → aircrack-ng → manual export
Sniffy Boi:   Automatic nonce extraction → ready for hashcat
```

**Advantages:**
- No intermediate tools needed
- Instant hashcat format
- Real-time completion detection
- Portable (ESP32 battery powered)

### Wigle.net Wardriving
Combine with GPS (next feature):
- Capture handshakes while mapping APs
- Export geolocation + handshake data
- Build attack surface database

## 🎓 Educational Use Only

### Legal Warning
**This tool is for authorized testing ONLY:**
- ✅ Your own networks
- ✅ Authorized penetration tests
- ✅ CTF competitions
- ✅ Security research labs
- ❌ Unauthorized network access
- ❌ Capturing neighbor's WiFi
- ❌ Commercial espionage

### Penalties
Unauthorized WiFi hacking violates:
- Computer Fraud and Abuse Act (CFAA) - up to 10 years
- Wiretap Act - criminal penalties
- State computer crime laws

**Use responsibly and ethically.**

## 🛠️ Next Features

1. **SD Card Export** - Save handshakes to SD in PCAP format
2. **Deauth Attack Mode** - Trigger handshakes on demand
3. **PMKID Capture** - Alternative to full handshake (faster)
4. **Handshake Filtering** - Target specific SSIDs/BSSIDs
5. **Auto-export** - Save to SD when complete
6. **OUI Vendor Lookup** - Identify device manufacturers

---

**Happy (ethical) hacking!** 🎯
