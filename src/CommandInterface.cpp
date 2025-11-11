#include "CommandInterface.h"

#define MAGIC_PREFIX "SNIFFY:"
#define MAGIC_PREFIX_LEN 7

#define SESSION_TIMEOUT 120000  // 2 minutes
#define ERROR_DISPLAY_TIME 20000  // 20 seconds
#define CONFIG_DISPLAY_TIME 10000  // 10 seconds
#define COOLDOWN_TIME 60000  // 60 seconds

CommandInterface::CommandInterface(PacketSniffer* sniffer, DisplayManager* display, SystemLogger* logger)
    : sniffer(sniffer),
      display(display),
      logger(logger),
      serial_buffer(""),
      last_prompt_time(0),
      last_display_update(0) {
    ledger = new CommandLedger();
}

void CommandInterface::begin() {
    // Initialize ledger (mounts LittleFS and loads state)
    if (!ledger->begin()) {
        Serial.println("[CommandInterface] Failed to initialize ledger");
        return;
    }

    // Check for previous error on startup
    if (ledger->hasError()) {
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║          ⚠ PREVIOUS ERROR DETECTED                      ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.print("  Error:   ");
        Serial.println(ledger->getLastError());
        Serial.print("  Detail:  ");
        Serial.println(ledger->getLastErrorDetail());
        Serial.println("════════════════════════════════════════════════════════════");

        // Show error on OLED for 5 seconds
        display->showErrorMessage(ledger->getLastError(), ledger->getLastErrorDetail(), 5);
        delay(5000);

        // Clear error from ledger
        ledger->clearError();
    }

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

void CommandInterface::loop() {
    // Check for session timeout (120 seconds)
    if (ledger->isSessionActive()) {
        unsigned long elapsed = millis() - ledger->getSessionStartTime();
        if (elapsed > SESSION_TIMEOUT) {
            handleSessionTimeout();
            return;
        }
    }

    // Check for state-specific timeouts
    CommandState state = ledger->getState();
    unsigned long state_elapsed = millis() - ledger->getStateEnterTime();

    switch (state) {
        case CommandState::ERROR_DISPLAY:
            if (state_elapsed > ERROR_DISPLAY_TIME) {
                // Reset session after error display
                ledger->resetSession();
                display->showOperationalView(logger);
            }
            break;

        case CommandState::CHANNEL_COMPLETE:
        case CommandState::HOPPING_COMPLETE:
            if (state_elapsed > CONFIG_DISPLAY_TIME) {
                // Return to IDLE after config change display
                ledger->setState(CommandState::IDLE);
                display->showOperationalView(logger);
            }
            break;

        case CommandState::SCAN_COMPLETE:
        case CommandState::ATTACK_COMPLETE:
        case CommandState::PMKID_COMPLETE:
        case CommandState::STATUS_DISPLAY:
        case CommandState::EXPORT_COMPLETE:
            if (state_elapsed > COOLDOWN_TIME) {
                // Return to IDLE after cooldown
                ledger->setState(CommandState::IDLE);
                ledger->endSession();
                display->showOperationalView(logger);
            }
            break;

        case CommandState::AWAITING_CHANNEL_VALUE:
        case CommandState::AWAITING_HOPPING_VALUE:
            if (state_elapsed > SESSION_TIMEOUT) {
                handleStateTimeout();
            }
            break;

        default:
            break;
    }

    // Update display periodically (every second)
    if (millis() - last_display_update > 1000) {
        updateDisplay();
        last_display_update = millis();
    }
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
                uint8_t dummy_mac[6] = {0};  // Serial commands don't have source MAC
                Command cmd = parseCommand(serial_buffer, false, dummy_mac);
                processCommand(cmd);
                serial_buffer = "";
                if (ledger->getState() == CommandState::IDLE) {
                    showPrompt();
                }
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

    // Parse and process
    Command cmd = parseCommand(ssid, true, source_mac);
    processCommand(cmd);
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
    } else if (cmdStr == "CHANNEL" || cmdStr == "CH") {
        cmd.type = CommandType::CHANNEL;
    } else if (cmdStr == "HOPPING" || cmdStr == "HOP") {
        cmd.type = CommandType::HOPPING;
    } else if (cmdStr == "BEACON" || cmdStr == "FLOOD" || cmdStr == "SPAM") {
        cmd.type = CommandType::BEACON;
    } else if (cmdStr == "STATUS" || cmdStr == "STATS") {
        cmd.type = CommandType::STATUS;
    } else if (cmdStr == "EXPORT" || cmdStr == "DUMP") {
        cmd.type = CommandType::EXPORT;
    } else if (cmdStr == "CONFIRM" || cmdStr == "YES") {
        cmd.type = CommandType::CONFIRM;
    } else if (cmdStr == "CANCEL" || cmdStr == "ABORT") {
        cmd.type = CommandType::CANCEL;
    } else if (cmdStr == "HELP" || cmdStr == "?") {
        cmd.type = CommandType::HELP;
    } else {
        cmd.type = CommandType::UNKNOWN;
    }

    return cmd;
}

void CommandInterface::processCommand(const Command& cmd) {
    // CANCEL can be used from any state
    if (cmd.type == CommandType::CANCEL) {
        if (validateSession(cmd.source_mac)) {
            Serial.println("✓ Cancelled");
            ledger->resetSession();
            display->showOperationalView(logger);
        }
        return;
    }

    // HELP is always available
    if (cmd.type == CommandType::HELP) {
        // Show help (implementation from old CommandInterface)
        Serial.println("\n╔════════════════════════════════════════════════════════════╗");
        Serial.println("║                  COMMAND REFERENCE                       ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("  DISCOVERY:");
        Serial.println("    scan                  - Scan for APs");
        Serial.println("    status                - Show system status");
        Serial.println();
        Serial.println("  ATTACKS:");
        Serial.println("    attack <MAC>          - Deauth attack (auto-locks channel)");
        Serial.println("    pmkid <MAC>           - Clientless PMKID attack");
        Serial.println();
        Serial.println("  CONFIGURATION:");
        Serial.println("    channel               - Show current channel");
        Serial.println("    channel <1-13>        - Lock to channel (disables hopping)");
        Serial.println("    hopping               - Show hopping status");
        Serial.println("    hopping <ON|OFF>      - Enable/disable channel hopping");
        Serial.println();
        Serial.println("  EXPORT:");
        Serial.println("    export                - Export captures (hashcat format)");
        Serial.println();
        Serial.println("  CONTROL:");
        Serial.println("    cancel                - Cancel current operation");
        Serial.println("════════════════════════════════════════════════════════════");
        return;
    }

    // Route to state-specific handler
    CommandState state = ledger->getState();

    switch (state) {
        case CommandState::IDLE:
            executeCommand(cmd);
            break;

        case CommandState::AWAITING_CHANNEL_VALUE:
            handleAwaitingChannelValue(cmd);
            break;

        case CommandState::AWAITING_HOPPING_VALUE:
            handleAwaitingHoppingValue(cmd);
            break;

        default:
            // Commands not allowed during execution/cooldown
            Serial.println("❌ Command not available in current state");
            Serial.println("   Use 'cancel' to abort current operation");
            break;
    }
}

bool CommandInterface::validateSession(const uint8_t* source_mac) {
    if (!ledger->isSessionActive()) return true;  // No session, allow anyone

    // Check MAC match
    if (!ledger->isAuthorizedMAC(source_mac)) {
        Serial.println("❌ UNAUTHORIZED MAC");
        Serial.print("   Session locked to: ");
        printMACAddress(ledger->getAuthorizedMAC());
        Serial.println();

        display->showSessionLocked(ledger->getAuthorizedMAC());
        return false;
    }

    return true;
}

bool CommandInterface::validateTarget(const uint8_t* target_mac, APInfo& out_info) {
    // Check if target is in scan results
    if (!ledger->findAP(target_mac, out_info)) {
        showError("TARGET NOT FOUND", "Run SCAN first");
        return false;
    }

    // Check if target is same as source (self-attack prevention)
    if (memcmp(target_mac, ledger->getAuthorizedMAC(), 6) == 0) {
        showError("SELF-ATTACK BLOCKED", "Cannot attack command source");
        return false;
    }

    return true;
}

bool CommandInterface::validateChannel(int channel) {
    if (channel < 1 || channel > 13) {
        showError("INVALID CHANNEL", "Must be 1-13");
        return false;
    }
    return true;
}

void CommandInterface::executeCommand(const Command& cmd) {
    // Start session if not active
    if (!ledger->isSessionActive() && cmd.is_wireless) {
        ledger->startSession(cmd.source_mac);
    }

    // Validate session for wireless commands
    if (cmd.is_wireless && !validateSession(cmd.source_mac)) {
        return;
    }

    switch (cmd.type) {
        case CommandType::SCAN:
            handleScan(cmd);
            break;
        case CommandType::ATTACK:
            handleAttack(cmd);
            break;
        case CommandType::PMKID:
            handlePMKID(cmd);
            break;
        case CommandType::CHANNEL:
            handleChannel(cmd);
            break;
        case CommandType::HOPPING:
            handleHopping(cmd);
            break;
        case CommandType::BEACON:
            handleBeacon(cmd);
            break;
        case CommandType::STATUS:
            handleStatus(cmd);
            break;
        case CommandType::EXPORT:
            handleExport(cmd);
            break;
        case CommandType::UNKNOWN:
            Serial.println("❌ Unknown command. Type 'help' for available commands.");
            break;
        default:
            break;
    }
}

// ==================== Command Handlers (from IDLE state) ====================

void CommandInterface::handleScan(const Command& cmd) {
    Serial.println("✓ Starting AP scan...");

    // Transition to SCAN_EXECUTING
    ledger->setState(CommandState::SCAN_EXECUTING);

    // Execute scan
    executeScan();
}

void CommandInterface::handleAttack(const Command& cmd) {
    if (cmd.param1.length() == 0) {
        showError("MISSING TARGET", "Usage: attack <MAC>");
        return;
    }

    uint8_t target_mac[6];
    if (!parseMACAddress(cmd.param1, target_mac)) {
        showError("INVALID MAC", "Format: AABBCCDDEEFF");
        return;
    }

    APInfo target_info;
    if (!validateTarget(target_mac, target_info)) {
        return;
    }

    Serial.println("✓ Starting attack...");

    // Auto-lock to target AP's channel and disable hopping
    Serial.print("  Locking to channel ");
    Serial.println(target_info.channel);
    sniffer->setChannel(target_info.channel);
    sniffer->channelHop(false);
    ledger->setChannel(target_info.channel);
    ledger->setHopping(false);

    // Transition to ATTACK_EXECUTING
    ledger->setState(CommandState::ATTACK_EXECUTING);

    // Execute attack
    executeAttack(target_mac);
}

void CommandInterface::handlePMKID(const Command& cmd) {
    if (cmd.param1.length() == 0) {
        showError("MISSING TARGET", "Usage: pmkid <MAC>");
        return;
    }

    uint8_t target_mac[6];
    if (!parseMACAddress(cmd.param1, target_mac)) {
        showError("INVALID MAC", "Format: AABBCCDDEEFF");
        return;
    }

    APInfo target_info;
    if (!validateTarget(target_mac, target_info)) {
        return;
    }

    Serial.println("✓ Starting PMKID attack...");

    // Auto-lock to target AP's channel
    Serial.print("  Locking to channel ");
    Serial.println(target_info.channel);
    sniffer->setChannel(target_info.channel);
    sniffer->channelHop(false);
    ledger->setChannel(target_info.channel);
    ledger->setHopping(false);

    // Transition to PMKID_EXECUTING
    ledger->setState(CommandState::PMKID_EXECUTING);

    // Execute PMKID attack
    executePMKID(target_mac);
}

void CommandInterface::handleChannel(const Command& cmd) {
    // No parameter = query current channel
    if (cmd.param1.length() == 0) {
        String current = String(ledger->getCurrentChannel());
        display->showAwaitingValue("CHANNEL", current, "1-13");

        ledger->setState(CommandState::AWAITING_CHANNEL_VALUE);

        Serial.print("Current channel: ");
        Serial.println(current);
        Serial.println("Send: SNIFFY:CHANNEL:<1-13>");
        return;
    }

    // Has parameter = set channel immediately
    int channel = cmd.param1.toInt();
    if (!validateChannel(channel)) {
        return;
    }

    executeChannelChange(channel);
}

void CommandInterface::handleHopping(const Command& cmd) {
    // No parameter = query current state
    if (cmd.param1.length() == 0) {
        String current = ledger->isHoppingEnabled() ? "ON" : "OFF";
        display->showAwaitingValue("HOPPING", current, "ON/OFF");

        ledger->setState(CommandState::AWAITING_HOPPING_VALUE);

        Serial.print("Channel hopping: ");
        Serial.println(current);
        Serial.println("Send: SNIFFY:HOPPING:<ON|OFF>");
        return;
    }

    // Has parameter = set immediately
    String value = cmd.param1;
    value.toUpperCase();

    bool enable;
    if (value == "ON" || value == "1" || value == "ENABLE" || value == "TRUE") {
        enable = true;
    } else if (value == "OFF" || value == "0" || value == "DISABLE" || value == "FALSE") {
        enable = false;
    } else {
        showError("INVALID VALUE", "Use ON or OFF");
        return;
    }

    executeHoppingToggle(enable);
}

void CommandInterface::handleStatus(const Command& cmd) {
    ledger->setState(CommandState::STATUS_DISPLAY);

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

    Serial.print("  APs Found:    ");
    Serial.println(ledger->getAPCount());

    Serial.print("  Handshakes:   ");
    Serial.println(sniffer->getHandshakeCount());

    Serial.print("  Channel:      ");
    Serial.println(ledger->getCurrentChannel());

    Serial.print("  Hopping:      ");
    Serial.println(ledger->isHoppingEnabled() ? "ON" : "OFF");

    Serial.println("════════════════════════════════════════════════════════════");

    // Show on OLED
    std::vector<String> results;
    results.push_back("APs: " + String(ledger->getAPCount()));
    results.push_back("Handshakes: " + String(sniffer->getHandshakeCount()));
    results.push_back("Ch:" + String(ledger->getCurrentChannel()) + " Hop:" + (ledger->isHoppingEnabled() ? "ON" : "OFF"));
    display->showCooldownResults("STATUS", results, 60);
}

void CommandInterface::handleExport(const Command& cmd) {
    ledger->setState(CommandState::EXPORT_EXECUTING);

    const auto& handshakes = sniffer->getHandshakes();

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║              HASHCAT EXPORT (MODE 22000)                 ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();

    if (handshakes.empty()) {
        Serial.println("  No handshakes captured yet.");
        ledger->setState(CommandState::EXPORT_COMPLETE);
        ledger->setOperationResult(false, "No handshakes");
        display->showCommandResult("EXPORT", false, "No handshakes", 0);
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

    ledger->setState(CommandState::EXPORT_COMPLETE);
    ledger->setOperationResult(true, "Exported to serial");

    std::vector<String> results;
    results.push_back("Exported " + String(handshakes.size()) + " handshakes");
    results.push_back("Check serial output");
    display->showCooldownResults("EXPORT", results, 60);
}

void CommandInterface::handleBeacon(const Command& cmd) {
    // Check if beacon flood is already active - toggle behavior
    if (sniffer->isBeaconFloodActive()) {
        // Stop beacon flood
        sniffer->stopBeaconFlood();

        ledger->setState(CommandState::BEACON_COMPLETE);
        ledger->setOperationResult(true, "Beacon flood stopped");

        Serial.println("✓ Beacon flood stopped");

        std::vector<String> results;
        results.push_back("Flood stopped");
        display->showCooldownResults("BEACON", results, 60);
        return;
    }

    // Start beacon flood
    ledger->setState(CommandState::BEACON_EXECUTING);

    // Parse optional channel parameter (default to current channel)
    uint8_t channel = ledger->getCurrentChannel();
    if (cmd.param1.length() > 0) {
        int ch = cmd.param1.toInt();
        if (validateChannel(ch)) {
            channel = ch;
        } else {
            return;  // validateChannel already showed error
        }
    }

    Serial.println("✓ Starting beacon flood attack...");
    Serial.print("  Channel: ");
    Serial.println(channel);
    Serial.println("  ⚠️  AUTHORIZED USE ONLY - Security Research/CTF");

    // Lock to target channel and disable hopping
    sniffer->setChannel(channel);
    sniffer->channelHop(false);
    ledger->setChannel(channel);
    ledger->setHopping(false);

    // Start beacon flood
    sniffer->startBeaconFlood(channel);

    // Simulate brief execution animation
    int progress = 0;
    while (progress < 100) {
        progress += 20;
        int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
        display->showCommandExecuting("BEACON FLOOD", timeout_remaining, progress);
        delay(100);
    }

    ledger->setState(CommandState::BEACON_COMPLETE);
    ledger->setOperationResult(true, "Beacon flood active on Ch" + String(channel));

    Serial.println("✓ Beacon flood active!");
    Serial.println("  Send BEACON again to stop");

    std::vector<String> results;
    results.push_back("Flood active");
    results.push_back("Ch:" + String(channel));
    results.push_back("Send BEACON to stop");
    display->showCooldownResults("BEACON", results, 60);
}

// ==================== State-Specific Handlers ====================

void CommandInterface::handleAwaitingChannelValue(const Command& cmd) {
    if (!validateSession(cmd.source_mac)) return;

    // Expecting CHANNEL:<N> command
    if (cmd.type == CommandType::CHANNEL && cmd.param1.length() > 0) {
        int channel = cmd.param1.toInt();
        if (!validateChannel(channel)) {
            return;
        }

        executeChannelChange(channel);
    } else {
        Serial.println("❌ Expected: CHANNEL:<1-13>");
    }
}

void CommandInterface::handleAwaitingHoppingValue(const Command& cmd) {
    if (!validateSession(cmd.source_mac)) return;

    // Expecting HOPPING:<ON|OFF> command
    if (cmd.type == CommandType::HOPPING && cmd.param1.length() > 0) {
        String value = cmd.param1;
        value.toUpperCase();

        bool enable;
        if (value == "ON" || value == "1" || value == "ENABLE") {
            enable = true;
        } else if (value == "OFF" || value == "0" || value == "DISABLE") {
            enable = false;
        } else {
            showError("INVALID VALUE", "Use ON or OFF");
            return;
        }

        executeHoppingToggle(enable);
    } else {
        Serial.println("❌ Expected: HOPPING:<ON|OFF>");
    }
}

// ==================== Operation Executors ====================

void CommandInterface::executeScan() {
    Serial.println("[CommandInterface] Executing scan...");

    // Clear old scan results
    ledger->clearScanResults();

    // Enable channel hopping for scan (to find APs on all channels)
    sniffer->channelHop(true);
    ledger->setHopping(true);

    // Simulate scanning progress (in real implementation, RFScanner fills this)
    // For now, we'll use existing device data from PacketSniffer
    unsigned long scan_start = millis();
    unsigned long scan_duration = 15000;  // 15 seconds

    while (millis() - scan_start < scan_duration) {
        int progress = ((millis() - scan_start) * 100) / scan_duration;
        ledger->setOperationProgress(progress);

        int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
        display->showCommandExecuting("SCANNING", timeout_remaining, progress);

        delay(500);
    }

    // Copy devices from PacketSniffer to ledger
    const auto& devices = sniffer->getDevices();
    int ap_count = 0;

    for (const auto& device : devices) {
        if (device.second.is_ap) {
            int channel = device.second.channels.size() > 0 ? device.second.channels[0] : 0;
            ledger->addAP(device.second.mac, device.second.ssid, channel,
                         device.second.max_rssi, device.second.encryption_type);
            ap_count++;
        }
    }

    onScanComplete(ap_count);
}

void CommandInterface::executeAttack(const uint8_t* target_mac) {
    Serial.println("[CommandInterface] Executing attack...");

    // Send deauth broadcast
    sniffer->sendDeauthBroadcast(target_mac);

    // Simulate attack progress
    unsigned long attack_start = millis();
    unsigned long attack_duration = 10000;  // 10 seconds monitoring

    bool handshake_captured = false;

    while (millis() - attack_start < attack_duration) {
        int progress = ((millis() - attack_start) * 100) / attack_duration;
        ledger->setOperationProgress(progress);

        int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
        display->showCommandExecuting("ATTACKING", timeout_remaining, progress);

        // Check if handshake was captured
        // (In real implementation, PacketSniffer would set a flag)
        // For now, we'll check if handshakes exist for this target
        const auto& handshakes = sniffer->getHandshakes();
        for (const auto& hs : handshakes) {
            if (memcmp(hs.ap_mac, target_mac, 6) == 0) {
                handshake_captured = true;
                break;
            }
        }

        if (handshake_captured) break;

        delay(500);
    }

    if (handshake_captured) {
        onAttackComplete(true, "Handshake captured!");
    } else {
        onAttackComplete(false, "No handshake captured");
    }
}

void CommandInterface::executePMKID(const uint8_t* target_mac) {
    Serial.println("[CommandInterface] Executing PMKID attack...");

    // PMKID attack implementation (placeholder)
    Serial.println("❌ PMKID attack not fully implemented yet");

    // Simulate progress
    unsigned long attack_start = millis();
    unsigned long attack_duration = 10000;

    while (millis() - attack_start < attack_duration) {
        int progress = ((millis() - attack_start) * 100) / attack_duration;
        ledger->setOperationProgress(progress);

        int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
        display->showCommandExecuting("PMKID ATTACK", timeout_remaining, progress);

        delay(500);
    }

    onPMKIDComplete(false, "Not implemented");
}

void CommandInterface::executeChannelChange(int channel) {
    ledger->setState(CommandState::CHANNEL_EXECUTING);

    String old_val = String(ledger->getCurrentChannel());

    // Apply channel change
    sniffer->setChannel(channel);
    sniffer->channelHop(false);  // Disable hopping when locking channel
    ledger->setChannel(channel);
    ledger->setHopping(false);

    // Simulate brief execution
    int progress = 0;
    while (progress < 100) {
        progress += 25;
        int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
        display->showCommandExecuting("CHANNEL", timeout_remaining, progress);
        delay(200);
    }

    onConfigChange("CHANNEL", old_val, String(channel));
}

void CommandInterface::executeHoppingToggle(bool enable) {
    ledger->setState(CommandState::HOPPING_EXECUTING);

    String old_val = ledger->isHoppingEnabled() ? "ON" : "OFF";

    // Apply hopping change
    sniffer->channelHop(enable);
    ledger->setHopping(enable);

    // Simulate brief execution
    int progress = 0;
    while (progress < 100) {
        progress += 25;
        int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
        display->showCommandExecuting("HOPPING", timeout_remaining, progress);
        delay(200);
    }

    onConfigChange("HOPPING", old_val, enable ? "ON" : "OFF");
}

// ==================== Completion Handlers ====================

void CommandInterface::onScanComplete(int ap_count) {
    ledger->setState(CommandState::SCAN_COMPLETE);
    ledger->setOperationResult(true, String(ap_count) + " APs found");

    Serial.println("✓ Scan complete");
    Serial.print("  Found ");
    Serial.print(ap_count);
    Serial.println(" APs");

    // List APs
    const auto& aps = ledger->getAPList();
    for (int i = 0; i < aps.size() && i < 5; i++) {
        Serial.print("    [");
        Serial.print(i);
        Serial.print("] ");
        printMACAddress(aps[i].mac);
        Serial.print(" | ");
        Serial.print(aps[i].ssid.length() > 0 ? aps[i].ssid : "(hidden)");
        Serial.print(" | Ch:");
        Serial.println(aps[i].channel);
    }

    if (aps.size() > 5) {
        Serial.print("    ... and ");
        Serial.print(aps.size() - 5);
        Serial.println(" more");
    }

    // Show on OLED
    std::vector<String> results;
    for (int i = 0; i < aps.size() && i < 3; i++) {
        String line = aps[i].ssid;
        if (line.length() > 15) line = line.substring(0, 15);
        if (line.length() == 0) line = "(hidden)";
        line += " Ch" + String(aps[i].channel);
        results.push_back(line);
    }

    display->showCooldownResults("SCAN: " + String(ap_count) + " APs", results, 60);
}

void CommandInterface::onAttackComplete(bool success, const String& message) {
    ledger->setState(CommandState::ATTACK_COMPLETE);
    ledger->setOperationResult(success, message);

    if (success) {
        Serial.println("✓ Attack successful");
    } else {
        Serial.println("⚠ Attack completed");
    }
    Serial.print("  ");
    Serial.println(message);

    display->showCommandResult("ATTACK", success, message, 0);
}

void CommandInterface::onPMKIDComplete(bool success, const String& message) {
    ledger->setState(CommandState::PMKID_COMPLETE);
    ledger->setOperationResult(success, message);

    if (success) {
        Serial.println("✓ PMKID attack successful");
    } else {
        Serial.println("⚠ PMKID attack completed");
    }
    Serial.print("  ");
    Serial.println(message);

    display->showCommandResult("PMKID", success, message, 0);
}

void CommandInterface::onConfigChange(const String& setting, const String& old_val, const String& new_val) {
    ledger->setState(setting == "CHANNEL" ? CommandState::CHANNEL_COMPLETE : CommandState::HOPPING_COMPLETE);
    ledger->setOperationResult(true, "Changed to " + new_val);

    Serial.print("✓ ");
    Serial.print(setting);
    Serial.print(" changed: ");
    Serial.print(old_val);
    Serial.print(" → ");
    Serial.println(new_val);

    display->showConfigComparison(setting, old_val, new_val, 10);
}

// ==================== Error Handling ====================

void CommandInterface::showError(const String& error, const String& detail) {
    Serial.println("❌ ERROR: " + error);
    Serial.println("   " + detail);

    // Store error in ledger
    ledger->setError(error, detail);
    ledger->setState(CommandState::ERROR_DISPLAY);

    display->showErrorMessage(error, detail, 20);
}

void CommandInterface::handleSessionTimeout() {
    Serial.println("⚠ Session timeout (120 seconds)");
    ledger->resetSession();
    display->showOperationalView(logger);
}

void CommandInterface::handleStateTimeout() {
    Serial.println("⚠ State timeout - returning to IDLE");
    ledger->resetSession();
    display->showOperationalView(logger);
}

// ==================== Display Updates ====================

void CommandInterface::updateDisplay() {
    CommandState state = ledger->getState();
    unsigned long elapsed = millis() - ledger->getStateEnterTime();

    switch (state) {
        case CommandState::SCAN_EXECUTING:
        case CommandState::ATTACK_EXECUTING:
        case CommandState::PMKID_EXECUTING:
        case CommandState::CHANNEL_EXECUTING:
        case CommandState::HOPPING_EXECUTING: {
            int timeout_remaining = SESSION_TIMEOUT / 1000 - ((millis() - ledger->getSessionStartTime()) / 1000);
            int progress = ledger->getOperationProgress();

            String cmd_name;
            if (state == CommandState::SCAN_EXECUTING) cmd_name = "SCANNING";
            else if (state == CommandState::ATTACK_EXECUTING) cmd_name = "ATTACKING";
            else if (state == CommandState::PMKID_EXECUTING) cmd_name = "PMKID";
            else if (state == CommandState::CHANNEL_EXECUTING) cmd_name = "CHANNEL";
            else cmd_name = "HOPPING";

            display->showCommandExecuting(cmd_name, timeout_remaining, progress);
            break;
        }

        case CommandState::SCAN_COMPLETE:
        case CommandState::STATUS_DISPLAY:
        case CommandState::EXPORT_COMPLETE: {
            int countdown = (COOLDOWN_TIME - elapsed) / 1000;
            // Display already updated in completion handlers
            break;
        }

        case CommandState::CHANNEL_COMPLETE:
        case CommandState::HOPPING_COMPLETE: {
            int countdown = (CONFIG_DISPLAY_TIME - elapsed) / 1000;
            // Display already updated in completion handlers
            break;
        }

        case CommandState::ERROR_DISPLAY: {
            int countdown = (ERROR_DISPLAY_TIME - elapsed) / 1000;
            // Display already updated in showError
            break;
        }

        default:
            break;
    }
}

// ==================== Helper Functions ====================

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

String CommandInterface::macToString(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void CommandInterface::showPrompt() {
    Serial.print("\nsniffy> ");
    last_prompt_time = millis();
}
