# Soliseco – AWS IoT Test Report

## 1. Objective

Verify secure MQTT communication between the OpenWatt P1M5
project and the Soliseco AWS IoT Core platform.

The objective is to validate:

- Secure connection to AWS IoT Core.
- Authentication using official X.509 certificates.
- MQTT publication to the Soliseco STAGING environment.
- Confirmation of message publication by the MQTT broker.
- MQTT connection stability.
- Proper disconnection and reconnection behavior.

These initial tests were performed using Python on a Windows
computer, without physical ESP32 hardware.

---

## 2. Test Environment

| Parameter | Value |
|-----------|-------|
| Operating system | Windows |
| Test language | Python |
| MQTT library | Paho MQTT |
| Cloud platform | AWS IoT Core |
| AWS region | eu-central-1 |
| MQTT broker | a1rskt6dx2e2f2-ats.iot.eu-central-1.amazonaws.com |
| MQTT port | 8883 |
| MQTT protocol | MQTT 3.1.1 |
| MQTT QoS | 1 |
| Authentication | Mutual TLS (X.509) |
| Environment | STAGING |
| Test device | P1TEST001 |
| Test script | test_aws_mqtt_cert.py |

The tests used the official certificates provided
for the test device P1TEST001.

The certificates are stored outside the Git repository.

---

## 3. MQTT Topics

### 3.1 Meter Readings

The following topic is used to publish electricity
meter readings:

`P1M5-STAGING/P1TEST001/data/readings`

**Status: TESTED SUCCESSFULLY**

### 3.2 Device Configuration

The following topic is intended for device
configuration information:

`P1M5-STAGING/P1TEST001/sys/config`

**Status: NOT YET TESTED**

The final MQTT topic structure must still be
confirmed with the Soliseco backend team.

---

## 4. Tests Performed

### Test 1 – AWS IoT Connection

**Result: PASSED**

The Python MQTT client successfully connected to
AWS IoT Core using the official X.509 certificates
provided for P1TEST001.

Console output:

```text
Réponse MQTT : Success
Connexion MQTT réussie !
Device ID : P1TEST001
Certificat : P1TEST001
```

The connection was established using:

- MQTT over TLS.
- Port 8883.
- X.509 client certificate authentication.
- Client ID P1TEST001.

No MQTT username or password was required.

**Conclusion:**

The official device credentials successfully
authenticated the Python MQTT client to AWS IoT Core.

---

### Test 2 – MQTT Message Publication

**Result: PASSED**

A simulated JSON message was published to:

`P1M5-STAGING/P1TEST001/data/readings`

The message contained simulated electricity
measurements using OBIS field identifiers.

Console output:

```text
Envoi du message MQTT...
Topic : P1M5-STAGING/P1TEST001/data/readings
Code de publication local : 0

Confirmation MQTT reçue
Message ID : 1
Résultat : Success

Publication confirmée par le broker MQTT !
```

The MQTT publication used QoS 1.

The broker returned a successful PUBACK.

**Important:**

The published data was simulated.

It did not originate from a physical electricity
meter or ESP32 dongle.

**Conclusion:**

MQTT publication to the Soliseco STAGING readings
topic was successfully confirmed by AWS IoT Core.

This does not yet confirm that the Soliseco backend
stored or processed the message.

---

### Test 3 – Initial MQTT Connection Stability

**Result: REQUIRES INVESTIGATION**

During the initial publication tests, the Python
client experienced unexpected MQTT disconnections
after successful publication.

Console output:

```text
========== DÉCONNEXION MQTT ==========
Raison : Unspecified error
Code numérique : 128
Indicateurs : DisconnectFlags(is_disconnect_packet_from_server=False)
Informations complémentaires : []
ATTENTION : déconnexion inattendue.
======================================
```

The MQTT client automatically reconnected.

Initially, the test script published a new message
each time a connection was established.

This happened because the publication was triggered
inside the on_connect() callback.

As a result, each automatic reconnection caused
another MQTT publication.

#### Investigation

The following observations were made:

- AWS IoT accepted the MQTT connection.
- The MQTT broker confirmed the publication.
- Unexpected disconnections occurred afterward.
- The client automatically reconnected.
- The original script published again after reconnection.

The disconnection reason was reported as:

`Unspecified error (128)`

This error alone does not identify the exact cause.

**Conclusion:**

Unexpected disconnections were observed.

Their original cause has not been conclusively
identified.

---

### Test 4 – Prevention of Duplicate Publications

**Result: PASSED**

The Python script was modified to prevent repeated
publications after automatic MQTT reconnection.

A new variable was introduced:

```python
message_sent = False
```

The on_connect() callback was updated to publish
only when:

```python
if ENABLE_PUBLISH and not message_sent:
```

After a successful local publish request,
the variable is updated:

```python
if result.rc == mqtt.MQTT_ERR_SUCCESS:
    message_sent = True
```

This prevents another publication during the
same script execution.

The following message was observed after
automatic reconnection:

```text
Message déjà envoyé : aucune nouvelle publication.
```

**Important:**

The message_sent variable indicates that the
publication request was accepted locally by Paho.

Successful broker acknowledgement is verified
separately through the on_publish() callback.

**Conclusion:**

Repeated MQTT publications after reconnection
were prevented during the test.

---

### Test 5 – Connection Stability Without Publication

**Result: PASSED**

The test was executed with:

```python
ENABLE_PUBLISH = False
```

The Python client successfully connected to
AWS IoT Core without publishing a message.

The connection remained active during the
observation period.

No unexpected disconnection was observed.

The test was stopped manually by pressing Enter.

Console output:

```text
========== DÉCONNEXION MQTT ==========
Raison : Normal disconnection
Code numérique : 0
Indicateurs : DisconnectFlags(is_disconnect_packet_from_server=False)
Informations complémentaires : []
Déconnexion normale.
======================================

Test MQTT terminé.
```

**Conclusion:**

The MQTT connection remained stable without
publication during the observed test.

The manual shutdown produced a normal
MQTT disconnection.

---

### Test 6 – Python Process Cleanup

**Result: COMPLETED**

During the investigation, the Windows process
list showed two Python processes associated
with the MQTT test script.

The remaining test processes were stopped.

A subsequent process check confirmed that
no Python processes were still running.

This cleanup was performed before repeating
the MQTT publication test.

#### Possible explanation

Multiple MQTT connections using the same
client ID can cause connection conflicts.

The test device ID was:

`P1TEST001`

However, the two Windows process entries
did not conclusively establish that two
independent MQTT clients were connected.

The exact cause of the original disconnections
therefore remains unconfirmed.

**Conclusion:**

The previous Python test processes were
successfully stopped before the next test.

---

### Test 7 – MQTT Connection Stability After Cleanup

**Result: PASSED (SINGLE OBSERVATION)**

After stopping the remaining Python test
processes, a new MQTT publication test
was performed.

The script was configured with:

```python
ENABLE_PUBLISH = True
```

The test used:

- Official P1TEST001 certificates.
- MQTT over TLS.
- Port 8883.
- MQTT QoS 1.
- Soliseco STAGING readings topic.

#### Console output

```text
Réponse MQTT : Success
Connexion MQTT réussie !
Device ID : P1TEST001
Certificat : P1TEST001

Envoi du message MQTT...
Topic : P1M5-STAGING/P1TEST001/data/readings
Code de publication local : 0

Confirmation MQTT reçue
Message ID : 1
Résultat : Success

Publication confirmée par le broker MQTT !
```

No unexpected disconnection was observed
during the test.

The program was then stopped manually
by pressing Enter.

Final console output:

```text
========== DÉCONNEXION MQTT ==========
Raison : Normal disconnection
Code numérique : 0
Indicateurs : DisconnectFlags(is_disconnect_packet_from_server=False)
Informations complémentaires : []
Déconnexion normale.
======================================

Test MQTT terminé.
```

#### Results

- AWS IoT connection successful.
- One simulated JSON message published.
- MQTT PUBACK received successfully.
- No duplicate publication.
- No unexpected disconnection observed.
- Normal MQTT disconnection after manual shutdown.

**Conclusion:**

The previous unexpected disconnection issue
was not reproduced during this test.

The test was successful.

However, the original cause of the previous
disconnections has not been conclusively
identified.

Additional stability tests are recommended.

---

## 5. Results Summary

| Test | Result |
|------|--------|
| Official certificate files found | PASSED |
| TLS/MQTT connection | PASSED |
| MQTT publication to readings topic | PASSED |
| MQTT broker acknowledgement | PASSED |
| Prevention of duplicate publications | PASSED |
| Connection without publication | PASSED |
| Python process cleanup | COMPLETED |
| Connection stability after cleanup | PASSED (single observation) |
| Previous unexpected disconnections | CAUSE NOT CONFIRMED |
| Publication to sys/config topic | NOT TESTED |
| Backend database ingestion | NOT VERIFIED |
| Firmware running on ESP32 | NOT TESTED |
| Physical smart meter readings | NOT TESTED |
| End-to-end dongle-to-backend communication | NOT TESTED |

---

## 6. Security

The official P1TEST001 device credentials
are stored outside the Git repository.

The local certificate directory is:

`~/soliseco-device-certs/P1TEST001/`

The following files are required:

- Amazon Root CA.
- Device certificate.
- Device private key.

### Security Requirements

- Never commit private keys to Git.
- Never include private keys in documentation.
- Never expose certificate contents in screenshots.
- Never publish device credentials on GitHub.
- Restrict access to private keys.

The current firmware references certificate
constants defined in mqtt_certificates.h.

The intended production approach is to install
device credentials separately after provisioning,
following the method used for Metop devices.

This mechanism still needs to be implemented
and validated on the ESP32 firmware.

---

## 7. Remaining Tests

### 7.1 MQTT Stability

- Repeat MQTT publication tests.
- Verify connection stability over longer periods.
- Investigate the original disconnection cause
  if the issue returns.
- Check for concurrent clients using the same
  MQTT client ID.

### 7.2 AWS IoT and Soliseco Backend

- Confirm backend ingestion of published messages.
- Validate publication to the sys/config topic.
- Confirm the final MQTT topic structure.
- Verify the expected JSON field names and types.

### 7.3 Firmware Validation

- Compile the Soliseco firmware configuration.
- Confirm the final ESP32 hardware model.
- Implement secure certificate installation.
- Verify MQTT authentication directly from ESP32.
- Test firmware publication to AWS IoT Core.
- Verify device identification and topic generation.

### 7.4 Physical Hardware

- Connect the dongle to a compatible electricity meter.
- Read real P1 meter measurements.
- Publish real meter readings.
- Verify communication with the Soliseco backend.
- Validate long-term operation.

---

## 8. Conclusion

The initial Soliseco AWS IoT integration tests
were successful.

The official P1TEST001 certificates enabled
secure MQTT connection and successful publication
of simulated JSON readings to the Soliseco
STAGING environment.

During the initial tests, unexpected MQTT
disconnections occurred after publication.

The Python test script was modified to prevent
duplicate publications after reconnection.

The remaining Python test processes were then
stopped before performing another test.

The latest test produced the following results:

- MQTT connection established successfully.
- One simulated JSON message published.
- Successful MQTT PUBACK received.
- No unexpected disconnection observed.
- Normal disconnection after manual shutdown.

The previous disconnection issue was not
reproduced during the latest test.

However, its original cause has not been
conclusively identified.

Additional stability tests are recommended.

The complete communication chain from
physical ESP32 hardware to the Soliseco
backend remains to be tested.

---

## 9. Current Project Status

**AWS IoT connection from Python: VALIDATED**

**MQTT publication from Python: VALIDATED**

**MQTT stability: INITIAL SUCCESS, FURTHER TESTING REQUIRED**

**ESP32 firmware integration: NOT YET VALIDATED ON HARDWARE**

**Backend ingestion: NOT YET VERIFIED**

**Physical electricity meter integration: NOT YET TESTED**