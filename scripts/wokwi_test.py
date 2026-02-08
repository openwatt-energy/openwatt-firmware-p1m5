#!/usr/bin/env python3
"""
Run HTTP API tests against the Wokwi-simulated ESP32.
Start the simulator first (VS Code: F1 > Wokwi: Start Simulator), then:
  python3 scripts/wokwi_test.py

Uses net forward: host localhost:8080 -> ESP32:80 (set in wokwi.toml).
"""
import json
import os
import sys
import time
import urllib.request
import urllib.error

BASE = os.environ.get("WOKWI_HTTP", "http://localhost:8080")
TIMEOUT = 2
RETRIES = 30  # wait up to ~30s for sim to boot (use WOKWI_NO_WAIT=1 to fail immediately)


def get(path: str) -> dict:
  req = urllib.request.Request(f"{BASE}{path}")
  with urllib.request.urlopen(req, timeout=TIMEOUT) as r:
    return json.loads(r.read().decode())


def main() -> int:
  no_wait = os.environ.get("WOKWI_NO_WAIT", "").strip() in ("1", "true", "yes")
  retries = 1 if no_wait else RETRIES
  print(f"Testing API at {BASE} (set WOKWI_HTTP to override)")
  for i in range(retries):
    try:
      get("/api/system")
      break
    except (urllib.error.URLError, OSError) as e:
      if i == 0:
        print("Waiting for simulator to boot and serve HTTP...")
      time.sleep(1)
  else:
    print("ERROR: No response from simulator. Start Wokwi first (F1 > Wokwi: Start Simulator).")
    if no_wait:
      print("(Use WOKWI_NO_WAIT=0 or unset to wait up to 30s for boot)")
    return 1

  failed = 0

  # GET /api/system
  try:
    d = get("/api/system")
    assert "firmware_version" in d, d
    assert "device_id" in d, d
    print(f"  GET /api/system OK (firmware={d.get('firmware_version')}, device={d.get('device_id')})")
  except Exception as e:
    print(f"  GET /api/system FAIL: {e}")
    failed += 1

  # GET /api/state
  try:
    d = get("/api/state")
    assert "wifi_connected" in d, d
    assert "meter_connected" in d, d
    print(f"  GET /api/state OK (wifi={d.get('wifi_connected')}, meter={d.get('meter_connected')})")
  except Exception as e:
    print(f"  GET /api/state FAIL: {e}")
    failed += 1

  # GET /api/config
  try:
    d = get("/api/config")
    assert "wifi" in d, d
    assert "dongle_ip" in d or "email" in d, d
    print(f"  GET /api/config OK (dongle_ip={d.get('dongle_ip', 'N/A')})")
  except Exception as e:
    print(f"  GET /api/config FAIL: {e}")
    failed += 1

  # GET /api/meter (may be empty if no P1 data yet)
  try:
    d = get("/api/meter")
    assert "meter_connected" in d, d
    assert "valid" in d, d
    print(f"  GET /api/meter OK (meter_connected={d.get('meter_connected')}, valid={d.get('valid')})")
  except Exception as e:
    print(f"  GET /api/meter FAIL: {e}")
    failed += 1

  if failed:
    print(f"\n{failed} test(s) failed.")
    return 1
  print("\nAll API checks passed.")
  return 0


if __name__ == "__main__":
  sys.exit(main())
