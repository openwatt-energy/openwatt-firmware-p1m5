import argparse
import asyncio
import json
import os
import sys
import threading
import urllib.parse
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path

import websockets

sys.path.append(os.path.join(os.path.dirname(__file__), '../scripts'))
import build_templates

parser = argparse.ArgumentParser(description="Run the UI Emulator")
parser.add_argument(
    '--variant',
    choices=['openwatt', 'soliseco', 'creos'],
    default='openwatt',
    help='Customer variant to emulate'
)
args = parser.parse_args()

SCRIPT_DIR = Path(__file__).parent
DATA_DIR = SCRIPT_DIR.parent / 'src' / 'templates'

config_path = SCRIPT_DIR.parent / f"src/customers/{args.variant}.json"
if os.path.exists(config_path):
    with open(config_path, 'r') as f:
        customer_config = json.load(f)
else:
    customer_config = {}

CUSTOMER_NAME = customer_config.get('display_name', 'OpenWatt')
MQTT_ENABLED = customer_config.get('features', {}).get('mqtt_settings_ui', True)
theme = customer_config.get('theme', {})
THEME_CSS = (
    f"<style>:root{{--primary:{theme.get('primary', '#2563eb')};"
    f"--bg:{theme.get('background', '#ffffff')};"
    f"--text:{theme.get('text', '#111827')};"
    f"--accent:{theme.get('accent', '#1d4ed8')};}}</style>"
)

STATUS_DATA = {
    "wifi_connected": True,
    "meter_connected": True,
    "cloud_connected": True,
    "ap_mode": False,
}
SYSTEM_DATA = {"firmware_version": "v1.0.x-mock", "device_id": "P1812345"}
CONFIG_DATA = {
    "mqtt_host": "mqtt.example.com",
    "customer_fingerprint": customer_config.get('fingerprint_default', ""),
}
WIFI_SCAN_DATA = {
    "networks": [
        {"ssid": "HomeNetwork", "rssi": -60, "secure": 1},
        {"ssid": "GuestWiFi", "rssi": -75, "secure": 1},
        {"ssid": "OpenCafe", "rssi": -85, "secure": 0},
    ]
}
LIVE_DATA = {
    "0-0:96.1.1": "3132333435363738",
    "0-0:96.1.4": "MockMeterV1",
    "1-0:94.32.1": "v2.0",
    "1-0:1.7.0": 1.250,
    "1-0:2.7.0": 0.000,
    "1-0:1.8.1": 1500.5,
    "1-0:1.8.2": 2000.3,
    "1-0:2.8.1": 500.1,
    "1-0:2.8.2": 100.5,
    "1-0:32.7.0": 230.5,
    "1-0:52.7.0": 230.1,
    "1-0:72.7.0": 230.8,
    "1-0:31.7.0": 5.4,
    "1-0:51.7.0": 0.0,
    "1-0:71.7.0": 0.0,
}


class EmulatorHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DATA_DIR), **kwargs)

    def do_GET(self):
        parsed_url = urllib.parse.urlparse(self.path)
        path = parsed_url.path

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
            elif path in ("/api/wifiscan", "/api/wifi/scan"):
                self.wfile.write(json.dumps(WIFI_SCAN_DATA).encode())
            else:
                self.wfile.write(b"{}")
            return

        if path == "/style.css":
            css_path = DATA_DIR / 'tailwind.min.css'
            if os.path.exists(css_path):
                self.send_response(200)
                self.send_header("Content-Type", "text/css")
                self.end_headers()
                with open(css_path, 'rb') as f:
                    self.wfile.write(f.read())
            else:
                self.send_response(404)
                self.end_headers()
            return

        if (path == "/" or path == "/index.html") and STATUS_DATA.get("ap_mode"):
            self.send_response(302)
            self.send_header('Location', '/settings')
            self.end_headers()
            return

        template_map = {
            "/": "dashboard.html",
            "/index.html": "dashboard.html",
            "/settings": "settings.html",
            "/settings.html": "settings.html",
            "/system": "system.html",
            "/system.html": "system.html",
        }

        if path in template_map:
            filepath = DATA_DIR / template_map[path]
            if os.path.exists(filepath):
                with open(filepath, 'r') as f:
                    content = f.read()

                processed_content = build_templates.process_template(content, MQTT_ENABLED)
                processed_content = processed_content.replace('{{CUSTOMER_NAME}}', CUSTOMER_NAME)
                processed_content = processed_content.replace('{{THEME_CSS}}', THEME_CSS)
                processed_content = processed_content.replace('{{LOGO}}', '<!-- Mock SVG Logo -->')
                processed_content = processed_content.replace('{{DASHBOARD_JS}}', '// Mock Dashboard JS')
                processed_content = processed_content.replace('{{SETTINGS_JS}}', '// Mock Settings JS')
                processed_content = processed_content.replace('{{SYSTEM_JS}}', '// Mock System JS')

                self.send_response(200)
                self.send_header('Content-type', 'text/html')
                self.end_headers()
                self.wfile.write(processed_content.encode('utf-8'))
                return

        return super().do_GET()

    def do_PATCH(self):
        parsed_path = urllib.parse.urlparse(self.path)
        if parsed_path.path == '/api/config/fingerprint':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(
                json.dumps({"status": "success", "message": "Key updated"}).encode('utf-8')
            )
            return
        self.send_error(404, "Not Found")


async def mock_ws_handler(websocket):
    print("WebSocket client connected")
    try:
        while True:
            await websocket.send(json.dumps(LIVE_DATA))
            LIVE_DATA["1-0:1.7.0"] = max(
                0, LIVE_DATA["1-0:1.7.0"] + (id(websocket) % 3 - 1) * 0.05
            )
            LIVE_DATA["1-0:32.7.0"] = max(
                220, min(240, LIVE_DATA["1-0:32.7.0"] + (id(websocket) % 3 - 1) * 0.2)
            )
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


if __name__ == '__main__':
    port = 3030
    ws_thread = threading.Thread(target=start_ws_server, daemon=True)
    ws_thread.start()
    print(f"Starting emulator on http://localhost:{port} with variant: '{args.variant}'")
    httpd = HTTPServer(('', port), EmulatorHandler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
