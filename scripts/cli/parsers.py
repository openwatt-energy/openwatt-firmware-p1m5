import json
from .modbus import decode_s32, decode_u64

def parse_response(command: str, data: dict, args):
    """Parse and print the dongle's response based on the original command."""

    print("\n[Raw JSON Response]")
    print(json.dumps(data, indent=2))

    if data.get("error"):
        print(f"\n=> Error: {data.get('error')} (Exception Code: {data.get('exception_code', 'N/A')})")
        return

    # Handle Modbus Responses
    if data.get("action") == "modbus_response":
        if data.get("success"):
            if command == "sma_throttle":
                print(f"\n=> Successfully throttled inverter to {args.limit}%")
        elif "data" in data:
            words = data["data"]
            if command == "sma_power":
                power = decode_s32(words)
                if power is not None: print(f"\n=> Decoded Active Power: {power} W")
                else: print("\n=> Decoded Active Power: Invalid/NaN")
            elif command == "sma_yield":
                yield_wh = decode_u64(words)
                print(f"\n=> Decoded Total Yield: {yield_wh} Wh")

    # Handle HTTP Responses
    elif data.get("action") == "http_response":
        status = data.get("status_code")
        if status == 200:
            try:
                body_json = json.loads(data.get("body", "{}"))
                if command == "tesla_soe":
                    soe = body_json.get("percentage")
                    print(f"\n=> Decoded State of Energy: {soe}%")
                elif command == "tesla_meters":
                    solar = body_json.get("solar", {}).get("instant_power", 0)
                    site = body_json.get("site", {}).get("instant_power", 0)
                    load = body_json.get("load", {}).get("instant_power", 0)
                    battery = body_json.get("battery", {}).get("instant_power", 0)
                    print(f"\n=> Solar Power: {solar} W")
                    print(f"=> Grid Power: {site} W")
                    print(f"=> Load Power: {load} W")
                    print(f"=> Battery Power: {battery} W")
                elif command == "tesla_wall_vitals":
                    grid_v = body_json.get("grid_v", 0)
                    vehicle_current_a = body_json.get("vehicle_current_a", 0)
                    state = body_json.get("evse_state", 0)
                    print(f"\n=> Grid Voltage: {grid_v} V")
                    print(f"=> Vehicle Current: {vehicle_current_a} A")
                    print(f"=> EVSE State: {state}")
                elif command == "tesla_wall_version":
                    version = body_json.get("firmware_version", "unknown")
                    serial = body_json.get("serial_number", "unknown")
                    print(f"\n=> Firmware Version: {version}")
                    print(f"=> Serial Number: {serial}")
                elif command == "tesla_wall_lifetime":
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
            print(f"\n=> HTTP Error: Status Code {status}")

    # Handle Scanner Responses
    elif data.get("action") == "scan_result":
        found = data.get("found", [])
        print(f"\n=> Scan Results for subnet {args.scan_subnet}x:")
        if not found:
            print("  No devices found on the requested ports.")
        for dev in found:
            print(f"  - {dev['ip']}: Port {dev['port']} OPEN")

