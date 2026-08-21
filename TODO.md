# TODO

Open items for the OpenWatt P1M5 firmware. Details and file/line references
are in [docs/CODE_REVIEW.md](docs/CODE_REVIEW.md) and
[docs/PRE_RELEASE_REVIEW.md](docs/PRE_RELEASE_REVIEW.md).

## Security

- [ ] **HTTP API authentication** — every `/api/*` endpoint (including
      `/api/system/ota-pull`, `/api/config`, `/api/system/factory_reset`) is
      unauthenticated, and the fallback AP is open. Add auth before broad
      distribution.
- [ ] **MQTT TLS verification** — currently `MQTT_TLS_VERIFY 0`
      (`src/config.h`) because the broker is self-signed. Move the broker to a
      public CA cert, then set `MQTT_TLS_VERIFY 1`.
- [ ] **OTA TLS verification** — the OTA HTTP client does not validate the
      server certificate.
- [ ] **OTA image signing** — downloaded firmware is flashed with no
      signature/checksum verification.
- [ ] **Real build-time secrets** — `SALT_STRING` and `OTA_FIRMWARE_URL` are
      still placeholders (`CHANGE_ME_SALT`, `api.example.com`). Inject real
      values at build time and never commit them. (`MQTT_BROKER_HOST` now points
      at `mqtt.openwatt.eu`.)

## Code quality

- [ ] Remove fake/placeholder published data (`99.999` kW limiter, `999.99` A
      fuse, `"0"` switch positions) in `src/main.cpp`.
- [ ] Fix device-ID collision risk (`P1` + 3 MAC bytes = 24 bits), also used as
      the MQTT username.
- [ ] Reconcile MQTT config — `MQTTClient::begin()` overwrites user-set MQTT
      settings on reboot.
- [ ] Deduplicate the OBIS field mapping (`publishToMQTT` vs
      `broadcastToWebSocket` in `src/main.cpp`).
- [ ] Split the ~1000-line `src/main.cpp` into modules.
- [ ] Add a native (headless) unit-test target — `pio test` currently needs
      real hardware.

## Release / infra

- [ ] Serve firmware from GitHub Releases instead of Supabase storage
      (see [docs/OTA_SERVER.md](docs/OTA_SERVER.md)).
- [ ] Update the OTA server to a generic, server-driven protocol (decouple from
      the `Xenn-*` header names).
