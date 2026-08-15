# Security & Configuration

This firmware authenticates against your own infrastructure. Every deployment
**must** supply its own secret values — the repository ships with safe
placeholders and no real credentials.

## SALT

The SALT is a per-deployment secret used to derive device credentials:

| Credential | Derivation |
|------------|------------|
| WiFi AP password | `MD5(deviceId + SALT)` |
| MQTT broker password | `base64(SALT + deviceId)` |

Because the DeviceID is discoverable (it appears in the AP SSID
`OpenWatt-P1XXXXXX`), **anyone with the SALT can derive the credentials of every
device on your network.** Treat the SALT as a production secret.

Set it at build time — never hardcode it in source:

```ini
; platformio.ini
build_flags =
  -DSALT_STRING='"your-salt-here"'
```

The matching value must be supplied to the password calculator:

```bash
python3 calculate_password.py P1A1B2C3 --salt "your-salt-here"
# or
OPENWATT_SALT="your-salt-here" python3 calculate_password.py P1A1B2C3
```

## MQTT broker

Set the broker host at build time:

```ini
build_flags =
  -DMQTT_BROKER_HOST='"broker.example.com"'
```

Provision the broker with the corresponding username/password scheme (see the
MQTT server documentation).

## OTA

The over-the-air update endpoint is configured per deployment. See the OTA
service documentation for setup instructions.

## Reporting a vulnerability

Please report security issues privately to the maintainers rather than opening
a public issue.
