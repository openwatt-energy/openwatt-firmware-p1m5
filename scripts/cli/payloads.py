import sys

def build_payload(command: str, req_id: str, args) -> dict:
    """Construct the MQTT JSON payload based on the requested command."""
    if command == "sma_power":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "modbus_read",
            "req_id": req_id,
            "ip": args.target_ip,
            "port": 502,
            "unit_id": args.unit_id,
            "function": 3,
            "address": 30775,
            "count": 2
        }
    elif command == "sma_yield":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "modbus_read",
            "req_id": req_id,
            "ip": args.target_ip,
            "port": 502,
            "unit_id": args.unit_id,
            "function": 3,
            "address": 30513,
            "count": 4
        }
    elif command == "sma_throttle":
        if not args.target_ip: raise ValueError("--target-ip required")
        if args.limit is None:
            raise ValueError("--limit argument is required for sma_throttle")
        val_to_write = int(args.limit * 100)
        return {
            "action": "modbus_write",
            "req_id": req_id,
            "ip": args.target_ip,
            "port": 502,
            "unit_id": args.unit_id,
            "function": 6,
            "address": 40016,
            "count": 1,
            "write_values": [val_to_write]
        }
    elif command == "tesla_soe":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "http_request",
            "req_id": req_id,
            "method": "GET",
            "url": f"http://{args.target_ip}/api/system_status/soe",
            "headers": {"Accept": "application/json"},
            "timeout_ms": 5000
        }
    elif command == "tesla_meters":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "http_request",
            "req_id": req_id,
            "method": "GET",
            "url": f"http://{args.target_ip}/api/meters/aggregates",
            "headers": {"Accept": "application/json"},
            "timeout_ms": 5000
        }
    elif command == "tesla_wall_vitals":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "http_request",
            "req_id": req_id,
            "method": "GET",
            "url": f"http://{args.target_ip}/api/1/vitals",
            "headers": {"Accept": "application/json"},
            "timeout_ms": 5000
        }
    elif command == "tesla_wall_version":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "http_request",
            "req_id": req_id,
            "method": "GET",
            "url": f"http://{args.target_ip}/api/1/version",
            "headers": {"Accept": "application/json"},
            "timeout_ms": 5000
        }
    elif command == "tesla_wall_lifetime":
        if not args.target_ip: raise ValueError("--target-ip required")
        return {
            "action": "http_request",
            "req_id": req_id,
            "method": "GET",
            "url": f"http://{args.target_ip}/api/1/lifetime",
            "headers": {"Accept": "application/json"},
            "timeout_ms": 5000
        }
    elif command == "scan_ports":
        if not args.scan_subnet or not args.scan_ports:
            raise ValueError("--scan-subnet and --scan-ports are required for scan_ports")
        ports = [int(p.strip()) for p in args.scan_ports.split(",")]
        return {
            "action": "scan_ports",
            "req_id": req_id,
            "base_ip": args.scan_subnet,
            "start_ip": 1,
            "end_ip": 254,
            "ports": ports
        }
    else:
        raise ValueError(f"Unknown command: {command}")
