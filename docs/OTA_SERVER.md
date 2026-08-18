# OTA Server

How over-the-air updates work for the P1M5 dongle, and what a server must do
to serve firmware to it.

## Default OTA server

The default firmware endpoint is:

```
https://api.openwatt.eu/ota
```

This is the value the firmware should use for `OTA_FIRMWARE_URL`
(`src/ota_client.cpp`). The dongle sends both its update checks and its
downloads to this single URL (no path or query is appended).

> ⚠️ The code currently points at a placeholder
> (`https://api.example.com/api/p1m5/firmware`). It must be switched to
> `https://api.openwatt.eu/ota` (see "Open items" below).

## How the client behaves

The client is `src/ota_client.cpp`, driven by `src/ota_update.cpp`. Checks
happen 30 seconds after boot, then every hour. Every request carries the
following headers:

| Header | Value | Meaning |
|--------|-------|---------|
| `User-Agent` | `Xenn-Update` | Fixed identifier |
| `Cache-Control` | `no-cache` | Never serve from cache |
| `xenn-target` | `esp32` | Hardware target |
| `xenn-version` | e.g. `v1.0.47-ow` | Version currently running |
| `xenn-serial` | e.g. `P1846680` | Device ID (from MAC) |
| `xenn-mac` | e.g. `D4:D4:DA:84:66:80` | Full MAC address |

### 1. Update check (`HEAD`)

The client sends `HEAD` to the firmware URL.

- **`200 OK`** → an update is available. The response may carry descriptive
  headers the client logs:
  - `Xenn-Firmware-Name`
  - `Xenn-Firmware-Tag`
  - `Xenn-Firmware-Date`
- **`304 Not Modified`** → the device is up to date.
- Anything else → treated as "check failed"; the client logs the HTTP code and
  backs off.

### 2. Download (`GET`)

If the check returned `200`, the client sends `GET` to the same URL.

- **`200 OK`** → the response body is the raw `.bin` firmware image. The client:
  1. reads `Content-Length` and verifies it fits in the OTA partition
     (`ESP.getFreeSketchSpace()`),
  2. streams the body directly into the OTA partition,
  3. finishes the update and reboots.
- Any other status → download aborted.

The server must serve the body as `application/octet-stream` with a correct
`Content-Length`. Range/redirect support is not required (redirects up to
301/302/307 are followed).

## What a server must do

A minimal OTA server for this firmware:

1. **Identify the device** from `xenn-serial` (primary) and `xenn-mac`
   (secondary).
2. **Decide the target version** for that device, typically from a database
   mapping `device_id → fw_version` (the current backend uses an
   `ota_firmwares` table keyed by `device_id` + `fw_type = 'p1m5'`).
3. **Compare** `xenn-version` against the target version.
4. **Respond to `HEAD`:**
   - `304` if the device is already on (or ahead of) the target version.
   - `200` otherwise, with `Xenn-Firmware-*` headers describing the target
     build.
5. **Respond to `GET`:** stream the matching binary (`200`, binary body,
   correct length) when the device is due an update.

Version comparison should be **numeric per segment**, not lexicographic
(`v1.0.9` < `v1.0.47` < `v1.1.0`).

## Firmware naming

Binaries are named `p1m5_<version>-<variant>.bin`:

- `<version>` — e.g. `v1.0.47`
- `<variant>` — `ow` (OpenWatt) or `soliseco` (SolisEco)

The `-<variant>` suffix is part of `xenn-version` (e.g. `v1.0.47-ow`), so the
server must match the variant as well as the version, or it may downgrade a
SolisEco device onto the OpenWatt build.

Example public URLs (current Supabase storage):

```
https://eohagurgmhwipdmqwcfw.supabase.co/storage/v1/object/firmwares/p1m5_v1.0.47-ow.bin
https://eohagurgmhwipdmqwcfw.supabase.co/storage/v1/object/firmwares/p1m5_v1.0.47-soliseco.bin
```

## Current architecture

Today the pieces are:

1. **Build + upload** — `scripts/build_variant.py` builds both variants;
   the release skill uploads them to a Supabase Storage `firmwares` bucket.
2. **Device → target mapping** — an `ota_firmwares` table (Supabase /
   PostgREST) records which version each dongle should run.
3. **Serving** — an HTTP endpoint that reads the table and the storage bucket
   to answer the `HEAD`/`GET` protocol above (this is what
   `api.openwatt.eu/ota` implements).

## Security

- Firmware is served over HTTPS. The dongle currently does **not** verify the
  server certificate (`setInsecure()` is only applied to MQTT, but no root CA
  is configured for the OTA HTTP client either) and does **not** verify a
  signature/checksum on the image before flashing.
- See `docs/CODE_REVIEW.md` (§3 Security) — OTA authenticity is an open item.

## Open items

- Change `OTA_FIRMWARE_URL` from `https://api.example.com/api/p1m5/firmware`
  to `https://api.openwatt.eu/ota`.
- Make the protocol more generic (server-driven) rather than tied to the
  `Xenn-*` header names, and consider serving firmware straight from GitHub
  Releases instead of Supabase / cloud storage.
