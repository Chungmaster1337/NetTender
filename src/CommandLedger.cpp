#include "CommandLedger.h"
#include "Utils.h"

CommandLedger::CommandLedger()
    : session_active(false),
      session_start_time(0),
      current_state(CommandState::IDLE),
      state_enter_time(0),
      last_scan_time(0),
      current_channel(1),
      previous_channel(1),
      hopping_enabled(true),
      previous_hopping_enabled(true),
      operation_progress(0),
      operation_success(false),
      operation_message(""),
      pending_target_set(false),
      has_error(false),
      last_error(""),
      last_error_detail(""),
      error_time(0) {
    memset(authorized_mac, 0, 6);
    memset(pending_target_mac, 0, 6);
}

bool CommandLedger::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[CommandLedger] Failed to mount LittleFS");
        return false;
    }

    Serial.println("[CommandLedger] LittleFS mounted successfully");

    // Load existing ledger if present
    if (LittleFS.exists(LEDGER_PATH)) {
        load();
        Serial.println("[CommandLedger] Loaded existing ledger");
    } else {
        Serial.println("[CommandLedger] No existing ledger, starting fresh");
        save();  // Create initial ledger file
    }

    return true;
}

void CommandLedger::save() {
    File file = LittleFS.open(LEDGER_PATH, "w");
    if (!file) {
        Serial.println("[CommandLedger] Failed to open ledger for writing");
        return;
    }

    // Session state
    file.printf("session_active=%d\n", session_active ? 1 : 0);
    file.printf("authorized_mac=%s\n", macToString(authorized_mac).c_str());
    file.printf("session_start_time=%lu\n", session_start_time);

    // Current state
    file.printf("current_state=%s\n", stateToString(current_state).c_str());
    file.printf("state_enter_time=%lu\n", state_enter_time);

    // Scan results
    file.printf("last_scan_time=%lu\n", last_scan_time);
    file.printf("ap_count=%d\n", ap_list.size());
    for (const auto& ap : ap_list) {
        file.printf("ap=%s,%s,%d,%d,%d\n",
                    macToString(ap.mac).c_str(),
                    ap.ssid.c_str(),
                    ap.channel,
                    ap.rssi,
                    ap.encryption);
    }

    // Configuration
    file.printf("current_channel=%d\n", current_channel);
    file.printf("previous_channel=%d\n", previous_channel);
    file.printf("hopping_enabled=%d\n", hopping_enabled ? 1 : 0);
    file.printf("previous_hopping_enabled=%d\n", previous_hopping_enabled ? 1 : 0);

    // Operation tracking
    file.printf("operation_progress=%d\n", operation_progress);
    file.printf("operation_success=%d\n", operation_success ? 1 : 0);
    file.printf("operation_message=%s\n", operation_message.c_str());

    // Pending operation
    file.printf("pending_target_set=%d\n", pending_target_set ? 1 : 0);
    if (pending_target_set) {
        file.printf("pending_target_mac=%s\n", macToString(pending_target_mac).c_str());
    }

    // Error tracking
    file.printf("has_error=%d\n", has_error ? 1 : 0);
    if (has_error) {
        file.printf("last_error=%s\n", last_error.c_str());
        file.printf("last_error_detail=%s\n", last_error_detail.c_str());
        file.printf("error_time=%lu\n", error_time);
    }

    file.close();
    Serial.println("[CommandLedger] Ledger saved");
}

void CommandLedger::load() {
    File file = LittleFS.open(LEDGER_PATH, "r");
    if (!file) {
        Serial.println("[CommandLedger] Failed to open ledger for reading");
        return;
    }

    ap_list.clear();

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() == 0) continue;

        int eq_pos = line.indexOf('=');
        if (eq_pos == -1) continue;

        String key = line.substring(0, eq_pos);
        String value = line.substring(eq_pos + 1);

        // Parse key-value pairs
        if (key == "session_active") {
            session_active = (value.toInt() == 1);
        } else if (key == "authorized_mac") {
            stringToMAC(value, authorized_mac);
        } else if (key == "session_start_time") {
            session_start_time = value.toInt();
        } else if (key == "current_state") {
            current_state = stringToState(value);
        } else if (key == "state_enter_time") {
            state_enter_time = value.toInt();
        } else if (key == "last_scan_time") {
            last_scan_time = value.toInt();
        } else if (key == "ap") {
            // Parse AP entry: MAC,SSID,Channel,RSSI,Encryption
            int comma1 = value.indexOf(',');
            int comma2 = value.indexOf(',', comma1 + 1);
            int comma3 = value.indexOf(',', comma2 + 1);
            int comma4 = value.indexOf(',', comma3 + 1);

            if (comma1 > 0 && comma2 > 0 && comma3 > 0 && comma4 > 0) {
                APInfo ap;
                stringToMAC(value.substring(0, comma1), ap.mac);
                ap.ssid = value.substring(comma1 + 1, comma2);
                ap.channel = value.substring(comma2 + 1, comma3).toInt();
                ap.rssi = value.substring(comma3 + 1, comma4).toInt();
                ap.encryption = value.substring(comma4 + 1).toInt();
                ap_list.push_back(ap);
            }
        } else if (key == "current_channel") {
            current_channel = value.toInt();
        } else if (key == "previous_channel") {
            previous_channel = value.toInt();
        } else if (key == "hopping_enabled") {
            hopping_enabled = (value.toInt() == 1);
        } else if (key == "previous_hopping_enabled") {
            previous_hopping_enabled = (value.toInt() == 1);
        } else if (key == "operation_progress") {
            operation_progress = value.toInt();
        } else if (key == "operation_success") {
            operation_success = (value.toInt() == 1);
        } else if (key == "operation_message") {
            operation_message = value;
        } else if (key == "pending_target_set") {
            pending_target_set = (value.toInt() == 1);
        } else if (key == "pending_target_mac") {
            stringToMAC(value, pending_target_mac);
        } else if (key == "has_error") {
            has_error = (value.toInt() == 1);
        } else if (key == "last_error") {
            last_error = value;
        } else if (key == "last_error_detail") {
            last_error_detail = value;
        } else if (key == "error_time") {
            error_time = value.toInt();
        }
    }

    file.close();
    Serial.println("[CommandLedger] Ledger loaded");
}

void CommandLedger::resetSession() {
    session_active = false;
    memset(authorized_mac, 0, 6);
    current_state = CommandState::IDLE;
    operation_progress = 0;
    operation_success = false;
    operation_message = "";
    pending_target_set = false;
    memset(pending_target_mac, 0, 6);

    // Note: Does NOT clear scan results or configuration
    save();
}

void CommandLedger::clearScanResults() {
    ap_list.clear();
    last_scan_time = millis();
    save();
}

void CommandLedger::startSession(const uint8_t* mac) {
    session_active = true;
    memcpy(authorized_mac, mac, 6);
    session_start_time = millis();
    save();
}

void CommandLedger::endSession() {
    resetSession();
}

bool CommandLedger::isAuthorizedMAC(const uint8_t* mac) const {
    if (!session_active) return true;  // No active session, allow anyone
    return memcmp(mac, authorized_mac, 6) == 0;
}

void CommandLedger::setState(CommandState state) {
    current_state = state;
    state_enter_time = millis();
    save();
}

void CommandLedger::addAP(const uint8_t* mac, const String& ssid, int channel, int rssi, uint8_t encryption) {
    // Check if AP already exists (update if so)
    for (auto& ap : ap_list) {
        if (memcmp(ap.mac, mac, 6) == 0) {
            ap.ssid = ssid;
            ap.channel = channel;
            ap.rssi = rssi;
            ap.encryption = encryption;
            save();
            return;
        }
    }

    // Add new AP
    APInfo ap;
    memcpy(ap.mac, mac, 6);
    ap.ssid = ssid;
    ap.channel = channel;
    ap.rssi = rssi;
    ap.encryption = encryption;
    ap_list.push_back(ap);
    save();
}

bool CommandLedger::findAP(const uint8_t* mac, APInfo& out_info) const {
    for (const auto& ap : ap_list) {
        if (memcmp(ap.mac, mac, 6) == 0) {
            out_info = ap;
            return true;
        }
    }
    return false;
}

void CommandLedger::setChannel(int channel) {
    previous_channel = current_channel;
    current_channel = channel;
    save();
}

void CommandLedger::setHopping(bool enabled) {
    previous_hopping_enabled = hopping_enabled;
    hopping_enabled = enabled;
    save();
}

void CommandLedger::setOperationProgress(int progress) {
    operation_progress = progress;
    // Don't save on every progress update (too frequent)
}

void CommandLedger::setOperationResult(bool success, const String& message) {
    operation_success = success;
    operation_message = message;
    save();
}

void CommandLedger::setPendingTarget(const uint8_t* mac) {
    memcpy(pending_target_mac, mac, 6);
    pending_target_set = true;
    save();
}

void CommandLedger::clearPendingTarget() {
    memset(pending_target_mac, 0, 6);
    pending_target_set = false;
    save();
}

void CommandLedger::setError(const String& error, const String& detail) {
    has_error = true;
    last_error = error;
    last_error_detail = detail;
    error_time = millis();
    save();

    Serial.println("[CommandLedger] ERROR recorded: " + error + " - " + detail);
}

void CommandLedger::clearError() {
    has_error = false;
    last_error = "";
    last_error_detail = "";
    error_time = 0;
    save();
}

// Helper functions

String CommandLedger::macToString(const uint8_t* mac) const {
    // Use unified MAC formatting utility
    char buf[18];
    Utils::macToString(mac, buf);
    return String(buf);
}

bool CommandLedger::stringToMAC(const String& str, uint8_t* mac) const {
    // Use unified MAC parsing utility
    return Utils::stringToMAC(str, mac);
}

String CommandLedger::stateToString(CommandState state) const {
    switch (state) {
        case CommandState::IDLE: return "IDLE";
        case CommandState::AWAITING_CHANNEL_VALUE: return "AWAITING_CHANNEL_VALUE";
        case CommandState::AWAITING_HOPPING_VALUE: return "AWAITING_HOPPING_VALUE";
        case CommandState::SCAN_EXECUTING: return "SCAN_EXECUTING";
        case CommandState::SCAN_COMPLETE: return "SCAN_COMPLETE";
        case CommandState::ATTACK_EXECUTING: return "ATTACK_EXECUTING";
        case CommandState::ATTACK_COMPLETE: return "ATTACK_COMPLETE";
        case CommandState::PMKID_EXECUTING: return "PMKID_EXECUTING";
        case CommandState::PMKID_COMPLETE: return "PMKID_COMPLETE";
        case CommandState::CHANNEL_EXECUTING: return "CHANNEL_EXECUTING";
        case CommandState::CHANNEL_COMPLETE: return "CHANNEL_COMPLETE";
        case CommandState::HOPPING_EXECUTING: return "HOPPING_EXECUTING";
        case CommandState::HOPPING_COMPLETE: return "HOPPING_COMPLETE";
        case CommandState::STATUS_DISPLAY: return "STATUS_DISPLAY";
        case CommandState::EXPORT_EXECUTING: return "EXPORT_EXECUTING";
        case CommandState::EXPORT_COMPLETE: return "EXPORT_COMPLETE";
        case CommandState::BEACON_EXECUTING: return "BEACON_EXECUTING";
        case CommandState::BEACON_COMPLETE: return "BEACON_COMPLETE";
        case CommandState::ERROR_DISPLAY: return "ERROR_DISPLAY";
        default: return "UNKNOWN";
    }
}

CommandState CommandLedger::stringToState(const String& str) const {
    if (str == "IDLE") return CommandState::IDLE;
    if (str == "AWAITING_CHANNEL_VALUE") return CommandState::AWAITING_CHANNEL_VALUE;
    if (str == "AWAITING_HOPPING_VALUE") return CommandState::AWAITING_HOPPING_VALUE;
    if (str == "SCAN_EXECUTING") return CommandState::SCAN_EXECUTING;
    if (str == "SCAN_COMPLETE") return CommandState::SCAN_COMPLETE;
    if (str == "ATTACK_EXECUTING") return CommandState::ATTACK_EXECUTING;
    if (str == "ATTACK_COMPLETE") return CommandState::ATTACK_COMPLETE;
    if (str == "PMKID_EXECUTING") return CommandState::PMKID_EXECUTING;
    if (str == "PMKID_COMPLETE") return CommandState::PMKID_COMPLETE;
    if (str == "CHANNEL_EXECUTING") return CommandState::CHANNEL_EXECUTING;
    if (str == "CHANNEL_COMPLETE") return CommandState::CHANNEL_COMPLETE;
    if (str == "HOPPING_EXECUTING") return CommandState::HOPPING_EXECUTING;
    if (str == "HOPPING_COMPLETE") return CommandState::HOPPING_COMPLETE;
    if (str == "STATUS_DISPLAY") return CommandState::STATUS_DISPLAY;
    if (str == "EXPORT_EXECUTING") return CommandState::EXPORT_EXECUTING;
    if (str == "EXPORT_COMPLETE") return CommandState::EXPORT_COMPLETE;
    if (str == "BEACON_EXECUTING") return CommandState::BEACON_EXECUTING;
    if (str == "BEACON_COMPLETE") return CommandState::BEACON_COMPLETE;
    if (str == "ERROR_DISPLAY") return CommandState::ERROR_DISPLAY;
    return CommandState::IDLE;
}
