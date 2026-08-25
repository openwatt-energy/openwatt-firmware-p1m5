#!/usr/bin/env python3
"""
Post-build script to rename firmware.bin to p1m5_v<VERSION>-<VARIANT>.bin

Registered as a PlatformIO extra_script (env.AddPostAction). The variant is
derived from the build environment name (openwatt -> ow, soliseco -> soliseco,
creos -> creos).
"""

import shutil
from pathlib import Path

VARIANT_BY_ENV = {"openwatt": "ow", "soliseco": "soliseco", "creos": "creos"}


def _read_define(path, name):
    try:
        for line in path.read_text().splitlines():
            if name in line and '"' in line:
                parts = line.split('"')
                if len(parts) >= 2:
                    return parts[1]
    except OSError:
        pass
    return None


def rename_firmware(target, source, env):
    src_file = Path(str(target[0]))
    if not src_file.exists():
        return

    build_dir = src_file.parent
    project_dir = Path(env.get("PROJECT_DIR", "."))

    version = (_read_define(project_dir / "src" / "config.h", "FIRMWARE_VERSION_BASE") or "0.0.0").lstrip("v")
    pio_env = env.get("PIOENV", "openwatt")
    variant = VARIANT_BY_ENV.get(pio_env, pio_env)

    new_name = f"p1m5_v{version}-{variant}.bin"

    shutil.copy2(src_file, build_dir / new_name)
    shutil.copy2(src_file, project_dir / new_name)
    print(f"✓ Created {new_name}")


try:
    Import("env")
    env.AddPostAction("$BUILD_DIR/firmware.bin", rename_firmware)
    print("✓ Post-build rename script registered")
except Exception:
    pass
