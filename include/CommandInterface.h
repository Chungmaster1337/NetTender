#ifndef COMMAND_INTERFACE_H
#define COMMAND_INTERFACE_H

#include <Arduino.h>
#include <vector>
#include "PacketSniffer.h"

/**
 * Dual Command Interface for Sniffy Boi
 *
 * Supports TWO control methods:
 * 1. Serial CLI - Type commands over USB (development/debug)
 * 2. Wireless C2 - Send WiFi probe requests with magic SSIDs (field ops)
 *
 * Wireless Command Format:
 * SSID: "SNIFFY:<CMD>:<PARAMS>"
 *
 * Examples:
 *   SNIFFY:SCAN                   - List discovered APs
 *   SNIFFY:ATTACK:AABBCCDDEEFF    - Deauth attack on MAC
 *   SNIFFY:PMKID:001122334455     - Clientless PMKID attack
 *   SNIFFY:EXPORT                 - Print all hashcat hashes
 *   SNIFFY:STATUS                 - Show capture stats
 *   SNIFFY:CHANNEL:6              - Lock to channel 6
 *   SNIFFY:HOPPING:ON             - Enable channel hopping
 *
 * Security Note: Magic SSID prefix prevents accidental triggers
 */

enum class CommandType {
    SCAN,           // List discovered APs
    ATTACK,         // Deauth attack + handshake capture
    PMKID,          // Clientless PMKID attack
    EXPORT,         // Export all captures to hashcat format
    STATUS,         // Show statistics
    CHANNEL,        // Set specific channel
    HOPPING,        // Enable/disable channel hopping
    CLEAR,          // Clear capture database
    HELP,           // Show available commands
    UNKNOWN
};

struct Command {
    CommandType type;
    String param1;
    String param2;
    bool is_wireless;  // True if received via magic packet
    uint8_t source_mac[6];  // MAC of device that sent command
};

class CommandInterface {
public:
    CommandInterface(PacketSniffer* sniffer);

    /**
     * @brief Initialize command interface
     */
    void begin();

    /**
     * @brief Process serial input (USB commands)
     * Call this in main loop
     */
    void processSerial();

    /**
     * @brief Process wireless magic packet (called by PacketSniffer)
     * @param ssid SSID from probe request or beacon
     * @param source_mac MAC address of sender
     */
    void processWirelessCommand(const String& ssid, const uint8_t* source_mac);

    /**
     * @brief Check if SSID is a magic command packet
     * @return true if SSID starts with "SNIFFY:"
     */
    static bool isMagicPacket(const String& ssid);

    /**
     * @brief Show command prompt
     */
    void showPrompt();

private:
    PacketSniffer* sniffer;
    String serial_buffer;
    unsigned long last_prompt_time;

    /**
     * @brief Parse command string (from serial or SSID)
     * @param input Command string
     * @param is_wireless True if from WiFi packet
     * @param source_mac MAC of sender (wireless only)
     * @return Parsed command structure
     */
    Command parseCommand(const String& input, bool is_wireless = false, const uint8_t* source_mac = nullptr);

    /**
     * @brief Execute parsed command
     */
    void executeCommand(const Command& cmd);

    // Command handlers
    void handleScan();
    void handleAttack(const String& target_mac);
    void handlePMKID(const String& target_mac);
    void handleExport();
    void handleStatus();
    void handleChannel(const String& channel_str);
    void handleHopping(const String& state);
    void handleClear();
    void handleHelp();

    // Helper functions
    bool parseMACAddress(const String& mac_str, uint8_t* mac_out);
    void printMACAddress(const uint8_t* mac);
};

#endif // COMMAND_INTERFACE_H
