#ifndef COMMAND_LEDGER_H
#define COMMAND_LEDGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>
#include <map>

/**
 * CommandLedger - Persistent state storage for command interface
 *
 * Stores session state, scan results, and configuration to filesystem.
 * Only modified by commands (not by errors/timeouts).
 *
 * File: /command.ledger (LittleFS)
 * Format: Simple key=value pairs
 */

enum class CommandState {
    IDLE,
    AWAITING_CHANNEL_VALUE,     // After SNIFFY:CHANNEL, waiting for CHANNEL:N
    AWAITING_HOPPING_VALUE,     // After SNIFFY:HOPPING, waiting for HOPPING:ON/OFF
    SCAN_EXECUTING,             // Scan in progress
    SCAN_COMPLETE,              // Scan done, showing results (60s cooldown)
    ATTACK_EXECUTING,           // Attack in progress
    ATTACK_COMPLETE,            // Attack done, showing results (60s cooldown)
    PMKID_EXECUTING,            // PMKID attack in progress
    PMKID_COMPLETE,             // PMKID done, showing results (60s cooldown)
    CHANNEL_EXECUTING,          // Channel change in progress
    CHANNEL_COMPLETE,           // Channel changed, showing OLD/NEW (10s)
    HOPPING_EXECUTING,          // Hopping toggle in progress
    HOPPING_COMPLETE,           // Hopping toggled, showing OLD/NEW (10s)
    STATUS_DISPLAY,             // Status displayed (60s cooldown)
    EXPORT_EXECUTING,           // Export in progress
    EXPORT_COMPLETE,            // Export done (60s cooldown)
    BEACON_EXECUTING,           // Beacon flood starting
    BEACON_COMPLETE,            // Beacon flood active/stopped (60s cooldown)
    ERROR_DISPLAY               // Showing error (20s, then reset)
};

struct APInfo {
    uint8_t mac[6];
    String ssid;
    int channel;
    int rssi;
    uint8_t encryption;
};

class CommandLedger {
public:
    CommandLedger();

    /**
     * @brief Initialize filesystem and load ledger
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Save current state to filesystem
     */
    void save();

    /**
     * @brief Load state from filesystem
     */
    void load();

    /**
     * @brief Clear all session data (called on error/timeout)
     * Does NOT clear configuration or scan results
     */
    void resetSession();

    /**
     * @brief Clear scan results
     */
    void clearScanResults();

    // Session management
    bool isSessionActive() const { return session_active; }
    void startSession(const uint8_t* mac);
    void endSession();
    unsigned long getSessionStartTime() const { return session_start_time; }
    const uint8_t* getAuthorizedMAC() const { return authorized_mac; }
    bool isAuthorizedMAC(const uint8_t* mac) const;

    // State management
    CommandState getState() const { return current_state; }
    void setState(CommandState state);
    unsigned long getStateEnterTime() const { return state_enter_time; }

    // Scan results
    void addAP(const uint8_t* mac, const String& ssid, int channel, int rssi, uint8_t encryption);
    const std::vector<APInfo>& getAPList() const { return ap_list; }
    int getAPCount() const { return ap_list.size(); }
    bool findAP(const uint8_t* mac, APInfo& out_info) const;

    // Configuration
    int getCurrentChannel() const { return current_channel; }
    int getPreviousChannel() const { return previous_channel; }
    void setChannel(int channel);

    bool isHoppingEnabled() const { return hopping_enabled; }
    bool wasPreviousHoppingEnabled() const { return previous_hopping_enabled; }
    void setHopping(bool enabled);

    // Error tracking
    bool hasError() const { return has_error; }
    String getLastError() const { return last_error; }
    String getLastErrorDetail() const { return last_error_detail; }
    unsigned long getErrorTime() const { return error_time; }
    void setError(const String& error, const String& detail);
    void clearError();

    // Operation tracking
    int getOperationProgress() const { return operation_progress; }
    void setOperationProgress(int progress);

    bool getOperationSuccess() const { return operation_success; }
    void setOperationResult(bool success, const String& message);
    String getOperationMessage() const { return operation_message; }

    // Pending operation data
    const uint8_t* getPendingTargetMAC() const { return pending_target_mac; }
    void setPendingTarget(const uint8_t* mac);
    void clearPendingTarget();

private:
    // Session state
    bool session_active;
    uint8_t authorized_mac[6];
    unsigned long session_start_time;

    // Current state
    CommandState current_state;
    unsigned long state_enter_time;

    // Scan results
    std::vector<APInfo> ap_list;
    unsigned long last_scan_time;

    // Configuration
    int current_channel;
    int previous_channel;
    bool hopping_enabled;
    bool previous_hopping_enabled;

    // Operation tracking
    int operation_progress;  // 0-100
    bool operation_success;
    String operation_message;

    // Pending operation data
    uint8_t pending_target_mac[6];
    bool pending_target_set;

    // Error tracking
    bool has_error;
    String last_error;
    String last_error_detail;
    unsigned long error_time;

    // Filesystem path
    static constexpr const char* LEDGER_PATH = "/command.ledger";

    // Helper functions
    String macToString(const uint8_t* mac) const;
    bool stringToMAC(const String& str, uint8_t* mac) const;
    String stateToString(CommandState state) const;
    CommandState stringToState(const String& str) const;
};

#endif // COMMAND_LEDGER_H
