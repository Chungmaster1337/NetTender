#include "TelnetServer.h"
#include "EngineManager.h"

TelnetServer::TelnetServer(EngineManager* engineMgr, uint16_t port)
    : engineManager(engineMgr), serverPort(port), running(false), enabled(true) {
    server = new WiFiServer(serverPort);
}

TelnetServer::~TelnetServer() {
    stop();
    if (server != nullptr) {
        delete server;
    }
}

bool TelnetServer::begin() {
    if (!enabled) {
        Serial.println("[Telnet] Telnet server is disabled");
        return false;
    }

    Serial.println("[Telnet] Starting telnet server...");

    server->begin();
    server->setNoDelay(true);
    running = true;

    Serial.print("[Telnet] Telnet server started on port ");
    Serial.println(serverPort);
    Serial.println("[Telnet] Connect with: telnet <IP> 23");

    return true;
}

void TelnetServer::stop() {
    if (running) {
        Serial.println("[Telnet] Stopping telnet server...");

        if (client && client.connected()) {
            client.println("\r\nServer shutting down...");
            client.stop();
        }

        server->stop();
        running = false;
    }
}

void TelnetServer::loop() {
    if (!running || !enabled) return;

    // Check for new client connections
    if (server->hasClient()) {
        // Disconnect existing client if present
        if (client && client.connected()) {
            WiFiClient newClient = server->available();
            newClient.println("Server busy. Disconnect existing session first.");
            newClient.stop();
        } else {
            client = server->available();
            Serial.println("[Telnet] Client connected");
            sendWelcome();
            sendPrompt();
        }
    }

    // Handle existing client
    if (client && client.connected()) {
        while (client.available()) {
            char c = client.read();
            processInput(c);
        }
    }
}

void TelnetServer::setEnabled(bool en) {
    enabled = en;
    if (!enabled && running) {
        stop();
    }
}

void TelnetServer::println(const String& message) {
    if (client && client.connected()) {
        client.println(message);
    }
}

// ==================== COMMAND PROCESSING ====================

void TelnetServer::processInput(char c) {
    // Handle backspace
    if (c == 0x08 || c == 0x7F) {
        if (commandBuffer.length() > 0) {
            commandBuffer.remove(commandBuffer.length() - 1);
            client.print("\b \b"); // Erase character on screen
        }
        return;
    }

    // Handle enter/newline
    if (c == '\n' || c == '\r') {
        if (commandBuffer.length() > 0) {
            client.println(); // New line
            handleCommand(commandBuffer);
            commandBuffer = "";
        }
        sendPrompt();
        return;
    }

    // Handle printable characters
    if (c >= 32 && c < 127) {
        commandBuffer += c;
        client.print(c); // Echo character
    }
}

void TelnetServer::handleCommand(const String& cmd) {
    String command = cmd;
    command.trim();
    command.toLowerCase();

    Serial.print("[Telnet] Command: ");
    Serial.println(command);

    if (command == "status") {
        cmdStatus();
    } else if (command == "engines") {
        cmdEngines();
    } else if (command.startsWith("start ")) {
        int engineId = command.substring(6).toInt();
        cmdStart(engineId);
    } else if (command == "stop") {
        cmdStop();
    } else if (command == "restart") {
        cmdRestart();
    } else if (command == "help") {
        cmdHelp();
    } else if (command == "clear" || command == "cls") {
        cmdClear();
    } else if (command == "exit" || command == "quit") {
        client.println("Goodbye!");
        client.stop();
    } else if (command.length() > 0) {
        client.print("Unknown command: ");
        client.println(command);
        client.println("Type 'help' for available commands");
    }
}

void TelnetServer::cmdStatus() {
    client.println("\r\n--- System Status ---");
    client.print("Uptime: ");
    client.print(millis() / 1000);
    client.println(" seconds");

    client.print("Free Heap: ");
    client.print(ESP.getFreeHeap());
    client.println(" bytes");

    client.print("WiFi Status: ");
    client.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    client.print("IP Address: ");
    client.println(WiFi.localIP());

    client.print("MAC Address: ");
    client.println(WiFi.macAddress());

    if (engineManager != nullptr) {
        client.print("Active Engine: ");
        if (engineManager->isEngineActive()) {
            int engineId = (int)engineManager->getCurrentEngine();
            String engineName = "Unknown";
            switch(engineId) {
                case 1: engineName = "RF Scanner"; break;
                case 2: engineName = "Network Analyzer"; break;
                case 3: engineName = "Emergency Router"; break;
            }
            client.print(engineName);
            client.print(" (ID: ");
            client.print(engineId);
            client.println(")");
        } else {
            client.println("None (Menu)");
        }
    }

    client.println("---");
}

void TelnetServer::cmdEngines() {
    client.println("\r\n--- Available Engines ---");
    client.println("1. RF Scanner (Flipper/Marauder-like)");
    client.println("2. Network Analyzer (MITM/DNS)");
    client.println("3. Emergency Router");
    client.println("\r\nUse 'start <id>' to launch an engine");
    client.println("---");
}

void TelnetServer::cmdStart(int engineId) {
    if (engineId < 1 || engineId > 3) {
        client.println("Invalid engine ID. Use 1, 2, or 3");
        return;
    }

    if (engineManager != nullptr) {
        client.print("Starting engine ");
        client.print(engineId);
        client.println("...");

        EngineType type = (EngineType)engineId;
        bool success = engineManager->loadEngine(type);

        if (success) {
            client.println("Engine started successfully");
        } else {
            client.println("ERROR: Failed to start engine");
        }
    } else {
        client.println("ERROR: Engine manager not available");
    }
}

void TelnetServer::cmdStop() {
    if (engineManager != nullptr) {
        if (engineManager->isEngineActive()) {
            client.println("Stopping current engine...");
            engineManager->returnToMenu();
            client.println("Engine stopped. Returned to menu");
        } else {
            client.println("No engine is currently running");
        }
    } else {
        client.println("ERROR: Engine manager not available");
    }
}

void TelnetServer::cmdRestart() {
    client.println("Restarting ESP32...");
    client.flush();
    delay(100);
    ESP.restart();
}

void TelnetServer::cmdHelp() {
    client.println("\r\n--- Available Commands ---");
    client.println("status         - Show system status");
    client.println("engines        - List available engines");
    client.println("start <1-3>    - Start an engine by ID");
    client.println("stop           - Stop current engine");
    client.println("restart        - Restart the ESP32");
    client.println("clear/cls      - Clear screen");
    client.println("help           - Show this help message");
    client.println("exit/quit      - Disconnect from telnet");
    client.println("---");
}

void TelnetServer::cmdClear() {
    // ANSI escape sequence to clear screen
    client.print("\033[2J\033[H");
    sendWelcome();
}

// ==================== HELPER METHODS ====================

void TelnetServer::sendPrompt() {
    client.print("esp32> ");
}

void TelnetServer::sendWelcome() {
    client.println("\r\n========================================");
    client.println("  ESP32 Tri-Engine Platform");
    client.println("  Telnet Console");
    client.println("========================================");
    client.print("  IP: ");
    client.println(WiFi.localIP());
    client.print("  Uptime: ");
    client.print(millis() / 1000);
    client.println(" seconds");
    client.println("========================================");
    client.println("\r\nType 'help' for available commands\r\n");
}
