#!/usr/bin/env python3
import argparse
import json
import ssl
import time
import threading
import sys
import paho.mqtt.client as mqtt

from cli.auth import generate_password
from cli.payloads import build_payload
from cli.parsers import parse_response

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
        else:
            print(f"Connection failed with reason code: {reason_code}")
            sys.exit(1)

    def send_command(self):
        topic_cmd = f"{self.args.prefix}/{self.args.device_id}/cmd"

        try:
            payload = build_payload(self.current_command, self.req_id, self.args)
        except ValueError as e:
            print(f"\nError building payload: {e}")
            sys.exit(1)

        target = self.args.scan_subnet if self.current_command == "scan_ports" else self.args.target_ip
        print(f"\nSending command '{self.current_command}' to {target} via dongle {self.args.device_id}...")
        print(f"Topic: {topic_cmd}")
        self.client.publish(topic_cmd, json.dumps(payload))

    def on_message(self, client, userdata, msg):
        try:
            data = json.loads(msg.payload.decode())
            # Only process responses matching our request ID
            if data.get("req_id") == self.req_id:
                parse_response(self.current_command, data, self.args)
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

    # Generic target options
    parser.add_argument("--target-ip", help="IP address of the target device on local network")
    parser.add_argument("--unit-id", type=int, default=3, help="Modbus Unit ID (default: 3)")
    parser.add_argument("--limit", type=float, help="Power limit percentage for throttling (0-100)")

    # Scanner options
    parser.add_argument("--scan-subnet", help="Base IP to scan (e.g., '192.168.1.')")
    parser.add_argument("--scan-ports", help="Comma-separated list of ports to scan (e.g., '80,502')")

    parser.add_argument("--timeout", type=int, default=45, help="Response timeout in seconds")

    commands = [
        "sma_power", "sma_yield", "sma_throttle",
        "tesla_soe", "tesla_meters", "tesla_wall_vitals", "tesla_wall_version", "tesla_wall_lifetime",
        "scan_ports"
    ]
    parser.add_argument("--command", choices=commands, nargs='+', required=True, help="Proxy command(s) to execute")

    args = parser.parse_args()

    cli = CLIClient(args)
    cli.run()
