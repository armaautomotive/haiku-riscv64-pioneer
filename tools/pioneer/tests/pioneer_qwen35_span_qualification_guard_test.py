#!/usr/bin/env python3
"""A loadable span qualification cannot escape its fixed restricted route."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: test.py HAIKU_BUILD')
root = Path(__file__).resolve().parents[3]
script = root/'tools/pioneer/build_qwen38_banks_driver.sh'
env = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
env.update(PIONEER_Q38_PANEL_DRIVER='15', PIONEER_DRIVER_RELOAD='1',
    PIONEER_DRIVER_BUILD_TAG='2026112300030017', PIONEER_Q38_SPAN_PRELOAD='1',
    PIONEER_Q38_SPAN_FAST='1', PIONEER_QWEN35_RECOVERY_COMPILE_ONLY='1',
    PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY='1', PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY='1',
    PIONEER_QWEN35_SHARED_ROLE_OWNER_COMPILE_ONLY='1', PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY='1',
    PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_COMPILE_ONLY='1', PIONEER_QWEN35_RECOVERY_QUALIFY='1',
    PIONEER_QWEN35_SUPPORTED_SPAN_COMPILE_ONLY='1', PIONEER_QWEN35_SPAN_QUALIFY='1')
cases = [
    {'PIONEER_QWEN35_SPAN_QUALIFY':'0'},
    {'PIONEER_QWEN35_SUPPORTED_SPAN_COMPILE_ONLY':'0'},
    {'PIONEER_QWEN35_RECOVERY_QUALIFY':'0'},
    {'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY':'0'},
    {'PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY':'0'},
    {'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL':'1'},
    {'PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY':'1'},
    {'PIONEER_QWEN35_FIRMWARE_RESET_QUALIFY':'1'},
    {'PIONEER_Q38_SPAN_FAST':'0'},
    {'PIONEER_DRIVER_BUILD_TAG':'2026112300030013'},
]
with tempfile.TemporaryDirectory(prefix='q35-span-guard-') as temp:
    for i, change in enumerate(cases):
        output=Path(temp)/str(i)
        result=subprocess.run(['sh', str(script), sys.argv[1], str(output)],
            env=dict(env, **change), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=25)
        assert result.returncode != 0 and not output.exists(), change
print('PASS supported span qualification rejects missing or conflicting build guards')
