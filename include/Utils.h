#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

/**
 * @file Utils.h
 * @brief Shared utility functions for common operations
 *
 * Provides unified implementations to avoid code duplication across:
 * - CommandInterface
 * - CommandLedger
 * - DisplayManager
 */

namespace Utils {
    /**
     * @brief Format MAC address to string buffer
     * @param mac 6-byte MAC address
     * @param buf Output buffer (must be at least 18 bytes)
     * @return Pointer to buf (for chaining)
     *
     * Format: "AA:BB:CC:DD:EE:FF"
     *
     * Example:
     *   char buf[18];
     *   Utils::macToString(mac, buf);
     *   Serial.println(buf);
     */
    inline const char* macToString(const uint8_t* mac, char* buf) {
        snprintf(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return buf;
    }

    /**
     * @brief Parse MAC address from string
     * @param str MAC string (formats: "AA:BB:CC:DD:EE:FF" or "AABBCCDDEEFF")
     * @param mac Output 6-byte array
     * @return true if parsing succeeded, false on invalid format
     *
     * Accepts formats:
     *   - "AA:BB:CC:DD:EE:FF"
     *   - "AA-BB-CC-DD-EE-FF"
     *   - "AABBCCDDEEFF"
     */
    inline bool stringToMAC(const String& str, uint8_t* mac) {
        String cleaned = str;
        cleaned.replace(":", "");
        cleaned.replace("-", "");
        cleaned.toUpperCase();

        if (cleaned.length() != 12) return false;

        for (int i = 0; i < 6; i++) {
            String byteStr = cleaned.substring(i * 2, i * 2 + 2);
            // Check if valid hex
            for (int j = 0; j < 2; j++) {
                char c = byteStr.charAt(j);
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                    return false;
                }
            }
            mac[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        }

        return true;
    }

    /**
     * @brief Parse MAC address from C-string (optimized version)
     * @param str MAC string (formats: "AA:BB:CC:DD:EE:FF" or "AABBCCDDEEFF")
     * @param mac Output 6-byte array
     * @return true if parsing succeeded, false on invalid format
     */
    inline bool stringToMAC(const char* str, uint8_t* mac) {
        // Try colon-separated format first
        if (sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
            return true;
        }

        // Try dash-separated format
        if (sscanf(str, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
            return true;
        }

        // Try continuous format (AABBCCDDEEFF)
        if (strlen(str) == 12) {
            for (int i = 0; i < 6; i++) {
                char byte[3] = {str[i*2], str[i*2+1], '\0'};
                mac[i] = (uint8_t)strtol(byte, NULL, 16);
            }
            return true;
        }

        return false;
    }

    /**
     * @brief Compare two MAC addresses
     * @return true if MACs are equal
     */
    inline bool macEquals(const uint8_t* mac1, const uint8_t* mac2) {
        return memcmp(mac1, mac2, 6) == 0;
    }

    /**
     * @brief Check if MAC address is broadcast
     * @return true if MAC is FF:FF:FF:FF:FF:FF
     */
    inline bool isBroadcast(const uint8_t* mac) {
        const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        return macEquals(mac, broadcast);
    }

    /**
     * @brief Check if MAC address is zero/null
     * @return true if MAC is 00:00:00:00:00:00
     */
    inline bool isNullMAC(const uint8_t* mac) {
        const uint8_t null_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        return macEquals(mac, null_mac);
    }

    /**
     * @brief Copy MAC address
     */
    inline void macCopy(uint8_t* dest, const uint8_t* src) {
        memcpy(dest, src, 6);
    }
}

#endif // UTILS_H
