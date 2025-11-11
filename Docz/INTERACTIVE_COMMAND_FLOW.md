# Interactive Command Flow - State Machine Design

## Overview
Multi-step command interface with MAC-based session authentication and contextual OLED feedback.

## State Machine Diagram

```
[IDLE] ──SNIFFY:SCAN──> [SCAN_COMPLETE]
                            │
                            ├─SNIFFY:ATTACK:MAC──> [ATTACK_TARGET_SET]
                            │                           │
                            │                           ├─SNIFFY:CONFIRM──> [ATTACK_EXECUTING]
                            │                           │                       │
                            │                           │                       └──> [ATTACK_COMPLETE] ──> [COOLDOWN] ──> [IDLE]
                            │                           │
                            │                           └─SNIFFY:CANCEL──> [SCAN_COMPLETE]
                            │
                            ├─SNIFFY:PMKID:MAC──> [PMKID_TARGET_SET]
                            │                          │
                            │                          ├─SNIFFY:CONFIRM──> [PMKID_EXECUTING] ──> [PMKID_COMPLETE] ──> [COOLDOWN] ──> [IDLE]
                            │                          │
                            │                          └─SNIFFY:CANCEL──> [SCAN_COMPLETE]
                            │
                            ├─SNIFFY:CHANNEL:N──> [CHANNEL_CHANGING] ──> [COOLDOWN] ──> [IDLE]
                            │
                            └─SNIFFY:HOPPING:ON/OFF──> [HOPPING_TOGGLING] ──> [COOLDOWN] ──> [IDLE]

[IDLE] ──SNIFFY:STATUS──> [STATUS_DISPLAY] ──> [COOLDOWN] ──> [IDLE]

[IDLE] ──SNIFFY:EXPORT──> [EXPORTING] ──> [COOLDOWN] ──> [IDLE]

[ANY STATE] ──TIMEOUT (120s)──> [IDLE]
[ANY STATE] ──SNIFFY:CANCEL (from authorized MAC)──> [IDLE]
```

## Command Chains

### Chain 1: Scan → Attack Flow
```
Step 1: User broadcasts "SNIFFY:SCAN"
  ├─ System: Captures source MAC (e.g., AA:BB:CC:DD:EE:FF)
  ├─ System: Runs AP scan
  ├─ State: SCAN_COMPLETE
  └─ OLED Display:
      ┌─────────────────┐
      │ SCAN COMPLETE   │
      │ 12 APs found    │
      │                 │
      │ FROM:           │
      │ AA:BB:CC:DD:EE  │
      │                 │
      │ NEXT COMMANDS:  │
      │ ATTACK:AABBCC.. │
      │ PMKID:112233..  │
      │ CHANNEL:6       │
      │ HOPPING:ON      │
      │ CANCEL          │
      └─────────────────┘

Step 2: User broadcasts "SNIFFY:ATTACK:001122334455"
  ├─ System: Verifies source MAC matches AA:BB:CC:DD:EE:FF ✓
  ├─ System: Parses target MAC: 00:11:22:33:44:55
  ├─ System: Validates target exists in scan results
  ├─ State: ATTACK_TARGET_SET
  └─ OLED Display:
      ┌─────────────────┐
      │ ATTACK TARGET   │
      │ 00:11:22:33:44  │
      │                 │
      │ SSID: HomeNet   │
      │ Channel: 6      │
      │ Encryption: WPA2│
      │                 │
      │ READY TO ATTACK │
      │                 │
      │ CONFIRM to start│
      │ CANCEL to abort │
      └─────────────────┘

Step 3: User broadcasts "SNIFFY:CONFIRM"
  ├─ System: Verifies source MAC matches AA:BB:CC:DD:EE:FF ✓
  ├─ State: ATTACK_EXECUTING
  ├─ OLED Display (initial):
      ┌─────────────────┐
      │ ATTACKING...    │
      │ 00:11:22:33:44  │
      │                 │
      │ Sending deauth  │
      │ [=====>      ]  │
      │                 │
      │ Monitoring for  │
      │ handshake...    │
      └─────────────────┘
  │
  ├─ System: Sends 5 deauth packets
  ├─ System: Monitors for EAPOL frames
  │
  └─ OLED Display (on completion):
      ┌─────────────────┐
      │ ✓ SUCCESS       │
      │                 │
      │ Handshake       │
      │ captured!       │
      │                 │
      │ AP: 00:11:22:.. │
      │ Client: AA:BB.. │
      │                 │
      │ Exported to     │
      │ hashcat format  │
      └─────────────────┘

Step 4: Automatic cooldown
  ├─ State: COOLDOWN
  └─ OLED Display:
      ┌─────────────────┐
      │ COOLDOWN        │
      │                 │
      │ Returning to    │
      │ monitor mode    │
      │                 │
      │ in 45 seconds   │
      │                 │
      │ [=========>   ] │
      │                 │
      │ Send CANCEL to  │
      │ skip countdown  │
      └─────────────────┘

Step 5: Return to passive monitoring
  ├─ State: IDLE
  └─ OLED Display: Returns to normal operational view (engine health + logs)
```

### Chain 2: Scan → PMKID Flow
```
Step 1: "SNIFFY:SCAN" → [SCAN_COMPLETE]
Step 2: "SNIFFY:PMKID:AABBCCDDEEFF" → [PMKID_TARGET_SET]
  OLED:
      ┌─────────────────┐
      │ PMKID ATTACK    │
      │ AA:BB:CC:DD:EE  │
      │                 │
      │ Type: Clientless│
      │ No deauth needed│
      │                 │
      │ CONFIRM to start│
      │ CANCEL to abort │
      └─────────────────┘

Step 3: "SNIFFY:CONFIRM" → [PMKID_EXECUTING] → [PMKID_COMPLETE] → [COOLDOWN] → [IDLE]
```

### Chain 3: Direct Commands (No Confirmation)
```
"SNIFFY:CHANNEL:11" → [CHANNEL_CHANGING] → [COOLDOWN] → [IDLE]
  OLED:
      ┌─────────────────┐
      │ CHANNEL LOCKED  │
      │                 │
      │ Channel: 11     │
      │                 │
      │ Hopping stopped │
      │                 │
      │ Cooldown: 60s   │
      └─────────────────┘

"SNIFFY:HOPPING:ON" → [HOPPING_TOGGLING] → [COOLDOWN] → [IDLE]
  OLED:
      ┌─────────────────┐
      │ CHANNEL HOPPING │
      │                 │
      │ Status: ENABLED │
      │                 │
      │ Scanning 1-13   │
      │ Current: 6      │
      │                 │
      │ Cooldown: 60s   │
      └─────────────────┘

"SNIFFY:STATUS" → [STATUS_DISPLAY] → [COOLDOWN] → [IDLE]
  OLED:
      ┌─────────────────┐
      │ SYSTEM STATUS   │
      │                 │
      │ Uptime: 2h 15m  │
      │ Memory: 42KB    │
      │ Packets: 15,234 │
      │ APs: 47         │
      │ Handshakes: 3   │
      │                 │
      │ Cooldown: 60s   │
      └─────────────────┘

"SNIFFY:EXPORT" → [EXPORTING] → [COOLDOWN] → [IDLE]
  OLED:
      ┌─────────────────┐
      │ EXPORTING...    │
      │                 │
      │ 3 handshakes    │
      │ exported to     │
      │ serial (hashcat)│
      │                 │
      │ Check serial    │
      │ for output      │
      │                 │
      │ Cooldown: 60s   │
      └─────────────────┘
```

### Emergency Cancel
```
From ANY state (except IDLE):
  "SNIFFY:CANCEL" (from authorized MAC only!)
    ├─ Immediately abort current operation
    ├─ Clear session state
    ├─ Reset authorized MAC
    └─ Return to IDLE
```

## MAC-Based Session Authentication

### Session Initiation
```cpp
// When receiving first command (from IDLE state)
void initiateSession(const uint8_t* source_mac, CommandType cmd) {
    memcpy(authorized_mac, source_mac, 6);
    session_start_time = millis();
    session_active = true;

    logger->info("CommandInterface",
                 "Session started from " + macToString(source_mac), 1);
}
```

### Session Validation
```cpp
// Before processing any subsequent command
bool validateSession(const uint8_t* source_mac) {
    if (!session_active) return true;  // No session, allow anyone

    // Check MAC match
    if (memcmp(source_mac, authorized_mac, 6) != 0) {
        logger->warning("CommandInterface",
                        "Command rejected: Wrong MAC", 1);
        display->showSessionError("UNAUTHORIZED MAC");
        return false;
    }

    // Check timeout (2 minutes)
    if (millis() - session_start_time > 120000) {
        logger->warning("CommandInterface", "Session timeout", 1);
        resetSession();
        return false;
    }

    // Refresh session timeout
    session_start_time = millis();
    return true;
}
```

### Session Termination
```cpp
void resetSession() {
    session_active = false;
    memset(authorized_mac, 0, 6);
    current_state = CommandState::IDLE;
    display->showOperationalView(logger);  // Return to normal
}
```

## State Tracking Structure

```cpp
enum class CommandState {
    IDLE,                   // Normal monitoring
    SCAN_COMPLETE,          // Scan finished, showing options
    ATTACK_TARGET_SET,      // Target selected, awaiting confirm
    ATTACK_EXECUTING,       // Attack in progress
    ATTACK_COMPLETE,        // Attack finished, showing results
    PMKID_TARGET_SET,       // PMKID target set, awaiting confirm
    PMKID_EXECUTING,        // PMKID attack in progress
    PMKID_COMPLETE,         // PMKID finished
    CHANNEL_CHANGING,       // Changing channel
    HOPPING_TOGGLING,       // Toggling hopping
    STATUS_DISPLAY,         // Showing status
    EXPORTING,              // Exporting captures
    COOLDOWN                // Countdown before returning to IDLE
};

struct CommandSession {
    CommandState current_state;
    bool session_active;
    uint8_t authorized_mac[6];
    unsigned long session_start_time;
    unsigned long state_enter_time;

    // Pending operation data
    uint8_t pending_target_mac[6];
    int pending_channel;
    bool pending_hopping_state;

    // Operation results
    bool operation_success;
    String operation_message;
    int operation_progress;  // 0-100 for progress bars
};
```

## OLED Display States

### Display Layout (128x64 pixels)

#### During Commands (Full Screen)
```
┌──────────────────────────────┐ 128px wide
│ [COMMAND NAME]               │ Line 1: Large font, centered
│                              │
│ [STATUS/TARGET INFO]         │ Lines 2-4: Medium font
│ [DETAILS]                    │
│                              │
│ [PROGRESS BAR]               │ Line 5: Visual progress
│                              │
│ [NEXT ACTION / COUNTDOWN]    │ Lines 6-8: Instructions
│ [INSTRUCTIONS]               │
└──────────────────────────────┘ 64px tall
```

#### Cooldown Screen
```
┌──────────────────────────────┐
│     COOLDOWN                 │ Large font
│                              │
│   Returning to               │ Medium font
│   monitor mode               │
│                              │
│   in 45 seconds              │ Large numbers
│                              │
│ [==================>       ] │ Progress bar (empties)
│                              │
│ Send CANCEL to skip          │ Small font hint
└──────────────────────────────┘
```

## Timeout Behavior

### Session Timeout (120 seconds)
- If no command received within 2 minutes of session start
- Automatically reset to IDLE
- Display shows: "Session timeout - Returning to monitor mode"

### State Timeout (varies by state)
- SCAN_COMPLETE: 120 seconds (user has time to review list)
- ATTACK_TARGET_SET: 60 seconds (don't wait too long for confirm)
- PMKID_TARGET_SET: 60 seconds
- ATTACK_EXECUTING: No timeout (wait for completion or manual cancel)
- COOLDOWN: 60 seconds (fixed countdown)

## Error Handling

### Unauthorized MAC
```
User A: "SNIFFY:SCAN" (MAC: AA:BB:CC:DD:EE:FF)
  → Session starts, MAC AA:BB:CC:DD:EE:FF authorized

User B: "SNIFFY:ATTACK:112233445566" (MAC: 11:22:33:44:55:66)
  → REJECTED
  → OLED:
      ┌─────────────────┐
      │ ⚠ UNAUTHORIZED  │
      │                 │
      │ Session locked  │
      │ to:             │
      │ AA:BB:CC:DD:EE  │
      │                 │
      │ Wait for timeout│
      │ or use same MAC │
      └─────────────────┘
```

### Invalid Command for State
```
State: ATTACK_TARGET_SET
User: "SNIFFY:SCAN"
  → REJECTED (must CONFIRM or CANCEL first)
  → OLED:
      ┌─────────────────┐
      │ ⚠ INVALID CMD   │
      │                 │
      │ Expected:       │
      │ - CONFIRM       │
      │ - CANCEL        │
      │                 │
      │ Current command │
      │ must complete   │
      └─────────────────┘
```

### Invalid Target MAC
```
User: "SNIFFY:ATTACK:ZZZZZZZZZZZ"
  → REJECTED (invalid hex)
  → OLED:
      ┌─────────────────┐
      │ ⚠ INVALID MAC   │
      │                 │
      │ Format:         │
      │ AABBCCDDEEFF    │
      │                 │
      │ Try again       │
      └─────────────────┘
```

### Target Not Found
```
User: "SNIFFY:ATTACK:AABBCCDDEEFF"
  → Target MAC not in scan results
  → OLED:
      ┌─────────────────┐
      │ ⚠ TARGET NOT    │
      │   FOUND         │
      │                 │
      │ AA:BB:CC:DD:EE  │
      │ not in scan     │
      │ results         │
      │                 │
      │ Run SCAN first  │
      └─────────────────┘
```

## Implementation Files

### Modified Files
1. **CommandInterface.h** - Add CommandState enum, CommandSession struct, new methods
2. **CommandInterface.cpp** - Implement state machine logic, session validation
3. **DisplayManager.h** - Add display methods for each state
4. **DisplayManager.cpp** - Implement all state visualizations
5. **main.cpp** - Add cooldown timer logic to main loop

### New Methods Required

#### CommandInterface
```cpp
// State management
void transitionState(CommandState new_state);
bool validateStateTransition(CommandType cmd);
void resetSession();
bool validateSession(const uint8_t* source_mac);

// Command routing based on state
void processStatefulCommand(const Command& cmd);

// Progress tracking
void updateProgress(int percentage);
void setOperationResult(bool success, const String& message);
```

#### DisplayManager
```cpp
// State-specific displays
void showScanComplete(int ap_count, const uint8_t* session_mac);
void showAttackTarget(const uint8_t* target_mac, const String& ssid);
void showAttackExecuting(int progress);
void showAttackComplete(bool success, const String& details);
void showPMKIDTarget(const uint8_t* target_mac);
void showPMKIDExecuting(int progress);
void showChannelChange(int channel);
void showHoppingStatus(bool enabled, int current_channel);
void showStatusDisplay(/* system stats */);
void showExporting(int handshake_count);
void showCooldown(int seconds_remaining);
void showSessionError(const String& error_type);
void showInvalidCommand(const StringList& expected_commands);
```

## Testing Plan

### Test Sequence 1: Happy Path (Scan → Attack)
1. Reset device to IDLE
2. Send "SNIFFY:SCAN" from MAC A
   - Verify: State = SCAN_COMPLETE
   - Verify: OLED shows AP count + next options
   - Verify: Session locked to MAC A
3. Send "SNIFFY:ATTACK:AABBCCDDEEFF" from MAC A
   - Verify: State = ATTACK_TARGET_SET
   - Verify: OLED shows target + confirm/cancel
4. Send "SNIFFY:CONFIRM" from MAC A
   - Verify: State = ATTACK_EXECUTING
   - Verify: OLED shows progress
   - Verify: Deauth packets sent
5. Wait for attack completion
   - Verify: State = ATTACK_COMPLETE
   - Verify: OLED shows results
6. Wait for cooldown
   - Verify: State = COOLDOWN → IDLE
   - Verify: OLED counts down, then returns to operational view

### Test Sequence 2: MAC Authentication
1. Send "SNIFFY:SCAN" from MAC A
2. Try "SNIFFY:ATTACK:112233445566" from MAC B
   - Verify: Command rejected
   - Verify: OLED shows "UNAUTHORIZED"
   - Verify: State unchanged

### Test Sequence 3: Timeout
1. Send "SNIFFY:SCAN" from MAC A
2. Wait 125 seconds (past 120s timeout)
3. Try "SNIFFY:ATTACK:112233445566" from MAC A
   - Verify: Session expired
   - Verify: State reset to IDLE
   - Verify: New session starts

### Test Sequence 4: Cancel
1. Send "SNIFFY:SCAN" from MAC A
2. Send "SNIFFY:ATTACK:AABBCCDDEEFF" from MAC A
3. Send "SNIFFY:CANCEL" from MAC A
   - Verify: State immediately returns to IDLE
   - Verify: Session cleared
   - Verify: OLED returns to operational view

---

## Next Steps
1. Implement CommandState enum and CommandSession struct
2. Add session validation logic
3. Refactor executeCommand() to use state machine
4. Create all DisplayManager state methods
5. Test each command flow individually
6. Test error conditions and edge cases
