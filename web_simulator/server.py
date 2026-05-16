import os
import json
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse
from pathlib import Path
import threading
import time

# Get the absolute path to the data directory
SCRIPT_DIR = Path(__file__).parent
DATA_DIR = SCRIPT_DIR.parent / 'data'

# Mock data
STATUS_DATA = {
    "wifi_connected": True,
    "meter_connected": True,
    "cloud_connected": True
}

SYSTEM_DATA = {
    "firmware_version": "v1.0.45",
    "device_id": "P1812345"
}

CONFIG_DATA = {
    "mqtt_host": "mqtt.example.com",
    "mqtt_port": 8883,
    "customer_fingerprint": "mock_fingerprint"
}

LIVE_DATA = {
    "0-0:96.1.1": "3132333435363738", # meter ID
    "0-0:96.1.4": "MockMeterV1",
    "1-0:94.32.1": "v2.0",
    "1-0:1.7.0": 1.250, # Import power
    "1-0:2.7.0": 0.000, # Export power
    "1-0:1.8.1": 1500.5, # Import energy T1
    "1-0:1.8.2": 2000.3, # Import energy T2
    "1-0:2.8.1": 500.1,  # Export energy T1
    "1-0:2.8.2": 100.5,  # Export energy T2
    "1-0:32.7.0": 230.5, # L1 Voltage
    "1-0:52.7.0": 230.1, # L2 Voltage
    "1-0:72.7.0": 230.8, # L3 Voltage
    "1-0:31.7.0": 5.4,   # L1 Current
    "1-0:51.7.0": 0.0,   # L2 Current
    "1-0:71.7.0": 0.0,   # L3 Current
}

WIFI_SCAN_DATA = [
    {"ssid": "HomeNetwork", "rssi": -60, "secure": 1},
    {"ssid": "GuestWiFi", "rssi": -75, "secure": 1},
    {"ssid": "OpenCafe", "rssi": -85, "secure": 0}
]

class MockDongleHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DATA_DIR), **kwargs)

    def do_GET(self):
        parsed_url = urlparse(self.path)
        path = parsed_url.path

        # Handle API routes
        if path.startswith("/api/"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()

            if path == "/api/state":
                self.wfile.write(json.dumps(STATUS_DATA).encode())
            elif path == "/api/system":
                self.wfile.write(json.dumps(SYSTEM_DATA).encode())
            elif path == "/api/config":
                self.wfile.write(json.dumps(CONFIG_DATA).encode())
            elif path == "/api/live":
                self.wfile.write(json.dumps(LIVE_DATA).encode())
            elif path == "/api/wifi/scan":
                self.wfile.write(json.dumps(WIFI_SCAN_DATA).encode())
            else:
                self.wfile.write(b"{}")
            return

        # Map HTML routes to static files
        if path == "/":
            self.path = "/index.html"
        elif path == "/settings":
            self.path = "/settings.html"
        elif path == "/system":
            self.path = "/system.html"
        elif path == "/live":
            self.path = "/live.html"

        return super().do_GET()

import asyncio
import websockets
import json
import threading

async def mock_ws_handler(websocket):
    print("WebSocket client connected")
    try:
        while True:
            # Send live data every second
            await websocket.send(json.dumps(LIVE_DATA))

            # Tiny random variations for realism
            LIVE_DATA["1-0:1.7.0"] = max(0, LIVE_DATA["1-0:1.7.0"] + (id(websocket) % 3 - 1) * 0.05)
            LIVE_DATA["1-0:32.7.0"] = max(220, min(240, LIVE_DATA["1-0:32.7.0"] + (id(websocket) % 3 - 1) * 0.2))

            await asyncio.sleep(1)
    except websockets.exceptions.ConnectionClosed:
        print("WebSocket client disconnected")

def start_ws_server():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    start_server = websockets.serve(mock_ws_handler, "0.0.0.0", 8081)
    loop.run_until_complete(start_server)
    print("Starting WebSocket simulator on ws://localhost:8081")
    loop.run_forever()

if __name__ == "__main__":
    port = 3030

    # Start WS server in background thread
    ws_thread = threading.Thread(target=start_ws_server, daemon=True)
    ws_thread.start()

    print(f"Starting dongle HTTP simulator on http://localhost:{port}")
    server = HTTPServer(("0.0.0.0", port), MockDongleHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()
