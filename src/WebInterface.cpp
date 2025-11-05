#include "WebInterface.h"
#include "EngineManager.h"
#include "SystemLogger.h"
#include <ArduinoJson.h>

WebServerManager::WebServerManager(EngineManager* engineMgr, SystemLogger* logger, uint16_t port)
    : engineManager(engineMgr), systemLogger(logger), serverPort(port), running(false), enabled(true) {
    server = new WebServer(serverPort);
}

WebServerManager::~WebServerManager() {
    stop();
    if (server != nullptr) {
        delete server;
    }
}

bool WebServerManager::begin() {
    if (!enabled) {
        Serial.println("[WebServer] Web server is disabled");
        return false;
    }

    Serial.println("[WebServer] Starting web server...");

    // Set up routes
    server->on("/", [this]() { handleRoot(); });
    server->on("/status", [this]() { handleStatus(); });
    server->on("/engines", [this]() { handleEngines(); });
    server->on("/logs", [this]() { handleLogs(); });
    server->on("/api/logs", [this]() { handleLogsAPI(); });
    server->on("/api/start", HTTP_POST, [this]() { handleStartEngine(); });
    server->on("/api/stop", HTTP_POST, [this]() { handleStopEngine(); });
    server->on("/api/config", [this]() { handleConfig(); });
    server->on("/api", [this]() { handleAPI(); });
    server->onNotFound([this]() { handleNotFound(); });

    // Start server
    server->begin();
    running = true;

    // Start mDNS responder
    if (MDNS.begin("esp32")) {
        Serial.println("[WebServer] mDNS responder started: http://esp32.local");
        MDNS.addService("http", "tcp", serverPort);
    }

    Serial.print("[WebServer] Web server started on port ");
    Serial.println(serverPort);
    Serial.print("[WebServer] Access at: http://");
    Serial.println(WiFi.localIP());

    return true;
}

void WebServerManager::stop() {
    if (running) {
        Serial.println("[WebServer] Stopping web server...");
        server->stop();
        MDNS.end();
        running = false;
    }
}

void WebServerManager::handleClient() {
    if (running && enabled) {
        server->handleClient();
    }
}

void WebServerManager::setEnabled(bool en) {
    enabled = en;
    if (!enabled && running) {
        stop();
    }
}

// ==================== REQUEST HANDLERS ====================

void WebServerManager::handleRoot() {
    String html = getHTMLHeader();

    html += "<div class='container'>";
    html += "<h1>ESP32 Tri-Engine Platform</h1>";
    html += "<div class='card'>";
    html += "<h2>System Status</h2>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
    html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";

    if (engineManager != nullptr) {
        html += "<p><strong>Active Engine:</strong> ";
        if (engineManager->isEngineActive()) {
            html += String((int)engineManager->getCurrentEngine());
        } else {
            html += "None (Menu)";
        }
        html += "</p>";
    }

    html += "</div>";

    html += "<div class='card'>";
    html += "<h2>Available Engines</h2>";
    html += "<ul>";
    html += "<li><a href='/engines?id=1'>1. RF Scanner</a></li>";
    html += "<li><a href='/engines?id=2'>2. Network Analyzer</a></li>";
    html += "<li><a href='/engines?id=3'>3. Emergency Router</a></li>";
    html += "</ul>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h2>Quick Actions</h2>";
    html += "<button onclick=\"fetch('/api/stop', {method: 'POST'}).then(() => location.reload())\">Stop Current Engine</button> ";
    html += "<button onclick=\"location.href='/status'\">View Status</button> ";
    html += "<button onclick=\"location.href='/logs'\">View Logs</button> ";
    html += "<button onclick=\"location.href='/api'\">API Documentation</button>";
    html += "</div>";

    html += "</div>";
    html += getHTMLFooter();

    server->send(200, "text/html", html);
}

void WebServerManager::handleStatus() {
    String html = getHTMLHeader();

    html += "<div class='container'>";
    html += "<h1>System Status</h1>";

    html += "<div class='card'>";
    html += "<h2>Hardware</h2>";
    html += "<table>";
    html += "<tr><td>Chip Model:</td><td>" + String(ESP.getChipModel()) + "</td></tr>";
    html += "<tr><td>CPU Frequency:</td><td>" + String(ESP.getCpuFreqMHz()) + " MHz</td></tr>";
    html += "<tr><td>Flash Size:</td><td>" + String(ESP.getFlashChipSize()) + " bytes</td></tr>";
    html += "<tr><td>Free Heap:</td><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>";
    html += "</table>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h2>Network</h2>";
    html += "<table>";
    html += "<tr><td>WiFi Status:</td><td>" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</td></tr>";
    html += "<tr><td>IP Address:</td><td>" + WiFi.localIP().toString() + "</td></tr>";
    html += "<tr><td>MAC Address:</td><td>" + WiFi.macAddress() + "</td></tr>";
    html += "<tr><td>RSSI:</td><td>" + String(WiFi.RSSI()) + " dBm</td></tr>";
    html += "</table>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h2>Engine Status</h2>";
    html += "<p><strong>Current Engine:</strong> ";
    if (engineManager != nullptr && engineManager->isEngineActive()) {
        int engineId = (int)engineManager->getCurrentEngine();
        String engineName = "Unknown";
        switch(engineId) {
            case 1: engineName = "RF Scanner"; break;
            case 2: engineName = "Network Analyzer"; break;
            case 3: engineName = "Emergency Router"; break;
        }
        html += engineName + " (ID: " + String(engineId) + ")";
    } else {
        html += "None (Menu)";
    }
    html += "</p>";
    html += "</div>";

    html += "<p><a href='/'>Back to Home</a></p>";
    html += "</div>";
    html += getHTMLFooter();

    server->send(200, "text/html", html);
}

void WebServerManager::handleEngines() {
    if (server->hasArg("id")) {
        int engineId = server->arg("id").toInt();

        String html = getHTMLHeader();
        html += "<div class='container'>";
        html += "<h1>Engine Control</h1>";
        html += "<div class='card'>";

        String engineName = "Unknown";
        switch(engineId) {
            case 1: engineName = "RF Scanner"; break;
            case 2: engineName = "Network Analyzer"; break;
            case 3: engineName = "Emergency Router"; break;
        }

        html += "<h2>" + engineName + "</h2>";
        html += "<p>Engine ID: " + String(engineId) + "</p>";

        html += "<form method='POST' action='/api/start'>";
        html += "<input type='hidden' name='engine' value='" + String(engineId) + "'>";
        html += "<button type='submit'>Start Engine</button>";
        html += "</form>";

        html += "</div>";
        html += "<p><a href='/'>Back to Home</a></p>";
        html += "</div>";
        html += getHTMLFooter();

        server->send(200, "text/html", html);
    } else {
        server->send(400, "text/plain", "Missing engine ID parameter");
    }
}

void WebServerManager::handleStartEngine() {
    if (server->hasArg("engine")) {
        int engineId = server->arg("engine").toInt();

        if (engineId >= 1 && engineId <= 3) {
            if (engineManager != nullptr) {
                EngineType type = (EngineType)engineId;
                bool success = engineManager->loadEngine(type);

                if (success) {
                    server->sendHeader("Location", "/", true);
                    server->send(302, "text/plain", "Engine started");
                } else {
                    server->send(500, "text/plain", "Failed to start engine");
                }
            } else {
                server->send(500, "text/plain", "Engine manager not available");
            }
        } else {
            server->send(400, "text/plain", "Invalid engine ID");
        }
    } else {
        server->send(400, "text/plain", "Missing engine parameter");
    }
}

void WebServerManager::handleStopEngine() {
    if (engineManager != nullptr) {
        engineManager->returnToMenu();
        server->send(200, "text/plain", "Engine stopped");
    } else {
        server->send(500, "text/plain", "Engine manager not available");
    }
}

void WebServerManager::handleConfig() {
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"ipAddress\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"macAddress\":\"" + WiFi.macAddress() + "\"";
    json += "}";

    server->send(200, "application/json", json);
}

void WebServerManager::handleLogs() {
    String html = getHTMLHeader();

    html += "<div class='container'>";
    html += "<h1>System Logs</h1>";

    if (systemLogger == nullptr) {
        html += "<p>Logger not available</p>";
    } else {
        // Engine health status
        html += "<div class='card'>";
        html += "<h2>Engine Health</h2>";
        html += "<table>";
        html += "<tr><th>Engine</th><th>Status</th><th>Errors</th><th>Warnings</th></tr>";

        const auto& engineHealth = systemLogger->getEngineHealth();
        for (const auto& engine : engineHealth) {
            String statusClass = engine.operational ? (engine.errorCount > 0 ? "warn" : "ok") : "error";
            String status = engine.operational ? "OPERATIONAL" : "OFFLINE";

            if (engine.operational && !engine.responsive) {
                status = "UNRESPONSIVE";
                statusClass = "error";
            }

            html += "<tr>";
            html += "<td>" + engine.name + "</td>";
            html += "<td class='" + statusClass + "'>" + status + "</td>";
            html += "<td>" + String(engine.errorCount) + "</td>";
            html += "<td>" + String(engine.warningCount) + "</td>";
            html += "</tr>";
        }

        html += "</table>";
        html += "</div>";

        // Log entries
        html += "<div class='card'>";
        html += "<h2>Recent Logs</h2>";
        html += "<div style='max-height:400px; overflow-y:auto; font-family:monospace; font-size:12px;'>";

        const auto& logs = systemLogger->getAllLogs();
        int count = 0;
        for (auto it = logs.rbegin(); it != logs.rend() && count < 100; ++it, ++count) {
            String levelClass = "log-info";
            if (it->level == LogLevel::ERROR || it->level == LogLevel::CRITICAL) {
                levelClass = "log-error";
            } else if (it->level == LogLevel::WARNING) {
                levelClass = "log-warn";
            } else if (it->level == LogLevel::SUCCESS) {
                levelClass = "log-success";
            } else if (it->level == LogLevel::FLAGGED) {
                levelClass = "log-flagged";
            }

            html += "<div class='" + levelClass + "' style='padding:4px; margin:2px 0; border-left:3px solid;'>";
            html += "[" + it->getTimeString() + "] ";
            html += "[" + it->getLevelString() + "] ";
            html += "[" + it->engineName + "] ";
            html += it->message;
            html += "</div>";
        }

        html += "</div>";
        html += "<p><a href='/api/logs'>View as JSON</a> | <button onclick=\"location.reload()\">Refresh</button></p>";
        html += "</div>";
    }

    html += "<p><a href='/'>Back to Home</a></p>";
    html += "</div>";

    // Add CSS for log levels
    html += "<style>";
    html += ".log-error { border-left-color: #d32f2f !important; background: #ffebee; }";
    html += ".log-warn { border-left-color: #f57c00 !important; background: #fff3e0; }";
    html += ".log-success { border-left-color: #388e3c !important; background: #e8f5e9; }";
    html += ".log-flagged { border-left-color: #fbc02d !important; background: #fffde7; }";
    html += ".log-info { border-left-color: #1976d2 !important; background: #e3f2fd; }";
    html += ".ok { color: #388e3c; font-weight: bold; }";
    html += ".warn { color: #f57c00; font-weight: bold; }";
    html += ".error { color: #d32f2f; font-weight: bold; }";
    html += "</style>";

    html += getHTMLFooter();

    server->send(200, "text/html", html);
}

void WebServerManager::handleLogsAPI() {
    if (systemLogger == nullptr) {
        server->send(500, "application/json", "{\"error\":\"Logger not available\"}");
        return;
    }

    String json = systemLogger->exportLogsJSON();
    server->send(200, "application/json", json);
}

void WebServerManager::handleAPI() {
    String html = getHTMLHeader();

    html += "<div class='container'>";
    html += "<h1>API Documentation</h1>";

    html += "<div class='card'>";
    html += "<h2>Endpoints</h2>";
    html += "<h3>GET /</h3>";
    html += "<p>Main dashboard</p>";

    html += "<h3>GET /status</h3>";
    html += "<p>System status page</p>";

    html += "<h3>GET /engines?id={1-3}</h3>";
    html += "<p>Engine control page</p>";

    html += "<h3>POST /api/start</h3>";
    html += "<p>Start an engine</p>";
    html += "<p>Parameters: <code>engine</code> (1-3)</p>";

    html += "<h3>POST /api/stop</h3>";
    html += "<p>Stop current engine and return to menu</p>";

    html += "<h3>GET /api/config</h3>";
    html += "<p>Get system configuration as JSON</p>";

    html += "</div>";
    html += "<p><a href='/'>Back to Home</a></p>";
    html += "</div>";
    html += getHTMLFooter();

    server->send(200, "text/html", html);
}

void WebServerManager::handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: " + server->uri() + "\n";
    message += "Method: " + String((server->method() == HTTP_GET) ? "GET" : "POST") + "\n";
    message += "Arguments: " + String(server->args()) + "\n";

    for (uint8_t i = 0; i < server->args(); i++) {
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }

    server->send(404, "text/plain", message);
}

// ==================== HELPER METHODS ====================

String WebServerManager::getSystemStatusJSON() {
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"ipAddress\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    return json;
}

String WebServerManager::getEngineStatusJSON() {
    String json = "{";
    if (engineManager != nullptr) {
        json += "\"active\":" + String(engineManager->isEngineActive() ? "true" : "false") + ",";
        json += "\"currentEngine\":" + String((int)engineManager->getCurrentEngine());
    } else {
        json += "\"active\":false,\"currentEngine\":0";
    }
    json += "}";
    return json;
}

String WebServerManager::generateHTML(const String& title, const String& content) {
    String html = getHTMLHeader();
    html += "<div class='container'>";
    html += "<h1>" + title + "</h1>";
    html += content;
    html += "</div>";
    html += getHTMLFooter();
    return html;
}

String WebServerManager::getHTMLHeader() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Tri-Engine Platform</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 0; background: #f0f0f0; }";
    html += ".container { max-width: 900px; margin: 20px auto; padding: 20px; }";
    html += ".card { background: white; border-radius: 8px; padding: 20px; margin: 15px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; margin-top: 0; }";
    html += "h2 { color: #555; border-bottom: 2px solid #007bff; padding-bottom: 10px; }";
    html += "h3 { color: #666; margin-top: 20px; }";
    html += "table { width: 100%; border-collapse: collapse; }";
    html += "td { padding: 8px; border-bottom: 1px solid #ddd; }";
    html += "td:first-child { font-weight: bold; width: 40%; }";
    html += "button { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; margin: 5px; }";
    html += "button:hover { background: #0056b3; }";
    html += "a { color: #007bff; text-decoration: none; }";
    html += "a:hover { text-decoration: underline; }";
    html += "code { background: #f4f4f4; padding: 2px 6px; border-radius: 3px; font-family: monospace; }";
    html += "ul { list-style-type: none; padding: 0; }";
    html += "li { padding: 10px; border-bottom: 1px solid #eee; }";
    html += "li:last-child { border-bottom: none; }";
    html += "</style>";
    html += "</head><body>";
    return html;
}

String WebServerManager::getHTMLFooter() {
    String html = "<div style='text-align: center; padding: 20px; color: #666;'>";
    html += "<p>ESP32 Tri-Engine Platform | Uptime: " + String(millis() / 1000) + "s</p>";
    html += "</div>";
    html += "</body></html>";
    return html;
}
