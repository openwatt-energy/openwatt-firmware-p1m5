#include "mqtt_client.h"
#include "mqtt_auth.h"
#include "serial_console.h"
#include "debug_logger.h"
#include "mqtt_certificates.h"

#define DEFAULT_MQTT_PORT MQTT_BROKER_PORT

PubSubClient* MQTTClient::client = nullptr;
WiFiClientSecure MQTTClient::wifiClientSecure;
WiFiClient MQTTClient::wifiClientPlain;
MQTTConfig MQTTClient::config;
String MQTTClient::deviceId;
String MQTTClient::secretKey;
bool MQTTClient::useSecure = true;
MQTTMessageCallback MQTTClient::messageCallback = nullptr;


// ==========================================
// INITIALISATION MQTT
// ==========================================

void MQTTClient::begin(
    Preferences& prefs,
    const String& devId,
    const String& secKey
) {
    deviceId = devId;
    secretKey = secKey;

    // Configuration AWS IoT Core
    config.host = MQTT_BROKER_HOST;
    config.port = MQTT_BROKER_PORT;
    config.topic = MQTT_DEFAULT_TOPIC;
    config.username = deviceId;
    config.useTLS = true;

    // Sauvegarde dans NVS
    prefs.putString("mqtt_host", config.host);
    prefs.putUShort("mqtt_port", config.port);
    prefs.putString("mqtt_topic", config.topic);
    prefs.putBool("mqtt_use_tls", config.useTLS);
    prefs.putString("mqtt_username", config.username);

    LOG_INFO(MODULE_MQTT, "MQTT settings saved to NVS:");
    LOG_INFO(MODULE_MQTT, "  Host: %s", config.host.c_str());
    LOG_INFO(MODULE_MQTT, "  Port: %d", config.port);
    LOG_INFO(MODULE_MQTT, "  Topic: %s", config.topic.c_str());

    useSecure = true;

    // Authentification TLS avec certificats X.509
    wifiClientSecure.setCACert(AWS_ROOT_CA);
    wifiClientSecure.setCertificate(AWS_DEVICE_CERT);
    wifiClientSecure.setPrivateKey(AWS_PRIVATE_KEY);

    client = new PubSubClient(wifiClientSecure);

    if (config.host.length() > 0) {
        client->setServer(config.host.c_str(), config.port);
        client->setBufferSize(4096);

        LOG_INFO(MODULE_MQTT, "MQTT configured:");
        LOG_INFO(
            MODULE_MQTT,
            "  Host: %s:%d",
            config.host.c_str(),
            config.port
        );
        LOG_INFO(MODULE_MQTT, "  Topic: %s", config.topic.c_str());
        LOG_INFO(MODULE_MQTT, "  TLS: Yes (X.509)");
        LOG_INFO(MODULE_MQTT, "  Buffer: 4096 bytes");
    } else {
        LOG_WARN(MODULE_MQTT, "MQTT not configured");
    }
}


// ==========================================
// BOUCLE MQTT
// ==========================================

void MQTTClient::loop() {
    if (client) {
        client->loop();
    }
}


// ==========================================
// ANCIENNE AUTHENTIFICATION
// Conservée pour compatibilité
// mais non utilisée avec X.509
// ==========================================

String MQTTClient::generatePassword() {
    if (secretKey.length() == 0) {
        LOG_ERROR(
            MODULE_MQTT,
            "Secret key not set, cannot generate password"
        );
        return String("");
    }

    return MQTTAuth::generatePassword(deviceId, secretKey);
}


// ==========================================
// CALLBACK DES MESSAGES
// ==========================================

void MQTTClient::setMessageCallback(
    MQTTMessageCallback callback
) {
    messageCallback = callback;
}


void MQTTClient::mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
) {
    String payloadStr;
    payloadStr.reserve(length);

    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    String topicStr = String(topic);

    LOG_INFO(
        MODULE_MQTT,
        "Message received on topic: %s",
        topicStr.c_str()
    );

    if (messageCallback) {
        messageCallback(topicStr, payloadStr);
    }
}


// ==========================================
// ABONNEMENT AUX COMMANDES
// ==========================================

void MQTTClient::subscribeToTopics() {
    if (!client || !client->connected()) {
        return;
    }

    String cmdTopic = config.topic;

    if (!cmdTopic.endsWith("/")) {
        cmdTopic += "/";
    }

    cmdTopic += deviceId + "/cmd";

    if (client->subscribe(cmdTopic.c_str())) {
        LOG_INFO(
            MODULE_MQTT,
            "Subscribed to command topic: %s",
            cmdTopic.c_str()
        );
    } else {
        LOG_ERROR(
            MODULE_MQTT,
            "Failed to subscribe to command topic: %s",
            cmdTopic.c_str()
        );
    }
}


// ==========================================
// RECONNEXION MQTT AVEC X.509
// ==========================================

void MQTTClient::reconnect() {
    if (!client || config.host.length() == 0) {
        return;
    }

    // Une tentative toutes les 10 secondes maximum
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();

    if (now - lastReconnectAttempt < 10000) {
        return;
    }

    lastReconnectAttempt = now;

    if (!client->connected()) {
        LOG_INFO(
            MODULE_MQTT,
            "Attempting MQTT X.509 connection to %s:%d...",
            config.host.c_str(),
            config.port
        );

        // Authentification réalisée par WiFiClientSecure
        // avec le certificat et la clé privée.
        // Aucun username/password MQTT nécessaire.
        if (client->connect(deviceId.c_str())) {
            LOG_INFO(
                MODULE_MQTT,
                "MQTT connected as %s",
                deviceId.c_str()
            );

            client->setCallback(mqttCallback);
            subscribeToTopics();

        } else {
            int state = client->state();

            LOG_ERROR(
                MODULE_MQTT,
                "MQTT connection failed, state: %d",
                state
            );
        }
    }
}


// ==========================================
// ÉTAT DE LA CONNEXION
// ==========================================

bool MQTTClient::isConnected() {
    return client && client->connected();
}


// ==========================================
// PUBLICATION MQTT
// Fonction conservée
// ==========================================

void MQTTClient::publish(
    const String& topic,
    const String& payload
) {
    if (client && client->connected() && topic.length() > 0) {

        String fullTopic;

        if (config.topic.length() > 0) {
            if (config.topic.endsWith("/")) {
                fullTopic = config.topic + topic;
            } else {
                fullTopic = config.topic + "/" + topic;
            }
        } else {
            fullTopic = topic;
        }

        bool result = client->publish(
            fullTopic.c_str(),
            payload.c_str()
        );

        if (!result) {
            LOG_WARN(
                MODULE_MQTT,
                "Failed to publish to %s",
                fullTopic.c_str()
            );
        }
    }
}


// ==========================================
// LECTURE CONFIGURATION
// ==========================================

MQTTConfig MQTTClient::getConfig() {
    return config;
}


// ==========================================
// MODIFICATION CONFIGURATION
// ==========================================

void MQTTClient::setConfig(
    const MQTTConfig& newConfig
) {
    config = newConfig;
    useSecure = true;

    if (client) {
        delete client;
        client = nullptr;
    }

    if (config.host.length() > 0) {

        // Reconfiguration TLS X.509
        wifiClientSecure.setCACert(AWS_ROOT_CA);
        wifiClientSecure.setCertificate(AWS_DEVICE_CERT);
        wifiClientSecure.setPrivateKey(AWS_PRIVATE_KEY);

        client = new PubSubClient(wifiClientSecure);

        client->setServer(
            config.host.c_str(),
            config.port
        );

        client->setBufferSize(4096);
    }
}


// ==========================================
// SAUVEGARDE CONFIGURATION
// ==========================================

void MQTTClient::saveConfig(Preferences& prefs) {
    prefs.putString("mqtt_host", config.host);
    prefs.putUShort("mqtt_port", config.port);
    prefs.putString("mqtt_topic", config.topic);
    prefs.putString("mqtt_username", config.username);
    prefs.putBool("mqtt_use_tls", config.useTLS);
}