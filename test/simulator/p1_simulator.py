#!/usr/bin/env python3
"""
P1 Telegram Simulator for OpenWatt P1 Reader
Generates valid DSMR telegrams and sends them via serial port.
"""

import argparse
import serial
import time
import random
from typing import Optional


def calculate_crc16(data: bytes) -> int:
  """Calculate CRC16 checksum for P1 telegram."""
  crc = 0
  for byte in data:
    crc ^= (byte << 8)
    for _ in range(8):
      if crc & 0x8000:
        crc = (crc << 1) ^ 0x1021
      else:
        crc <<= 1
    crc &= 0xFFFF
  return crc


def generate_telegram(
    equipment_id: str = "4530303531303035333030303035333137",
    consumption_t1: float = 1234.567,
    consumption_t2: float = 5678.901,
    production_t1: float = 0.0,
    production_t2: float = 0.0,
    power_consumed: float = 0.123,
    power_produced: float = 0.0,
    tariff: int = 1,
    add_crc: bool = True
) -> str:
  """Generate a valid DSMR P1 telegram."""
  
  # Build telegram
  lines = [
    f"/XMX5LGBBFG1009325509",
    "",
    "1-0:0.2.8(50)",
    f"0-0:1.0.0({time.strftime('%y%m%d%H%M%S')}W)",
    f"0-0:96.1.1({equipment_id})",
    f"1-0:1.8.1({consumption_t1:09.3f}*kWh)",
    f"1-0:1.8.2({consumption_t2:09.3f}*kWh)",
    f"1-0:2.8.1({production_t1:09.3f}*kWh)",
    f"1-0:2.8.2({production_t2:09.3f}*kWh)",
    f"0-0:96.14.0({tariff:04d})",
    f"1-0:1.7.0({power_consumed:06.3f}*kW)",
    f"1-0:2.7.0({power_produced:06.3f}*kW)",
    "0-0:96.7.21(00015)",
    "0-0:96.7.9(00001)",
    "1-0:99.97.0(0)(0-0:96.7.19)",
    "1-0:32.32.0(00000)",
    "1-0:32.36.0(00000)",
    "0-0:96.13.1()",
    "0-0:96.13.0()",
    f"1-0:1.4.0({power_consumed:06.3f}*kW)",
    f"1-0:1.6.0({power_consumed:06.3f}*kW)({time.strftime('%y%m%d%H%M%S')}W)",
    f"0-0:98.1.0({power_consumed * 2:06.3f}*kW)",
  ]
  
  telegram = "\r\n".join(lines)
  
  # Add CRC
  if add_crc:
    telegram_with_crc_marker = telegram + "\r\n!"
    crc = calculate_crc16(telegram_with_crc_marker.encode('ascii'))
    telegram = telegram_with_crc_marker + f"{crc:04X}\r\n"
  else:
    telegram = telegram + "\r\n"
  
  return telegram


def send_telegram(port: serial.Serial, telegram: str, delay: float = 0.1):
  """Send telegram character by character to simulate real meter."""
  for char in telegram:
    port.write(char.encode('ascii'))
    time.sleep(delay)
  port.flush()


def simulate_meter(
    port: str,
    baudrate: int = 115200,
    interval: float = 10.0,
    count: Optional[int] = None,
    vary_values: bool = True
):
  """Simulate a P1 meter sending telegrams periodically."""
  
  ser = serial.Serial(port, baudrate, timeout=1)
  print(f"Connected to {port} at {baudrate} baud")
  print(f"Sending telegrams every {interval} seconds")
  if count:
    print(f"Will send {count} telegrams")
  print("Press Ctrl+C to stop\n")
  
  try:
    sent = 0
    base_consumption = 1234.567
    base_production = 0.0
    
    while True:
      # Vary values slightly if requested
      if vary_values:
        consumption_t1 = base_consumption + random.uniform(-0.1, 0.1)
        consumption_t2 = base_consumption * 4.5 + random.uniform(-0.5, 0.5)
        power_consumed = random.uniform(0.05, 0.5)
        power_produced = random.uniform(0.0, 0.2) if random.random() > 0.7 else 0.0
        tariff = random.choice([1, 2])
      else:
        consumption_t1 = base_consumption
        consumption_t2 = base_consumption * 4.5
        power_consumed = 0.123
        power_produced = 0.0
        tariff = 1
      
      telegram = generate_telegram(
        consumption_t1=consumption_t1,
        consumption_t2=consumption_t2,
        production_t1=base_production,
        production_t2=base_production,
        power_consumed=power_consumed,
        power_produced=power_produced,
        tariff=tariff
      )
      
      print(f"[{time.strftime('%H:%M:%S')}] Sending telegram #{sent + 1}...")
      send_telegram(ser, telegram, delay=0.01)  # Fast transmission
      sent += 1
      
      if count and sent >= count:
        break
      
      time.sleep(interval)
      
  except KeyboardInterrupt:
    print("\n\nStopped by user")
  finally:
    ser.close()
    print(f"\nSent {sent} telegrams total")


def send_test_cases(port: str, baudrate: int = 115200):
  """Send various test cases for validation."""
  
  ser = serial.Serial(port, baudrate, timeout=1)
  print(f"Connected to {port} at {baudrate} baud")
  print("Sending test cases...\n")
  
  test_cases = [
    ("Valid telegram", generate_telegram()),
    ("Telegram with production", generate_telegram(
      production_t1=1111.222,
      power_produced=0.5
    )),
    ("Telegram without CRC", generate_telegram(add_crc=False)),
    ("High consumption", generate_telegram(
      consumption_t1=99999.999,
      power_consumed=9.999
    )),
  ]
  
  for name, telegram in test_cases:
    print(f"Sending: {name}")
    send_telegram(ser, telegram, delay=0.05)
    time.sleep(2)
  
  ser.close()
  print("\nTest cases sent")


def main():
  parser = argparse.ArgumentParser(
    description="P1 Telegram Simulator for OpenWatt P1 Reader"
  )
  parser.add_argument(
    "--port", "-p",
    required=True,
    help="Serial port (e.g., /dev/ttyUSB0 or COM3)"
  )
  parser.add_argument(
    "--baudrate", "-b",
    type=int,
    default=115200,
    help="Baud rate (default: 115200)"
  )
  parser.add_argument(
    "--interval", "-i",
    type=float,
    default=10.0,
    help="Interval between telegrams in seconds (default: 10.0)"
  )
  parser.add_argument(
    "--count", "-c",
    type=int,
    default=None,
    help="Number of telegrams to send (default: infinite)"
  )
  parser.add_argument(
    "--no-vary",
    action="store_true",
    help="Don't vary values between telegrams"
  )
  parser.add_argument(
    "--test-cases",
    action="store_true",
    help="Send predefined test cases instead of continuous simulation"
  )
  
  args = parser.parse_args()
  
  if args.test_cases:
    send_test_cases(args.port, args.baudrate)
  else:
    simulate_meter(
      args.port,
      args.baudrate,
      args.interval,
      args.count,
      vary_values=not args.no_vary
    )


if __name__ == "__main__":
  main()
