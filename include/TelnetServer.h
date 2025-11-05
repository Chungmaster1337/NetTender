#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <Arduino.h>
#include <WiFi.h>

class EngineManager;

/**
 * @brief Telnet server for remote command-line access
 *
 * Provides SSH-like terminal access via telnet on port 23
 * Commands:
 * - status: Show system status
 * - engines: List available engines
 * - start <1-3>: Start an engine
 * - stop: Stop current engine
 * - restart: Restart ESP32
 * - help: Show available commands
 */
class TelnetServer {
public:
    TelnetServer(EngineManager* engineMgr, uint16_t port = 23);
    ~TelnetServer();

    /**
     * @brief Start the telnet server
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Stop the telnet server
     */
    void stop();

    /**
     * @brief Handle client connections and commands
     */
    void loop();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running; }

    /**
     * @brief Enable/disable telnet server
     */
    void setEnabled(bool enabled);

    /**
     * @brief Send message to connected client
     */
    void println(const String& message);

private:
    WiFiServer* server;
    WiFiClient client;
    EngineManager* engineManager;
    uint16_t serverPort;
    bool running;
    bool enabled;
    String commandBuffer;

    // Command handlers
    void handleCommand(const String& cmd);
    void cmdStatus();
    void cmdEngines();
    void cmdStart(int engineId);
    void cmdStop();
    void cmdRestart();
    void cmdHelp();
    void cmdClear();

    // Helper methods
    void sendPrompt();
    void sendWelcome();
    void processInput(char c);
};

#endif // TELNET_SERVER_H
