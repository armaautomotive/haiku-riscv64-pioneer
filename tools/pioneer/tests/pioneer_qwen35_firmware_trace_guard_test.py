#!/usr/bin/env python3
"""Check the trace tag cannot build outside its restricted reset profile."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: test.py HAIKU_BUILD')
root = Path(__file__).resolve().parents[3]
script = root/'tools/pioneer/build_qwen38_banks_driver.sh'
base = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
base.update(PIONEER_Q38_PANEL_DRIVER='15', PIONEER_DRIVER_RELOAD='1',
    PIONEER_DRIVER_BUILD_TAG='2026112400030022', PIONEER_Q38_SPAN_PRELOAD='1',
    PIONEER_Q38_SPAN_FAST='1', PIONEER_QWEN35_RECOVERY_COMPILE_ONLY='1',
    PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY='1', PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY='1',
    PIONEER_QWEN35_SHARED_ROLE_OWNER_COMPILE_ONLY='1', PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY='1',
    PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_COMPILE_ONLY='1', PIONEER_QWEN35_RECOVERY_QUALIFY='1',
    PIONEER_QWEN35_FIRMWARE_RESET_QUALIFY='1')
cases = [
    {'PIONEER_QWEN35_FIRMWARE_RESET_QUALIFY': '0'},
    {'PIONEER_QWEN35_RECOVERY_QUALIFY': '0'},
    {'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY': '0'},
    {'PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY': '0'},
    {'PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY': '0'},
    {'PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_COMPILE_ONLY': '0'},
    {'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL': '1'},
    {'PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY': '1'},
    {'PIONEER_DRIVER_BUILD_TAG': '2026112400030023'},
]
with tempfile.TemporaryDirectory(prefix='q35-firmware-trace-guard-') as temp:
    for number, change in enumerate(cases):
        output=Path(temp)/str(number)
        result=subprocess.run(['sh',str(script),sys.argv[1],str(output)],env=dict(base,**change),
                              stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=25)
        assert result.returncode != 0 and not output.exists(),change
print('PASS firmware trace tag rejects missing or conflicting recovery guards')
