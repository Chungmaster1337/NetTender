#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

class EngineManager;
class SystemLogger;

/**
 * @brief Web server for remote control and monitoring
 *
 * Provides HTTP interface for:
 * - System status and statistics
 * - Engine control (start/stop/switch)
 * - Configuration management
 * - Live monitoring data
 * - RESTful API
 */
class WebServerManager {
public:
    WebServerManager(EngineManager* engineMgr, SystemLogger* logger, uint16_t port = 80);
    ~WebServerManager();

    /**
     * @brief Start the web server
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Stop the web server
     */
    void stop();

    /**
     * @brief Handle incoming requests (call in loop)
     */
    void handleClient();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running; }

    /**
     * @brief Enable/disable web server
     */
    void setEnabled(bool enabled);

private:
    WebServer* server;
    EngineManager* engineManager;
    SystemLogger* systemLogger;
    uint16_t serverPort;
    bool running;
    bool enabled;

    // Request handlers
    void handleRoot();
    void handleStatus();
    void handleEngines();
    void handleLogs();
    void handleLogsAPI();
    void handleStartEngine();
    void handleStopEngine();
    void handleConfig();
    void handleAPI();
    void handleNotFound();

    // Helper methods
    String getSystemStatusJSON();
    String getEngineStatusJSON();
    String generateHTML(const String& title, const String& content);
    String getHTMLHeader();
    String getHTMLFooter();
};

#endif // WEBINTERFACE_H
