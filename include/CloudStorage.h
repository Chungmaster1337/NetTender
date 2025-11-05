#ifndef CLOUD_STORAGE_H
#define CLOUD_STORAGE_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "NetworkConfig.h"

/**
 * @brief Cloud storage integration using universal protocols
 *
 * Supports three universal protocols that work with ANY cloud provider:
 * 1. WebDAV - NextCloud, ownCloud, Box, pCloud, Yandex, Koofr, Google Drive (with addon)
 * 2. S3-Compatible - AWS S3, MinIO, Wasabi, Backblaze B2, DigitalOcean, Cloudflare R2, Storj
 * 3. HTTP POST/PUT - Any custom endpoint you build
 *
 * No provider-specific integrations needed!
 */
class CloudStorage {
public:
    CloudStorage();
    ~CloudStorage();

    /**
     * @brief Initialize cloud storage based on enabled providers
     */
    bool begin();

    /**
     * @brief Upload a file to cloud storage
     * @param data File data buffer
     * @param size Size of data in bytes
     * @param filename Filename to save as
     * @param contentType MIME type (e.g., "application/octet-stream")
     * @return true if upload successful
     */
    bool uploadFile(const uint8_t* data, size_t size, const String& filename, const String& contentType = "application/octet-stream");

    /**
     * @brief Upload text data to cloud storage
     * @param text Text content to upload
     * @param filename Filename to save as
     * @return true if upload successful
     */
    bool uploadText(const String& text, const String& filename);

    /**
     * @brief Upload PCAP file to cloud storage
     * @param pcapData PCAP file data
     * @param size Size of PCAP data
     * @param filename Filename to save as
     * @return true if upload successful
     */
    bool uploadPCAP(const uint8_t* pcapData, size_t size, const String& filename);

    /**
     * @brief Upload JSON data to cloud storage
     * @param json JSON string
     * @param filename Filename to save as
     * @return true if upload successful
     */
    bool uploadJSON(const String& json, const String& filename);

    /**
     * @brief Get last upload status message
     */
    String getLastError() const { return lastError; }

    /**
     * @brief Test cloud connection
     * @return true if connection successful
     */
    bool testConnection();

private:
    WiFiClientSecure* secureClient;
    HTTPClient* httpClient;
    String lastError;

    // Universal protocol upload methods
    bool uploadViaWebDAV(const uint8_t* data, size_t size, const String& filename);
    bool uploadViaS3(const uint8_t* data, size_t size, const String& filename, const String& contentType);
    bool uploadViaHTTP(const uint8_t* data, size_t size, const String& filename, const String& contentType);

    // S3 signature helpers
    String calculateS3Signature(const String& method, const String& path, const String& dateStamp);
    String hmacSHA256(const String& key, const String& message);

    // Helper methods
    String generateFilename(const String& prefix, const String& extension);
    String urlEncode(const String& str);
};

#endif // CLOUD_STORAGE_H
