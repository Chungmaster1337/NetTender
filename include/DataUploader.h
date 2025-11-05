#ifndef DATA_UPLOADER_H
#define DATA_UPLOADER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class DataUploader {
public:
    DataUploader(const char* server_url);
    void begin();

    // Upload different data types
    bool uploadDeviceData(const String& mac, uint32_t packets, int8_t rssi, uint32_t timestamp);
    bool uploadScanEvent(const String& scanner_mac, const String& scan_type, uint32_t timestamp);
    bool uploadConnectionEvent(const String& mac, const String& event_type, int8_t rssi, uint32_t timestamp);
    bool uploadNetworkQuality(const String& mac, int8_t rssi, float loss_percentage, uint32_t timestamp);

    // Batch upload
    bool uploadBatch(JsonDocument& doc);

    // Configuration
    void setServerURL(const char* url);
    void setAuthToken(const char* token);
    void setCACertificate(const char* cert);
    void setUploadInterval(unsigned long interval_ms);

    // Status
    bool isConnected();
    unsigned long getLastUploadTime();
    uint32_t getFailedUploads();

private:
    String server_url;
    String auth_token;
    const char* ca_cert;
    unsigned long upload_interval;
    unsigned long last_upload;
    uint32_t failed_uploads;

    WiFiClientSecure client;
    HTTPClient https;

    bool sendRequest(const String& endpoint, JsonDocument& payload);
    String createPayload(JsonDocument& doc);
};

#endif
