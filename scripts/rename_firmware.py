#!/usr/bin/env python3
"""
Post-build script to rename firmware with variant extension
Renames firmware.bin to p1m5_v<VERSION>-<FW_EXT>.bin
"""

import os
import sys
from pathlib import Path

# Get build environment info
from platformio.project.config import ProjectConfig
from platformio.project.helpers import get_project_dir

def rename_firmware(source, target, env):
    """Rename firmware.bin to variant-specific filename"""
    
    # Get project directory
    project_dir = Path(env["PROJECT_DIR"])
    
    # Read variant config to get FW_EXT
    variant_config_path = project_dir / "src" / "variant_config.h"
    fw_ext = "ow"  # Default
    
    if variant_config_path.exists():
        with open(variant_config_path, 'r') as f:
            for line in f:
                if 'FW_EXT' in line and '"' in line:
                    # Extract FW_EXT value from #define FW_EXT "xxx"
                    parts = line.split('"')
                    if len(parts) >= 2:
                        fw_ext = parts[1]
                        break
    
    # Get version from config.h
    config_path = project_dir / "src" / "config.h"
    version = "1.0.25"  # Default
    
    if config_path.exists():
        with open(config_path, 'r') as f:
            for line in f:
                if 'FIRMWARE_VERSION_BASE' in line and '"' in line:
                    parts = line.split('"')
                    if len(parts) >= 2:
                        version = parts[1].replace('v', '')
                        break
    
    # Build new filename
    new_filename = f"p1m5_v{version}-{fw_ext}.bin"
    
    # Get source and target paths
    build_dir = Path(env["BUILD_DIR"])
    source_file = build_dir / "firmware.bin"
    target_file = build_dir / new_filename
    
    # Also copy to project root for convenience
    root_target = project_dir / new_filename
    
    if source_file.exists():
        # Copy with new name in build dir
        import shutil
        shutil.copy2(source_file, target_file)
        print(f"✓ Created: {target_file}")
        
        # Copy to project root
        shutil.copy2(source_file, root_target)
        print(f"✓ Created: {root_target}")
        
        # Also create a generic name symlink/copy
        generic_name = project_dir / "firmware-latest.bin"
        if generic_name.exists():
            generic_name.unlink()
        shutil.copy2(source_file, generic_name)
        print(f"✓ Created: {generic_name}")

# Main function for PlatformIO
if __name__ == "__main__":
    # When called from PlatformIO
    if len(sys.argv) > 1:
        # Parse arguments passed by PlatformIO
        import subprocess
        result = subprocess.run(
            [sys.executable, __file__],
            capture_output=True,
            text=True
        )
        print(result.stdout)
        if result.stderr:
            print(result.stderr, file=sys.stderr)
    else:
        # Standalone execution
        print("This script should be run by PlatformIO after build")
        sys.exit(1)
else:
    # PlatformIO import
    try:
        Import("env")
        env.AddPostAction("$BUILD_DIR/firmware.bin", rename_firmware)
        print("✓ Post-build rename script registered")
    except ImportError:
        pass
