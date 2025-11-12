#ifndef VERSION_H
#define VERSION_H

/**
 * @file version.h
 * @brief Sniffy Boi version information and build configuration
 *
 * Version History:
 * - v1.0 (2025-11-10): Initial release
 *   - RF Scanner with handshake capture
 *   - PMKID extraction
 *   - Deauth attacks
 *   - Interactive command interface (wireless C2 + serial)
 *   - OLED display with split-screen operational view
 *   - Hashcat mode 22000 export
 *
 * - v1.1 (2025-11-11): Performance optimizations
 *   - Unified MAC address utilities (Utils.h)
 *   - Font caching in DisplayManager (~33% faster rendering)
 *   - Named constants for magic numbers (improved readability)
 *   - Code deduplication (3 MAC implementations → 1)
 *   - Zero behavioral changes, fully backward compatible
 */

// ==================== VERSION INFORMATION ====================

#define SNIFFY_VERSION_MAJOR 1
#define SNIFFY_VERSION_MINOR 1
#define SNIFFY_VERSION_PATCH 0

// Auto-generated version string
#define SNIFFY_VERSION_STRING "v1.1.0"

// Build date (set at compile time)
#define SNIFFY_BUILD_DATE __DATE__
#define SNIFFY_BUILD_TIME __TIME__

// ==================== BUILD CONFIGURATION ====================

// Uncomment to enable debug features
// #define DEBUG_BUILD 1

#ifdef DEBUG_BUILD
    #define DEBUG_LOG(msg) Serial.println("[DEBUG] " msg)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf("[DEBUG] " fmt "\n", __VA_ARGS__)
#else
    #define DEBUG_LOG(msg)
    #define DEBUG_PRINTF(fmt, ...)
#endif

// ==================== FEATURE FLAGS ====================

// Core features (always enabled)
#define FEATURE_HANDSHAKE_CAPTURE   1
#define FEATURE_PMKID_EXTRACTION    1
#define FEATURE_DEAUTH_ATTACK       1
#define FEATURE_WIRELESS_C2         1
#define FEATURE_SERIAL_COMMANDS     1
#define FEATURE_OLED_DISPLAY        1

// Optional features (can be disabled to save memory)
#define FEATURE_BEACON_FLOOD        1
#define FEATURE_CHANNEL_HOPPING     1
#define FEATURE_SESSION_LOCKING     1
#define FEATURE_FILESYSTEM_LEDGER   1

// v1.1 optimizations (always enabled in v1.1)
#define OPTIMIZATION_FONT_CACHE     1
#define OPTIMIZATION_UNIFIED_MAC    1
#define OPTIMIZATION_NAMED_CONSTANTS 1

// ==================== HARDWARE CONFIGURATION ====================

// Default to Arduino Nano ESP32
#define HARDWARE_PLATFORM "Arduino Nano ESP32"
#define HARDWARE_CHIP "ESP32-S3"
#define HARDWARE_RAM_KB 327
#define HARDWARE_FLASH_KB 16384

// ==================== VERSION DISPLAY ====================

/**
 * @brief Print full version information to serial
 */
inline void printVersionInfo() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                   SNIFFY BOI " SNIFFY_VERSION_STRING "                        ║");
    Serial.println("║              Wardriving & WPA2 Attack Platform           ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("  Version:     " SNIFFY_VERSION_STRING);
    Serial.println("  Build:       " SNIFFY_BUILD_DATE " " SNIFFY_BUILD_TIME);
    Serial.println("  Platform:    " HARDWARE_PLATFORM);
    Serial.println("  Chip:        " HARDWARE_CHIP);
    Serial.println();
    Serial.println("  Features:");
    Serial.println("    - Handshake Capture (WPA/WPA2)");
    Serial.println("    - PMKID Extraction (clientless)");
    Serial.println("    - Deauth Attacks (targeted & broadcast)");
    Serial.println("    - Beacon Flooding");
    Serial.println("    - Wireless C2 (magic packet commands)");
    Serial.println("    - Interactive Serial CLI");
    Serial.println("    - OLED Status Display");
    Serial.println("    - Hashcat Mode 22000 Export");
    Serial.println();

    #if OPTIMIZATION_FONT_CACHE || OPTIMIZATION_UNIFIED_MAC || OPTIMIZATION_NAMED_CONSTANTS
    Serial.println("  Optimizations (v1.1):");
    #if OPTIMIZATION_FONT_CACHE
    Serial.println("    ✓ Font caching (33% faster display)");
    #endif
    #if OPTIMIZATION_UNIFIED_MAC
    Serial.println("    ✓ Unified MAC utilities");
    #endif
    #if OPTIMIZATION_NAMED_CONSTANTS
    Serial.println("    ✓ Named constants (improved readability)");
    #endif
    Serial.println();
    #endif

    Serial.println("  Output:      Hashcat mode 22000");
    Serial.println("  Network:     Monitor mode (standalone operation)");
    Serial.println();
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println();
}

/**
 * @brief Get version string for display
 */
inline const char* getVersionString() {
    return SNIFFY_VERSION_STRING;
}

/**
 * @brief Get version number as integer (for comparison)
 * Format: MAJOR * 10000 + MINOR * 100 + PATCH
 */
inline int getVersionNumber() {
    return (SNIFFY_VERSION_MAJOR * 10000) +
           (SNIFFY_VERSION_MINOR * 100) +
           SNIFFY_VERSION_PATCH;
}

#endif // VERSION_H
