#!/usr/bin/env python3
import argparse
import hashlib
import base64
import json
import ssl
import time
import threading
import sys
import paho.mqtt.client as mqtt

def generate_password(device_id, secret_key):
    """Generate the MQTT password based on secret key and device ID."""
    combined = secret_key + device_id
    sha256 = hashlib.sha256(combined.encode()).digest()
    pwd_bytes = sha256[:10]
    return base64.b64encode(pwd_bytes).decode()

def decode_s32(words):
    """Decode an array of two 16-bit registers into a signed 32-bit integer."""
    if len(words) < 2: return None
    val = (words[0] << 16) | words[1]
    if val >= (1 << 31): val -= (1 << 32)
    # SMA often returns 0x80000000 for NaN/Invalid
    if val == -2147483648: return None
    return val

def decode_u64(words):
    """Decode an array of four 16-bit registers into an unsigned 64-bit integer."""
    if len(words) < 4: return None
    val = (words[0] << 48) | (words[1] << 32) | (words[2] << 16) | words[3]
    return val

class CLIClient:
    def __init__(self, args):
        self.args = args
        self.password = generate_password(args.device_id, args.secret_key)

        # Initialize MQTT Client
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.username_pw_set(args.device_id, self.password)
        if args.port in [8883, 443]:
            self.client.tls_set(cert_reqs=ssl.CERT_NONE)

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

        self.response_event = threading.Event()
        self.response_data = None
        self.commands_to_run = list(args.command)
        self.current_command = None
        self.req_id = None

    def run(self):
        print(f"Connecting to {self.args.broker}:{self.args.port} as {self.args.device_id}...")
        try:
            self.client.connect(self.args.broker, self.args.port, 60)
        except Exception as e:
            print(f"Failed to connect to MQTT broker: {e}")
            sys.exit(1)

        self.client.loop_start()

        # Wait for all commands to complete
        while self.commands_to_run:
            self.response_event.clear()
            self.current_command = self.commands_to_run.pop(0)
            self.req_id = f"cli-{self.current_command}-{int(time.time())}"
            self.send_command()

            if not self.response_event.wait(self.args.timeout):
                print(f"\nError: Timeout waiting for response to {self.current_command} after {self.args.timeout} seconds")
                break

        self.client.loop_stop()
        self.client.disconnect()

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            topic_resp = f"{self.args.prefix}/{self.args.device_id}/cmd_response"
            self.client.subscribe(topic_resp)
            # The run() loop handles sending commands now
        else:
            print(f"Connection failed with reason code: {reason_code}")
            sys.exit(1)

    def send_command(self):
        topic_cmd = f"{self.args.prefix}/{self.args.device_id}/cmd"
        payload = {}

        if self.current_command == "sma_power":
            payload = {
                "action": "modbus_read",
                "req_id": self.req_id,
                "ip": self.args.target_ip,
                "port": 502,
                "unit_id": self.args.unit_id,
                "function": 3,
                "address": 30775,
                "count": 2
            }
        elif self.current_command == "sma_yield":
            payload = {
                "action": "modbus_read",
                "req_id": self.req_id,
                "ip": self.args.target_ip,
                "port": 502,
                "unit_id": self.args.unit_id,
                "function": 3,
                "address": 30513,
                "count": 4
            }
        elif self.current_command == "tesla_soe":
            payload = {
                "action": "http_request",
                "req_id": self.req_id,
                "method": "GET",
                "url": f"http://{self.args.target_ip}/api/system_status/soe",
                "headers": {
                    "Accept": "application/json"
                },
                "timeout_ms": 5000
            }
        elif self.current_command == "tesla_meters":
            payload = {
                "action": "http_request",
                "req_id": self.req_id,
                "method": "GET",
                "url": f"http://{self.args.target_ip}/api/meters/aggregates",
                "headers": {
                    "Accept": "application/json"
                },
                "timeout_ms": 5000
            }
        elif self.current_command == "tesla_wall_vitals":
            payload = {
                "action": "http_request",
                "req_id": self.req_id,
                "method": "GET",
                "url": f"http://{self.args.target_ip}/api/1/vitals",
                "headers": {
                    "Accept": "application/json"
                },
                "timeout_ms": 5000
            }
        elif self.current_command == "tesla_wall_version":
            payload = {
                "action": "http_request",
                "req_id": self.req_id,
                "method": "GET",
                "url": f"http://{self.args.target_ip}/api/1/version",
                "headers": {
                    "Accept": "application/json"
                },
                "timeout_ms": 5000
            }
        elif self.current_command == "tesla_wall_lifetime":
            payload = {
                "action": "http_request",
                "req_id": self.req_id,
                "method": "GET",
                "url": f"http://{self.args.target_ip}/api/1/lifetime",
                "headers": {
                    "Accept": "application/json"
                },
                "timeout_ms": 5000
            }

        print(f"\nSending command '{self.current_command}' to {self.args.target_ip} via dongle {self.args.device_id}...")
        print(f"Topic: {topic_cmd}")
        self.client.publish(topic_cmd, json.dumps(payload))

    def on_message(self, client, userdata, msg):
        try:
            data = json.loads(msg.payload.decode())
            # Only process responses matching our request ID
            if data.get("req_id") == self.req_id:
                self.response_data = data
                print("\n[Raw JSON Response]")
                print(json.dumps(data, indent=2))

                # Check for successful modbus array
                if "data" in data and not data.get("error"):
                    words = data["data"]
                    if self.current_command == "sma_power":
                        power = decode_s32(words)
                        if power is not None:
                            print(f"\n=> Decoded Active Power: {power} W")
                        else:
                            print("\n=> Decoded Active Power: Invalid/NaN")

                    elif self.current_command == "sma_yield":
                        yield_wh = decode_u64(words)
                        print(f"\n=> Decoded Total Yield: {yield_wh} Wh")

                elif data.get("action") == "http_response":
                    if data.get("status_code") == 200:
                        try:
                            body_json = json.loads(data.get("body", "{}"))
                            if self.current_command == "tesla_soe":
                                soe = body_json.get("percentage")
                                print(f"\n=> Decoded State of Energy: {soe}%")
                            elif self.current_command == "tesla_meters":
                                solar = body_json.get("solar", {}).get("instant_power", 0)
                                site = body_json.get("site", {}).get("instant_power", 0)
                                load = body_json.get("load", {}).get("instant_power", 0)
                                battery = body_json.get("battery", {}).get("instant_power", 0)
                                print(f"\n=> Solar Power: {solar} W")
                                print(f"=> Grid Power: {site} W")
                                print(f"=> Load Power: {load} W")
                                print(f"=> Battery Power: {battery} W")
                            elif self.current_command == "tesla_wall_vitals":
                                grid_v = body_json.get("grid_v", 0)
                                vehicle_current_a = body_json.get("vehicle_current_a", 0)
                                state = body_json.get("evse_state", 0)
                                print(f"\n=> Grid Voltage: {grid_v} V")
                                print(f"=> Vehicle Current: {vehicle_current_a} A")
                                print(f"=> EVSE State: {state}")
                            elif self.current_command == "tesla_wall_version":
                                version = body_json.get("firmware_version", "unknown")
                                serial = body_json.get("serial_number", "unknown")
                                print(f"\n=> Firmware Version: {version}")
                                print(f"=> Serial Number: {serial}")
                            elif self.current_command == "tesla_wall_lifetime":
                                energy_wh = body_json.get("energy_wh", 0)
                                starts = body_json.get("charge_starts", 0)
                                uptime = body_json.get("uptime_s", 0)
                                print(f"\n=> Total Energy Dispensed: {energy_wh / 1000.0:.2f} kWh")
                                print(f"=> Total Charge Sessions: {starts}")
                                print(f"=> Uptime: {uptime} seconds")
                        except Exception as e:
                            print(f"\n=> Failed to parse HTTP JSON body: {e}")
                            print(f"Raw body: {data.get('body')}")
                    else:
                        print(f"\n=> HTTP Error: Status Code {data.get('status_code')}")

                elif data.get("error"):
                    print(f"\n=> Error: {data.get('error')} (Modbus Exception Code: {data.get('exception_code', 'N/A')})")

                self.response_event.set()

        except Exception as e:
            print(f"Failed to parse message payload: {e}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="OpenWatt Proxy MQTT CLI")
    parser.add_argument("--broker", default="mqtt.example.com", help="MQTT Broker URL")
    parser.add_argument("--port", type=int, default=8883, help="MQTT Broker Port")
    parser.add_argument("--prefix", default="P1M5", help="MQTT Topic Prefix")
    parser.add_argument("--device-id", required=True, help="Dongle Device ID (e.g. P1850D1C)")
    parser.add_argument("--secret-key", default="CHANGE_ME_SALT", help="Device Secret Key")
    parser.add_argument("--target-ip", required=True, help="IP address of the target device on local network")
    parser.add_argument("--command", choices=["sma_power", "sma_yield", "tesla_soe", "tesla_meters", "tesla_wall_vitals", "tesla_wall_version", "tesla_wall_lifetime"], nargs='+', default=["sma_power"], help="Proxy command(s) to execute")
    parser.add_argument("--unit-id", type=int, default=3, help="Modbus Unit ID (default: 3)")
    parser.add_argument("--timeout", type=int, default=15, help="Response timeout in seconds")

    args = parser.parse_args()

    cli = CLIClient(args)
    cli.run()
