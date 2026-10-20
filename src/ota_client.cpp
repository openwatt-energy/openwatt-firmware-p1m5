#include "ota_client.h"
#include "config.h"
#include "debug_logger.h"

#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>


// ============================================================================
// OTA CONFIGURATION
// ============================================================================

#define OTA_FIRMWARE_URL \
    "https://api.example.com/api/p1m5/firmware"

// HTTPClient::setTimeout() utilise un uint16_t sur cette plateforme.
// La valeur doit donc rester <= 65535 ms.
//
// 60 secondes = 60000 ms.
#define OTA_TIMEOUT_MS 60000


// ============================================================================
// STATIC DATA
// ============================================================================

String OTAClient::firmwareURL = OTA_FIRMWARE_URL;


// ============================================================================
// CONFIGURATION
// ============================================================================

void OTAClient::setFirmwareURL(const String& url) {

    firmwareURL = url;
}


// ============================================================================
// MAC ADDRESS
// ============================================================================

String OTAClient::getMacAddressString() {

    uint8_t mac[6];

    WiFi.macAddress(mac);

    char macStr[18];

    snprintf(
        macStr,
        sizeof(macStr),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );

    return String(macStr);
}


// ============================================================================
// UPDATE HEADERS
// ============================================================================

void OTAClient::setUpdateHeaders(
    HTTPClient& client,
    const String& deviceSerial,
    const String& currentVersion,
    const String& macAddress
) {

    client.addHeader(
        "User-Agent",
        "Xenn-Update"
    );

    client.addHeader(
        "Cache-Control",
        "no-cache"
    );

    client.addHeader(
        "xenn-target",
        "esp32"
    );

    client.addHeader(
        "xenn-version",
        currentVersion
    );

    client.addHeader(
        "xenn-serial",
        deviceSerial
    );

    client.addHeader(
        "xenn-mac",
        macAddress
    );
}


// ============================================================================
// CHECK FOR UPDATE
// ============================================================================

OTAInfo OTAClient::checkForUpdate(
    const String& deviceSerial,
    const String& currentVersion,
    const String& macAddress
) {

    OTAInfo info;

    info.updateAvailable = false;


    // ------------------------------------------------------------------------
    // WiFi check
    // ------------------------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED) {

        LOG_WARN(
            MODULE_OTA,
            "WiFi not connected, cannot check for updates"
        );

        return info;
    }


    // ------------------------------------------------------------------------
    // HTTP client
    // ------------------------------------------------------------------------

    HTTPClient http;

    http.begin(firmwareURL);

    http.setTimeout(OTA_TIMEOUT_MS);

    http.setFollowRedirects(
        HTTPC_STRICT_FOLLOW_REDIRECTS
    );


    // ------------------------------------------------------------------------
    // Headers
    // ------------------------------------------------------------------------

    setUpdateHeaders(
        http,
        deviceSerial,
        currentVersion,
        macAddress
    );


    LOG_INFO(
        MODULE_OTA,
        "Checking for firmware update..."
    );

    LOG_DEBUG(
        MODULE_OTA,
        "  Serial: %s",
        deviceSerial.c_str()
    );

    LOG_DEBUG(
        MODULE_OTA,
        "  Version: %s",
        currentVersion.c_str()
    );

    LOG_DEBUG(
        MODULE_OTA,
        "  MAC: %s",
        macAddress.c_str()
    );


    // ------------------------------------------------------------------------
    // HEAD request
    // ------------------------------------------------------------------------

    int httpCode =
        http.sendRequest("HEAD");


    if (httpCode == 200) {

        // Update available

        info.updateAvailable = true;

        info.firmwareName =
            http.header("Xenn-Firmware-Name");

        info.firmwareTag =
            http.header("Xenn-Firmware-Tag");

        info.firmwareDate =
            http.header("Xenn-Firmware-Date");


        LOG_INFO(
            MODULE_OTA,
            "Update available:"
        );

        LOG_INFO(
            MODULE_OTA,
            "  Name: %s",
            info.firmwareName.c_str()
        );

        LOG_INFO(
            MODULE_OTA,
            "  Tag: %s",
            info.firmwareTag.c_str()
        );

        LOG_INFO(
            MODULE_OTA,
            "  Date: %s",
            info.firmwareDate.c_str()
        );

    } else if (httpCode == 304) {

        LOG_INFO(
            MODULE_OTA,
            "Firmware is up to date"
        );

        info.updateAvailable = false;

    } else {

        LOG_ERROR(
            MODULE_OTA,
            "Failed to check for update, HTTP code: %d",
            httpCode
        );
    }


    http.end();

    return info;
}


// ============================================================================
// DOWNLOAD AND APPLY UPDATE
// ============================================================================

bool OTAClient::downloadAndApply(
    const String& deviceSerial,
    const String& currentVersion,
    const String& macAddress
) {

    // ------------------------------------------------------------------------
    // WiFi check
    // ------------------------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED) {

        LOG_ERROR(
            MODULE_OTA,
            "WiFi not connected, cannot download update"
        );

        return false;
    }


    // ------------------------------------------------------------------------
    // HTTP client
    // ------------------------------------------------------------------------

    HTTPClient http;

    http.begin(firmwareURL);

    http.setTimeout(OTA_TIMEOUT_MS);

    http.setFollowRedirects(
        HTTPC_STRICT_FOLLOW_REDIRECTS
    );


    // ------------------------------------------------------------------------
    // Headers
    // ------------------------------------------------------------------------

    setUpdateHeaders(
        http,
        deviceSerial,
        currentVersion,
        macAddress
    );


    LOG_INFO(
        MODULE_OTA,
        "Downloading firmware update..."
    );


    // ------------------------------------------------------------------------
    // GET request
    // ------------------------------------------------------------------------

    int httpCode = http.GET();


    if (httpCode != 200) {

        LOG_ERROR(
            MODULE_OTA,
            "Failed to download firmware, HTTP code: %d",
            httpCode
        );

        http.end();

        return false;
    }


    // ------------------------------------------------------------------------
    // Firmware information
    // ------------------------------------------------------------------------

    String firmwareName =
        http.header("Xenn-Firmware-Name");

    String firmwareTag =
        http.header("Xenn-Firmware-Tag");

    String firmwareDate =
        http.header("Xenn-Firmware-Date");


    LOG_INFO(
        MODULE_OTA,
        "Firmware info:"
    );

    LOG_INFO(
        MODULE_OTA,
        "  Name: %s",
        firmwareName.c_str()
    );

    LOG_INFO(
        MODULE_OTA,
        "  Tag: %s",
        firmwareTag.c_str()
    );

    LOG_INFO(
        MODULE_OTA,
        "  Date: %s",
        firmwareDate.c_str()
    );


    // ------------------------------------------------------------------------
    // Content length
    // ------------------------------------------------------------------------

    int contentLength =
        http.getSize();


    if (contentLength <= 0) {

        LOG_ERROR(
            MODULE_OTA,
            "Invalid content length: %d",
            contentLength
        );

        http.end();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Content length: %d bytes",
        contentLength
    );


    // ------------------------------------------------------------------------
    // Free space check
    // ------------------------------------------------------------------------

    if (
        contentLength > UPDATE_SIZE_UNKNOWN &&
        contentLength >
            (ESP.getFreeSketchSpace() - 0x1000)
    ) {

        LOG_ERROR(
            MODULE_OTA,
            "Not enough space for update. Required: %d, Available: %d",
            contentLength,
            ESP.getFreeSketchSpace()
        );

        http.end();

        return false;
    }


    // ------------------------------------------------------------------------
    // Start OTA
    // ------------------------------------------------------------------------

    size_t updateSize =
        (contentLength > 0)
            ? contentLength
            : UPDATE_SIZE_UNKNOWN;


    if (!Update.begin(updateSize)) {

        LOG_ERROR(
            MODULE_OTA,
            "Update.begin() failed: %s",
            Update.errorString()
        );

        http.end();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Starting firmware update..."
    );


    // ------------------------------------------------------------------------
    // Stream download
    // ------------------------------------------------------------------------

    WiFiClient* stream =
        http.getStreamPtr();

    size_t written = 0;

    uint8_t buffer[1024];

    // Important :
    // cette variable appartient uniquement
    // au téléchargement courant.
    int lastPercent = -1;


    while (
        http.connected() &&
        (
            written < (size_t)contentLength ||
            contentLength == -1
        )
    ) {

        size_t available =
            stream->available();


        if (available) {

            size_t bytesToRead =
                min(
                    available,
                    sizeof(buffer)
                );


            int c =
                stream->readBytes(
                    buffer,
                    bytesToRead
                );


            if (c > 0) {

                size_t updateWritten =
                    Update.write(
                        buffer,
                        c
                    );


                if (updateWritten != (size_t)c) {

                    LOG_ERROR(
                        MODULE_OTA,
                        "Update.write() failed"
                    );

                    http.end();

                    Update.abort();

                    return false;
                }


                written += updateWritten;


                // ------------------------------------------------------------
                // Progress
                // ------------------------------------------------------------

                if (contentLength > 0) {

                    int percent =
                        (written * 100) /
                        contentLength;


                    if (
                        percent >=
                        lastPercent + 10
                    ) {

                        LOG_INFO(
                            MODULE_OTA,
                            "Progress: %d%% (%d/%d bytes)",
                            percent,
                            (int)written,
                            contentLength
                        );

                        lastPercent =
                            percent;
                    }
                }
            }

        } else {

            delay(1);
        }
    }


    http.end();


    // ------------------------------------------------------------------------
    // Validate download
    // ------------------------------------------------------------------------

    if (written == 0) {

        LOG_ERROR(
            MODULE_OTA,
            "No data received"
        );

        Update.abort();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Downloaded %d bytes",
        (int)written
    );


    // ------------------------------------------------------------------------
    // Finish update
    // ------------------------------------------------------------------------

    if (!Update.end()) {

        LOG_ERROR(
            MODULE_OTA,
            "Update.end() failed: %s",
            Update.errorString()
        );

        return false;
    }


    if (!Update.isFinished()) {

        LOG_ERROR(
            MODULE_OTA,
            "Update not finished"
        );

        Update.abort();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Firmware update successful! Rebooting..."
    );


    delay(1000);

    ESP.restart();

    return true;
}


// ============================================================================
// DOWNLOAD AND APPLY FROM CUSTOM URL
// ============================================================================

bool OTAClient::downloadAndApplyFromURL(
    const String& url
) {

    // ------------------------------------------------------------------------
    // WiFi check
    // ------------------------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED) {

        LOG_ERROR(
            MODULE_OTA,
            "WiFi not connected, cannot download update"
        );

        return false;
    }


    // ------------------------------------------------------------------------
    // URL check
    // ------------------------------------------------------------------------

    if (url.length() == 0) {

        LOG_ERROR(
            MODULE_OTA,
            "Empty URL"
        );

        return false;
    }


    // ------------------------------------------------------------------------
    // HTTP client
    // ------------------------------------------------------------------------

    HTTPClient http;

    http.begin(url);

    http.setTimeout(OTA_TIMEOUT_MS);

    http.setFollowRedirects(
        HTTPC_STRICT_FOLLOW_REDIRECTS
    );


    LOG_INFO(
        MODULE_OTA,
        "Downloading from %s",
        url.c_str()
    );


    // ------------------------------------------------------------------------
    // GET request
    // ------------------------------------------------------------------------

    int httpCode =
        http.GET();


    if (httpCode != 200) {

        LOG_ERROR(
            MODULE_OTA,
            "Failed to download, HTTP code: %d",
            httpCode
        );

        http.end();

        return false;
    }


    // ------------------------------------------------------------------------
    // Content length
    // ------------------------------------------------------------------------

    int contentLength =
        http.getSize();


    if (contentLength <= 0) {

        LOG_ERROR(
            MODULE_OTA,
            "Invalid content length: %d",
            contentLength
        );

        http.end();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Content length: %d bytes",
        contentLength
    );


    // ------------------------------------------------------------------------
    // Free space check
    // ------------------------------------------------------------------------

    if (
        contentLength > UPDATE_SIZE_UNKNOWN &&
        contentLength >
            (ESP.getFreeSketchSpace() - 0x1000)
    ) {

        LOG_ERROR(
            MODULE_OTA,
            "Not enough space. Required: %d, Available: %d",
            contentLength,
            ESP.getFreeSketchSpace()
        );

        http.end();

        return false;
    }


    // ------------------------------------------------------------------------
    // Start OTA
    // ------------------------------------------------------------------------

    size_t updateSize =
        (contentLength > 0)
            ? contentLength
            : UPDATE_SIZE_UNKNOWN;


    if (!Update.begin(updateSize)) {

        LOG_ERROR(
            MODULE_OTA,
            "Update.begin() failed: %s",
            Update.errorString()
        );

        http.end();

        return false;
    }


    // ------------------------------------------------------------------------
    // Stream download
    // ------------------------------------------------------------------------

    WiFiClient* stream =
        http.getStreamPtr();

    size_t written = 0;

    uint8_t buffer[1024];

    int lastPercent = -1;


    while (
        http.connected() &&
        (
            written < (size_t)contentLength ||
            contentLength == -1
        )
    ) {

        size_t available =
            stream->available();


        if (available) {

            size_t bytesToRead =
                min(
                    available,
                    sizeof(buffer)
                );


            int c =
                stream->readBytes(
                    buffer,
                    bytesToRead
                );


            if (c > 0) {

                size_t updateWritten =
                    Update.write(
                        buffer,
                        c
                    );


                if (updateWritten != (size_t)c) {

                    LOG_ERROR(
                        MODULE_OTA,
                        "Update.write() failed"
                    );

                    http.end();

                    Update.abort();

                    return false;
                }


                written += updateWritten;


                if (contentLength > 0) {

                    int percent =
                        (written * 100) /
                        contentLength;


                    if (
                        percent >=
                        lastPercent + 10
                    ) {

                        LOG_INFO(
                            MODULE_OTA,
                            "Progress: %d%% (%d/%d bytes)",
                            percent,
                            (int)written,
                            contentLength
                        );

                        lastPercent =
                            percent;
                    }
                }
            }

        } else {

            delay(1);
        }
    }


    http.end();


    // ------------------------------------------------------------------------
    // Validate download
    // ------------------------------------------------------------------------

    if (written == 0) {

        LOG_ERROR(
            MODULE_OTA,
            "No data received"
        );

        Update.abort();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Downloaded %d bytes",
        (int)written
    );


    // ------------------------------------------------------------------------
    // Finish update
    // ------------------------------------------------------------------------

    if (!Update.end()) {

        LOG_ERROR(
            MODULE_OTA,
            "Update.end() failed: %s",
            Update.errorString()
        );

        return false;
    }


    if (!Update.isFinished()) {

        LOG_ERROR(
            MODULE_OTA,
            "Update not finished"
        );

        Update.abort();

        return false;
    }


    LOG_INFO(
        MODULE_OTA,
        "Update successful! Rebooting..."
    );


    delay(1000);

    ESP.restart();

    return true;
}
