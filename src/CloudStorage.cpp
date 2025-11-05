#include "CloudStorage.h"
#include <time.h>

CloudStorage::CloudStorage()
    : secureClient(nullptr), httpClient(nullptr) {
}

CloudStorage::~CloudStorage() {
    if (httpClient != nullptr) {
        delete httpClient;
    }
    if (secureClient != nullptr) {
        delete secureClient;
    }
}

bool CloudStorage::begin() {
    Serial.println("[CloudStorage] Initializing cloud storage...");

    secureClient = new WiFiClientSecure();
    secureClient->setInsecure(); // Skip cert validation (use with caution in production!)

    httpClient = new HTTPClient();

    bool anyEnabled = false;

#if NET_ENABLE_WEBDAV
    Serial.println("[CloudStorage] WebDAV enabled");
    anyEnabled = true;
#endif

#if NET_ENABLE_S3
    Serial.println("[CloudStorage] S3-Compatible API enabled");
    anyEnabled = true;
#endif

#if NET_ENABLE_HTTP_UPLOAD
    Serial.println("[CloudStorage] HTTP Upload enabled");
    anyEnabled = true;
#endif

    if (!anyEnabled) {
        Serial.println("[CloudStorage] WARNING: No cloud protocols enabled");
        return false;
    }

    Serial.println("[CloudStorage] Cloud storage initialized");
    return true;
}

bool CloudStorage::uploadFile(const uint8_t* data, size_t size, const String& filename, const String& contentType) {
    if (data == nullptr || size == 0) {
        lastError = "Invalid data";
        return false;
    }

    Serial.print("[CloudStorage] Uploading: ");
    Serial.print(filename);
    Serial.print(" (");
    Serial.print(size);
    Serial.println(" bytes)");

    bool success = false;

#if NET_ENABLE_WEBDAV
    if (!success) {
        Serial.println("[CloudStorage] Trying WebDAV...");
        success = uploadViaWebDAV(data, size, filename);
    }
#endif

#if NET_ENABLE_S3
    if (!success) {
        Serial.println("[CloudStorage] Trying S3...");
        success = uploadViaS3(data, size, filename, contentType);
    }
#endif

#if NET_ENABLE_HTTP_UPLOAD
    if (!success) {
        Serial.println("[CloudStorage] Trying HTTP...");
        success = uploadViaHTTP(data, size, filename, contentType);
    }
#endif

    if (success) {
        Serial.println("[CloudStorage] Upload successful!");
    } else {
        Serial.print("[CloudStorage] Upload failed: ");
        Serial.println(lastError);
    }

    return success;
}

bool CloudStorage::uploadText(const String& text, const String& filename) {
    return uploadFile((const uint8_t*)text.c_str(), text.length(), filename, "text/plain");
}

bool CloudStorage::uploadPCAP(const uint8_t* pcapData, size_t size, const String& filename) {
    String pcapFilename = filename.endsWith(".pcap") ? filename : filename + ".pcap";
    return uploadFile(pcapData, size, pcapFilename, "application/vnd.tcpdump.pcap");
}

bool CloudStorage::uploadJSON(const String& json, const String& filename) {
    String jsonFilename = filename.endsWith(".json") ? filename : filename + ".json";
    return uploadFile((const uint8_t*)json.c_str(), json.length(), jsonFilename, "application/json");
}

bool CloudStorage::testConnection() {
    Serial.println("[CloudStorage] Testing connection...");
    String testData = "{\"test\":true,\"timestamp\":" + String(millis()) + "}";
    return uploadJSON(testData, "test_" + String(millis()) + ".json");
}

// ==================== WEBDAV UPLOAD ====================

bool CloudStorage::uploadViaWebDAV(const uint8_t* data, size_t size, const String& filename) {
#if NET_ENABLE_WEBDAV
    String url = String(NET_WEBDAV_URL);
    if (!url.endsWith("/")) url += "/";
    url += filename;

    httpClient->begin(*secureClient, url);
    httpClient->setAuthorization(NET_WEBDAV_USERNAME, NET_WEBDAV_PASSWORD);
    httpClient->addHeader("Content-Type", "application/octet-stream");

    int httpCode = httpClient->PUT((uint8_t*)data, size);
    String response = httpClient->getString();
    httpClient->end();

    if (httpCode == 200 || httpCode == 201 || httpCode == 204) {
        Serial.println("[CloudStorage] WebDAV upload OK");
        return true;
    } else {
        lastError = "WebDAV HTTP " + String(httpCode) + ": " + response;
        return false;
    }
#else
    lastError = "WebDAV not enabled";
    return false;
#endif
}

// ==================== S3-COMPATIBLE UPLOAD ====================

bool CloudStorage::uploadViaS3(const uint8_t* data, size_t size, const String& filename, const String& contentType) {
#if NET_ENABLE_S3
    // S3 upload requires AWS Signature Version 4
    // This is a simplified implementation - for production use a proper S3 library

    String bucket = NET_S3_BUCKET;
    String endpoint = NET_S3_ENDPOINT;
    String region = NET_S3_REGION;

    String url;
    if (NET_S3_USE_PATH_STYLE) {
        // Path-style (MinIO): https://endpoint/bucket/file
        url = "https://" + endpoint + "/" + bucket + "/" + filename;
    } else {
        // Virtual-hosted style (AWS): https://bucket.endpoint/file
        url = "https://" + bucket + "." + endpoint + "/" + filename;
    }

    httpClient->begin(*secureClient, url);
    httpClient->addHeader("Content-Type", contentType);

    // For simple S3 upload without signature (public bucket or pre-signed URL)
    // For production, implement AWS Signature V4

    int httpCode = httpClient->PUT((uint8_t*)data, size);
    String response = httpClient->getString();
    httpClient->end();

    if (httpCode == 200) {
        Serial.println("[CloudStorage] S3 upload OK");
        return true;
    } else {
        lastError = "S3 HTTP " + String(httpCode) + ": " + response;
        Serial.println("[CloudStorage] Note: S3 requires proper signature implementation");
        return false;
    }
#else
    lastError = "S3 not enabled";
    return false;
#endif
}

// ==================== HTTP POST/PUT UPLOAD ====================

bool CloudStorage::uploadViaHTTP(const uint8_t* data, size_t size, const String& filename, const String& contentType) {
#if NET_ENABLE_HTTP_UPLOAD
    httpClient->begin(*secureClient, NET_HTTP_UPLOAD_URL);
    httpClient->addHeader(NET_HTTP_UPLOAD_AUTH_HEADER, NET_HTTP_UPLOAD_AUTH_VALUE);
    httpClient->addHeader("Content-Type", contentType);
    httpClient->addHeader("X-Filename", filename);

    int httpCode;
    if (String(NET_HTTP_UPLOAD_METHOD) == "PUT") {
        httpCode = httpClient->PUT((uint8_t*)data, size);
    } else {
        httpCode = httpClient->POST((uint8_t*)data, size);
    }

    String response = httpClient->getString();
    httpClient->end();

    if (httpCode == 200 || httpCode == 201 || httpCode == 204) {
        Serial.println("[CloudStorage] HTTP upload OK");
        return true;
    } else {
        lastError = "HTTP " + String(httpCode) + ": " + response;
        return false;
    }
#else
    lastError = "HTTP upload not enabled";
    return false;
#endif
}

// ==================== HELPER METHODS ====================

String CloudStorage::calculateS3Signature(const String& method, const String& path, const String& dateStamp) {
    // Placeholder for AWS Signature Version 4 implementation
    // Requires HMAC-SHA256 hashing
    // Reference: https://docs.aws.amazon.com/general/latest/gr/signature-version-4.html
    return "";
}

String CloudStorage::hmacSHA256(const String& key, const String& message) {
    // Placeholder for HMAC-SHA256 implementation
    // Requires mbedTLS or similar crypto library
    return "";
}

String CloudStorage::generateFilename(const String& prefix, const String& extension) {
    time_t now = time(nullptr);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
    return prefix + "_" + String(timestamp) + "." + extension;
}

String CloudStorage::urlEncode(const String& str) {
    String encoded = "";
    for (size_t i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char buf[4];
            sprintf(buf, "%%%02X", c);
            encoded += buf;
        }
    }
    return encoded;
}
