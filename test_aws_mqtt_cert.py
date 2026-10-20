import os
import ssl
import json
import time
import paho.mqtt.client as mqtt
import glob

# ==========================================
# CONFIGURATION AWS IOT CORE
# ==========================================

HOST = "a1rskt6dx2e2f2-ats.iot.eu-central-1.amazonaws.com"
PORT = 8883

# Identifiant MQTT demandé par Soliseco
DEVICE_ID = "P1TEST001"

# Topic officiel STAGING
TOPIC = f"P1M5-STAGING/{DEVICE_ID}/data/readings"

# False = tester uniquement la connexion
# True = envoyer un message MQTT
ENABLE_PUBLISH = False
# Empêcher plusieurs publications après reconnexion
message_sent = False


# ==========================================
# CHEMINS DES CERTIFICATS OFFICIELS SOLISECO
# ==========================================

CERT_ID = "P1TEST001"

CERT_DIR = os.path.join(
    os.path.expanduser("~"),
    "soliseco-device-certs",
    "P1TEST001"
)

CA_FILE = os.path.join(
    CERT_DIR,
    "AmazonRootCA1.pem"
)

CERT_FILE = glob.glob(
    os.path.join(CERT_DIR, "*-certificate.pem.crt")
)[0]

KEY_FILE = glob.glob(
    os.path.join(CERT_DIR, "*-private.pem.key")
)[0]

# ==========================================
# VÉRIFICATION DES CERTIFICATS
# ==========================================

print("\nVérification des certificats...")

for file_path in [CA_FILE, CERT_FILE, KEY_FILE]:

    if not os.path.isfile(file_path):

        raise FileNotFoundError(
            f"Certificat ou clé introuvable : {file_path}"
        )

print("Certificats trouvés !")
print("Certificat utilisé :", CERT_ID)


# ==========================================
# MESSAGE MQTT DE TEST
# ==========================================

# ==========================================
# MESSAGE MQTT - SIMULATION DES MESURES P1
# ==========================================

message = {
    "timestamp": 120,
    "0-0:1.0.0": "261016123500",
    "0-0:96.1.1": "COMPTEUR_TEST",
    "0-0:96.1.2": "",
    "0-0:96.1.4": "",
    "0-0:96.13.0": "",
    "0-0:96.14.0": "1",
    "0-0:96.3.10": "1",

    "1-0:1.7.0": {
        "value": 1.25,
        "unit": "kW"
    },

    "1-0:1.8.1": {
        "value": 1250.5,
        "unit": "kWh"
    },

    "1-0:1.8.2": {
        "value": 830.2,
        "unit": "kWh"
    },

    "1-0:2.7.0": {
        "value": 0.0,
        "unit": "kW"
    },

    "1-0:31.7.0": {
        "value": 5.4,
        "unit": "A"
    },

    "1-0:32.7.0": {
        "value": 230.5,
        "unit": "V"
    },

    "id": "12345678-1234-4234-8234-123456789ABC",
    "dongle_id": DEVICE_ID,
    "sent_timestamp": 120
}

payload = json.dumps(message)


# ==========================================
# CALLBACK : CONNEXION
# ==========================================

def on_connect(client, userdata, flags, reason_code, properties):
    global message_sent

    print("\nRéponse MQTT :", reason_code)

    if reason_code == 0:

        print("Connexion MQTT réussie !")
        print("Device ID :", DEVICE_ID)
        print("Certificat :", CERT_ID)

        if ENABLE_PUBLISH and not message_sent:

            print("\nEnvoi du message MQTT...")

            result = client.publish(
                TOPIC,
                payload,
                qos=1
            )

            print("Topic :", TOPIC)
            print("Code de publication local :", result.rc)

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                message_sent = True

        elif message_sent:

            print("Message déjà envoyé : aucune nouvelle publication.")

        else:

            print("Publication désactivée (ENABLE_PUBLISH = False)")

    else:

        print("Connexion MQTT refusée :", reason_code)


# ==========================================
# CALLBACK : PUBLICATION
# ==========================================

def on_publish(
    client,
    userdata,
    mid,
    reason_code,
    properties
):

    print("\nConfirmation MQTT reçue")
    print("Message ID :", mid)
    print("Résultat :", reason_code)

    if reason_code == 0:

        print("Publication confirmée par le broker MQTT !")

    else:

        print("Publication non confirmée :", reason_code)


# ==========================================
# CALLBACK : DÉCONNEXION
# ==========================================

def on_disconnect(
    client,
    userdata,
    disconnect_flags,
    reason_code,
    properties
):

    print("\n========== DÉCONNEXION MQTT ==========")
    print("Raison :", reason_code)
    print("Code numérique :", getattr(reason_code, "value", "inconnu"))
    print("Indicateurs :", disconnect_flags)
    print("Informations complémentaires :", properties)

    if reason_code != 0:
        print("ATTENTION : déconnexion inattendue.")
    else:
        print("Déconnexion normale.")

    print("======================================")


# ==========================================
# CRÉATION DU CLIENT MQTT
# ==========================================

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id=DEVICE_ID,
    protocol=mqtt.MQTTv311
)


# ==========================================
# AUTHENTIFICATION X.509
# ==========================================

client.tls_set(
    ca_certs=CA_FILE,
    certfile=CERT_FILE,
    keyfile=KEY_FILE,
    cert_reqs=ssl.CERT_REQUIRED,
    tls_version=ssl.PROTOCOL_TLS_CLIENT
)

client.tls_insecure_set(False)


# ==========================================
# CALLBACKS
# ==========================================

client.on_connect = on_connect
client.on_publish = on_publish
client.on_disconnect = on_disconnect


# ==========================================
# CONNEXION AWS IOT CORE
# ==========================================

print("\n===================================")
print("TEST MQTT AWS IOT CORE - SOLISECO")
print("===================================")

print("Serveur :", HOST)
print("Port :", PORT)
print("Device ID :", DEVICE_ID)
print("Certificat :", CERT_ID)
print("Topic :", TOPIC)
print("Publication activée :", ENABLE_PUBLISH)


try:

    print("\nConnexion à AWS IoT Core...")

    client.connect(
        HOST,
        PORT,
        keepalive=60
    )

    client.loop_start()

    input("\nAppuie sur Entrée pour arrêter le test...\n")

except KeyboardInterrupt:

    print("\nArrêt demandé.")

except Exception as e:

    print("\nErreur MQTT :", repr(e))

finally:

    client.disconnect()
    client.loop_stop()

    print("\nTest MQTT terminé.")