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
        self.req_id = f"cli-{int(time.time())}"

    def run(self):
        print(f"Connecting to {self.args.broker}:{self.args.port} as {self.args.device_id}...")
        try:
            self.client.connect(self.args.broker, self.args.port, 60)
        except Exception as e:
            print(f"Failed to connect to MQTT broker: {e}")
            sys.exit(1)

        self.client.loop_start()

        # Wait for the response event to be set by the on_message callback
        if self.response_event.wait(self.args.timeout):
            self.client.loop_stop()
            self.client.disconnect()
        else:
            print(f"\nError: Timeout waiting for response after {self.args.timeout} seconds")
            self.client.loop_stop()
            self.client.disconnect()
            sys.exit(1)

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            topic_resp = f"{self.args.prefix}/{self.args.device_id}/cmd_response"
            self.client.subscribe(topic_resp)
            self.send_command()
        else:
            print(f"Connection failed with reason code: {reason_code}")
            sys.exit(1)

    def send_command(self):
        topic_cmd = f"{self.args.prefix}/{self.args.device_id}/cmd"
        payload = {}

        if self.args.command == "sma_power":
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
        elif self.args.command == "sma_yield":
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

        print(f"Sending command '{self.args.command}' to {self.args.target_ip} via dongle {self.args.device_id}...")
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
                    if self.args.command == "sma_power":
                        power = decode_s32(words)
                        if power is not None:
                            print(f"\n=> Decoded Active Power: {power} W")
                        else:
                            print("\n=> Decoded Active Power: Invalid/NaN")

                    elif self.args.command == "sma_yield":
                        yield_wh = decode_u64(words)
                        print(f"\n=> Decoded Total Yield: {yield_wh} Wh")

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
    parser.add_argument("--command", choices=["sma_power", "sma_yield"], default="sma_power", help="Proxy command to execute")
    parser.add_argument("--unit-id", type=int, default=3, help="Modbus Unit ID (default: 3)")
    parser.add_argument("--timeout", type=int, default=15, help="Response timeout in seconds")

    args = parser.parse_args()

    cli = CLIClient(args)
    cli.run()
