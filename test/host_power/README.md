# P1 power, voltage and CRC regression tests

Run on the PC without a dongle or serial connection:

```text
python test/host_power/run.py --compiler PATH_TO_ZIG_EXE --build-dir PATH_TO_SCRATCH_DIRECTORY
```

The runner compiles the production parser body from src/p1_reader.cpp and
P1Data merge/update methods. A minimal Arduino String shim replaces Arduino
string operations and logging. UART and network integration are not simulated.

Coverage includes import/export transitions through zero, absent and malformed
power fields, missing/malformed voltages, genuine zero voltage, voltage recovery,
CRC rejection and a captured simulator telegram with its transmitted CRC.
Validation on 2026-09-30: 74 checks passed; PlatformIO soliseco-c3 build succeeded.

Behavior and deployment limitations:
- Present, valid zero power replaces a previous nonzero total. Missing/malformed
  power totals still retain their last known values; no expiry is implemented.
- Missing/malformed voltages are null in API, MQTT and WebSocket output. A real
  zero remains numeric zero. The local dashboard displays Unavailable for null.
- DSMR CRC uses reflected polynomial 0xA001 with initial value zero, covering
  slash through exclamation mark inclusive. Invalid CRC telegrams are rejected.
- The voltage/CRC changes have not yet been flashed or validated on hardware.
  Check downstream null handling and compatibility of other meter formats,
  including encrypted DLMS, before deployment. A rejected telegram does not
  clear previously accepted measurements or establish their freshness.
- These fixes do not establish the cause of historical field incidents.

CRC reference: DSMR 5.0.2, section 6.2:
https://www.netbeheernederland.nl/sites/default/files/2024-02/dsmr_5.0.2_p1_companion_standard.pdf
