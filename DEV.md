# Development & testing order

Work in this order so you only change one thing at a time.

## 1. Stable boot (current)

- **P1 is OFF** (`ENABLE_P1_READER 0` in `src/config.h`) so the board does not touch Serial1 and cannot WDT.
- Build, flash, open serial @ 115200. You should see:
  - `*** Starting OpenWatt application ***`
  - `Web UI: http://192.168.4.1`
  - `P1 UART disabled (ENABLE_P1_READER=0)`
  - No reset.

If it resets, the image is wrong or something else is wrong; fix that before step 2.

## 2. WiFi save

- Connect to AP `OpenWatt-P1xxxx`, open http://192.168.4.1 → Settings.
- Enter SSID + password → Save. Board restarts.
- On serial after reboot you want: `Connecting to: <your SSID>` then `WiFi connected: <IP>`.
- If you still see `No saved WiFi credentials`, the NVS write path is still failing; that’s the only thing to debug in this step.

## 3. Turn P1 on

- In `src/config.h` set `ENABLE_P1_READER 1`.
- Clean build and flash:
  - `python3 -m platformio run -t clean -e openwatt`
  - `python3 -m platformio run -e openwatt`
  - `./build-and-flash.sh` (or your usual flash command)
- Serial should show:
  - `Creating P1 reader task (Serial1 init deferred)...`
  - `[P1 Task] Starting (waiting for Serial1 init from main loop)...`
  - After ~5 s: `[P1] Serial1 init from main loop...` → `[P1] Serial1 ready.` → `P1 reader task started`
  - No WDT reset.

If it WDTs again, the next move is to init Serial1 without `setRxBufferSize` (use default RX buffer) or to read P1 in `loop()` only (no task).

## 4. Meter test

- Connect smart meter P1 to GPIO 21/22 (RX/TX).
- Check `/api/meter` and WebSocket `/api/live` for data when the meter sends telegrams.

---

**Efficient workflow**

- One feature at a time; confirm with serial log before the next step.
- After any change: clean build + flash, then test. If it breaks, the last change is the cause.
- Reference: `src/p1_reader.h` and `src/web_api.cpp` for the P1 data format and API endpoints.

---

## If WiFi still doesn’t persist (NVS diagnostics)

After Save, check **serial**: you should see erase err (only if not NOT_FOUND), blob write, same-handle read ok=1/0, commit err. Then reboot and see if “No saved WiFi credentials” or “Connecting to: …”.

**1. Probe NVS from the device**

- After saving WiFi, before reboot, open: `http://192.168.4.1/api/debug/nvs`
- Response: `nvs_open`, `wifi_ssid_blob_err` (0 = OK), `wifi_ssid_blob_len`, `wifi_ssid_str_err`, `wifi_ssid_str_len`
- If blob_err is 0 and blob_len > 0, the key is stored as blob; if after reboot loadCredentials still fails, the problem is on **read path** (e.g. different partition/namespace at boot). If blob_err != 0 right after save, the **write** path failed (commit, partition full, etc.).

**2. Try another namespace**

- In `wifi_manager.cpp` change `NVS_NAMESPACE` to e.g. `"wifi_cfg"` for both save and load (and in main.cpp don’t create keys in that namespace). If it starts persisting, something in `"openwatt"` (init block or Preferences use) is conflicting.

**3. Explicit partition**

- Use `nvs_open_from_partition("nvs", NVS_NAMESPACE, NVS_READWRITE, &h)` everywhere we open NVS for WiFi (save + load + getSavedSSID). Ensures we’re not hitting a different partition.

**4. Full flash erase**

- `esptool.py erase_flash` then reflash app. Rules out corrupted or leftover NVS from another firmware (e.g. ESPHome).

**5. Minimal repro**

- Separate sketch: open `"openwatt"`, `nvs_set_blob(h, "wifi_ssid", "Test", 5)`, commit, same-handle get_blob, Serial.println result. Run on same board to confirm NVS works at all.

---

## If “Scan WiFi” shows no networks

- **Serial**: When you press “Scan WiFi”, serial should log e.g. `WiFi scan: raw_count=N (mode=…)` and up to 5 SSIDs. If **raw_count is 0 or negative**, the scan itself returned nothing or failed (e.g. scan in progress -1, scan failed -2). If **raw_count > 0** but UI shows “No networks”, the bug is in response/UI (we already log “returned M networks in JSON”).
- **Typical causes for raw_count 0**: (1) Device in AP-only mode briefly and scan not started on STA, (2) radio busy or timing – try adding a short delay before `WiFi.scanNetworks` or call scan from a timer after AP is up, (3) antenna / environment (weak or no other APs in range; your own AP may still show if STA can scan while AP is up).
- **Quick test**: From another device connected to the same OpenWatt AP, open `http://192.168.4.1/api/config/wifiscan` and check JSON: `raw_count` and `networks[]`. Compare with serial log.
