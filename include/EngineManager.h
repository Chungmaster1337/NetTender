#ifndef ENGINE_MANAGER_H
#define ENGINE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "config.h"

// Forward declarations
class DisplayManager;
class SystemLogger;

/**
 * @brief Engine types
 */
enum class EngineType {
    NONE = 0,
    RF_SCANNER = 1,
    NETWORK_ANALYZER = 2,
    EMERGENCY_ROUTER = 3
};

/**
 * @brief Base class for all engines
 */
class Engine {
public:
    virtual ~Engine() {}

    virtual bool begin() = 0;
    virtual void loop() = 0;
    virtual void stop() = 0;
    virtual const char* getName() = 0;
    virtual void handleButton(uint8_t button) {}
    virtual bool isHealthy() { return true; }
};

/**
 * @brief Manages engine lifecycle with concurrent operation support
 */
class EngineManager {
public:
    EngineManager(DisplayManager* display, SystemLogger* logger);
    ~EngineManager();

    /**
     * @brief Initialize and auto-start engines based on config
     */
    void begin();

    /**
     * @brief Main loop - runs all active engines
     */
    void loop();

    /**
     * @brief Perform Power-On Self Test
     */
    bool performPOST();

    /**
     * @brief Check if system is healthy
     */
    bool isSystemHealthy();

    /**
     * @brief Get active engine count
     */
    size_t getActiveEngineCount() const { return activeEngines.size(); }

    /**
     * @brief Manual engine start (for web/telnet control)
     */
    bool loadEngine(EngineType type);

    /**
     * @brief Stop all engines and return to idle
     */
    void returnToMenu();

    /**
     * @brief Check if any engine is active
     */
    bool isEngineActive() const { return !activeEngines.empty(); }

    /**
     * @brief Get current engine (for compatibility - returns first active)
     */
    EngineType getCurrentEngine() const;

private:
    DisplayManager* display;
    SystemLogger* logger;
    std::vector<Engine*> activeEngines;

    // Boot and initialization
    void autoStart();
    void startDualEngineMode();
    void startEmergencyRouterMode();
    void showBootStatus(const String& component, const String& message, bool success);

    // Engine management
    Engine* createEngine(EngineType type);
    bool startEngine(EngineType type);
    void stopAllEngines();

    // Health monitoring
    unsigned long lastHealthCheck;
    void checkEngineHealth();
};

#endif // ENGINE_MANAGER_H
