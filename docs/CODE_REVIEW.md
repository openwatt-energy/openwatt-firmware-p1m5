# Code Review — OpenWatt P1M5 Firmware

Quick evaluation of the `v1.0.47` firmware codebase (≈4,700 LOC of C++).

Scope: `src/` on `main`. Ordering reflects priorities: code quality first,
then efficiency, then security.

---

## 1. Code quality

### 1.1 Duplication

- **OTA download functions are ~95% identical.** `OTAClient::downloadAndApply`
  (`src/ota_client.cpp:83-197`) and `OTAClient::downloadAndApplyFromURL`
  (`:199-291`) differ only in the URL source and a few headers. Collapse into
  one function that takes a URL and an optional header set.

- **OBIS field mapping duplicated.** `publishToMQTT` (`src/main.cpp:122-331`)
  and `broadcastToWebSocket` (`:642-695`) both build JSON keyed by DSMR OBIS
  codes (`1-0:1.8.1`, `1-0:2.8.1`, ...). Extract a single
  `serializeObisFields(JsonObject&, const P1Data&)` helper so the two stay in
  sync.

- **NVS read pattern repeated.** `getNvsString` (`web_api.cpp:21`), the
  `nvs_dump` handler (`main.cpp:528-574`), and `loadCredentials`
  (`wifi_manager.cpp:151-227`) each hand-roll `nvs_open` → `nvs_get_str` →
  `malloc` → `nvs_close`. Consolidate into one utility.

- **AP setup copy-pasted.** The "compute AP SSID from MAC + `softAP` + static
  IP" block appears four times: `wifi_manager.cpp:36-52`, `:66-73`, `:303-314`,
  and `:337-350`. Extract `startSoftAP()`.

### 1.2 Bugs / correctness smells

- **Operator precedence bug.** `mqtt_client.cpp:120`:
  ```cpp
  if (!client || !config.host.length() > 0) { return; }
  ```
  `!config.host.length() > 0` parses as `(!length) > 0`. It only works by
  accident for the empty-host case. Intent is clearly `config.host.length() == 0`.

- **Fake/placeholder data published as real.** `main.cpp:152-155` hardcodes
  switch positions to `"0"`, `:159-160` sets a `99.999 kW` limiter, and
  `:263-264` sets a `999.99 A` fuse limit on every publish. Downstream
  consumers can't tell these are synthetic.

- **Stale `static` progress state.** `ota_client.cpp:159` (and `:258`) use a
  `static int lastPercent` inside a non-static member function, shared between
  the two duplicate functions. A second update in the same boot can mis-log
  progress. Should be a local/instance variable.

- **Device ID collision risk.** `getDeviceId()` (`main.cpp:65-73`) derives the
  ID from only the last 3 MAC bytes (`P1%02X%02X%02X` = 24 bits). Collisions
  are plausible across a fleet; the same ID is used as the MQTT username.

- **Contradictory MQTT configuration.** `PATCH /api/config/mqtt`
  (`web_api.cpp:349-376`) calls `setConfig`/`saveConfig`, but
  `MQTTClient::begin()` (`mqtt_client.cpp:22-33`) force-overrides host/port/
  topic from compile-time defines on every boot. User-set MQTT settings are
  silently discarded after reboot.

- **Confusing/duplicated identity state.** `state.serialNumber` and
  `getDeviceName()` (`main.cpp:75-83`) compute the same value; `getDeviceId()`
  re-reads the MAC from eFuse in several places instead of caching once.

### 1.3 Structure / maintainability

- **Two parallel variant systems.** `variant_config.h` + `build_variant.py`
  (generates the header) coexist with `firmware_variants.json` and
  `customer_config.py`. The relationship and which is authoritative is unclear.

- **God file.** `main.cpp` is 984 lines mixing setup, MQTT publishing, raw
  debug, WebSocket, command handling, and LED/state glue. Split into modules
  (e.g. `mqtt_publisher`, `command_handler`, `device_state`).

- **Magic strings everywhere.** OBIS codes and NVS key names are string
  literals scattered across files, not shared constants. Typo-prone and hard
  to grep (e.g. `"montly_power_peak"` is deliberately misspelled for
  compatibility but easy to break).

- **Inconsistent error handling.** Some `nvs_*` results are checked, others
  not; `malloc` results are checked in `web_api.cpp` but not uniformly
  elsewhere. Standardize.

- **Doc drift.** `README.md` and `DEV.md` still reference "Serial2 GPIO 16/17";
  the code uses Serial1 GPIO 21/22 (`src/p1_reader.cpp:11-16`). Fix the docs
  or the code.

### 1.4 Testability

- The P1 parser and MQTT auth logic have unit tests, but they run **on real
  hardware** (`test/test_main.cpp` targets the ESP32). There is no native/CI
  test target, so `pio test` cannot run headlessly (see `platformio.ini`
  `[env:test]`, which requires `upload_port`). Prefer a `native` environment
  with mocks for `Arduino.h`/`String`.

---

## 2. Efficiency (ESP32 RAM/CPU constrained)

- **Dynamic JSON churn.** `publishToMQTT` allocates a fresh `JsonDocument` and
  builds ~210 lines of JSON every 5 s; `broadcastToWebSocket` does the same per
  telegram. Prefer static, fixed-capacity `JsonDocument` and
  `serializeJson(doc, buffer)` to avoid heap fragmentation over long uptimes.

- **Heavy `String` usage** in the hot path — `p1_reader.cpp` uses `String`
  ~79× during telegram parsing. Prefer `char[]`/`memcpy` for the parser and
  pass buffers by reference.

- **Redundant work per publish.** `main.cpp` rebuilds the full OBIS mapping
  and re-generates a UUID on every publish; static placeholder fields could be
  hoisted.

- **Blocking `delay()` in connection paths.** `wifi_manager.cpp:291` (`delay(backoff)`
  up to 3 s) and the bootloader/factory-reset handlers (`web_api.cpp:472,494,536`)
  block the cooperative main loop.

---

## 3. Security

### Critical

- **MQTT TLS not validated.** `wifiClientSecure.setInsecure()` in
  `mqtt_client.cpp:46` and `:198` disables certificate verification → MITM of
  meter data and the DLMS key (sent via `set_creos_key`).

- **No HTTP API authentication.** All endpoints are open, and the fallback AP
  is passwordless (`wifi_manager.cpp:46,67,307,344`). Any network peer can:
  - `PATCH /api/system/ota-pull?url=...` → flash arbitrary firmware
    (`web_api.cpp:498`)
  - `PATCH /api/system/factory_reset` / `/reboot` / `/bootloader`
  - `PATCH /api/config/wifi`

- **OTA lacks integrity verification.** `ota_client.cpp` flashes whatever it
  downloads with no signature/checksum, and no root CA is configured → the
  update channel is unauthenticated.

- **Hardcoded shared secret.** `main.cpp:834` uses `SALT_STRING`
  (`config.h:30` = `"CHANGE_ME_SALT"`) to derive MQTT passwords; committed to a
  public repo.

- **Weak password derivation.** `mqtt_auth.cpp:19-24` base64-encodes only the
  first 10 bytes (80 bits) of SHA256(secret + deviceId).

- **Sensitive data exposed.** `GET /api/config` returns `creos_key`
  (`web_api.cpp:94`); the `nvs_dump` MQTT command (`main.cpp:528-574`) and the
  serial boot log (`main.cpp:782-784`) dump the Creos key / WiFi SSID.

### Moderate

- `generateUUID()` (`main.cpp:98-120`) uses `random(256)` without entropy
  seeding → weak/predictable UUIDs.
- Placeholder hosts `mqtt.example.com` / `api.example.com` shipped in
  `config.h` and `ota_client.cpp`.

---

## Suggested order of work

1. **Code quality:** fix the `mqtt_client.cpp:120` precedence bug; deduplicate
   the OTA and OBIS-mapping functions; extract AP setup and NVS helpers.
2. **Efficiency:** static `JsonDocument` + buffer-based serialization on the
   publish/WebSocket hot path.
3. **Security:** add API auth; enable TLS cert validation (pin the broker CA);
   sign/verify OTA images; move the secret out of `config.h`.
