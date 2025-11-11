#ifndef COMMAND_INTERFACE_H
#define COMMAND_INTERFACE_H

#include <Arduino.h>
#include "PacketSniffer.h"
#include "CommandLedger.h"
#include "DisplayManager.h"
#include "SystemLogger.h"

/**
 * CommandInterface V2 - Interactive State Machine
 *
 * Multi-step command flows with MAC-based session authentication.
 * Uses CommandLedger for persistent state and DisplayManager for visual feedback.
 */

enum class CommandType {
    SCAN,           // Scan for APs
    ATTACK,         // Deauth attack
    PMKID,          // PMKID attack
    CHANNEL,        // Set/query channel
    HOPPING,        // Toggle hopping
    BEACON,         // Beacon flood attack
    STATUS,         // Show status
    EXPORT,         // Export captures
    CONFIRM,        // Confirm pending operation
    CANCEL,         // Cancel/abort
    HELP,           // Show help
    UNKNOWN
};

struct Command {
    CommandType type;
    String param1;
    String param2;
    bool is_wireless;
    uint8_t source_mac[6];
};

class CommandInterface {
public:
    CommandInterface(PacketSniffer* sniffer, DisplayManager* display, SystemLogger* logger);

    /**
     * @brief Initialize command interface and ledger
     */
    void begin();

    /**
     * @brief Main loop - handles timeouts and display updates
     * MUST be called frequently from main loop
     */
    void loop();

    /**
     * @brief Process serial input (USB commands)
     */
    void processSerial();

    /**
     * @brief Process wireless magic packet (called by PacketSniffer)
     */
    void processWirelessCommand(const String& ssid, const uint8_t* source_mac);

    /**
     * @brief Check if SSID is a magic command packet
     */
    static bool isMagicPacket(const String& ssid);

    /**
     * @brief Get command ledger for state inspection
     */
    CommandLedger* getLedger() { return ledger; }

private:
    PacketSniffer* sniffer;
    DisplayManager* display;
    SystemLogger* logger;
    CommandLedger* ledger;

    String serial_buffer;
    unsigned long last_prompt_time;
    unsigned long last_display_update;

    // Parsing
    Command parseCommand(const String& input, bool is_wireless, const uint8_t* source_mac);

    // Command routing based on state
    void processCommand(const Command& cmd);

    // Validation
    bool validateSession(const uint8_t* source_mac);
    bool validateTarget(const uint8_t* target_mac, APInfo& out_info);
    bool validateChannel(int channel);

    // State transitions
    void executeCommand(const Command& cmd);

    // Command handlers (from IDLE state)
    void handleScan(const Command& cmd);
    void handleAttack(const Command& cmd);
    void handlePMKID(const Command& cmd);
    void handleChannel(const Command& cmd);
    void handleHopping(const Command& cmd);
    void handleBeacon(const Command& cmd);
    void handleStatus(const Command& cmd);
    void handleExport(const Command& cmd);

    // State-specific handlers
    void handleAwaitingChannelValue(const Command& cmd);
    void handleAwaitingHoppingValue(const Command& cmd);

    // Operation executors
    void executeScan();
    void executeAttack(const uint8_t* target_mac);
    void executePMKID(const uint8_t* target_mac);
    void executeChannelChange(int channel);
    void executeHoppingToggle(bool enable);

    // Completion handlers
    void onScanComplete(int ap_count);
    void onAttackComplete(bool success, const String& message);
    void onPMKIDComplete(bool success, const String& message);
    void onConfigChange(const String& setting, const String& old_val, const String& new_val);

    // Error handling
    void showError(const String& error, const String& detail);
    void handleSessionTimeout();
    void handleStateTimeout();

    // Display updates
    void updateDisplay();

    // Helper functions
    bool parseMACAddress(const String& mac_str, uint8_t* mac_out);
    void printMACAddress(const uint8_t* mac);
    String macToString(const uint8_t* mac);
    void showPrompt();
};

#endif // COMMAND_INTERFACE_H
