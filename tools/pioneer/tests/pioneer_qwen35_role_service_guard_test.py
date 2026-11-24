#!/usr/bin/env python3
"""Check that the service qualification tag cannot escape its guarded build."""
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
    PIONEER_DRIVER_BUILD_TAG='2026112300030012', PIONEER_Q38_SPAN_PRELOAD='1',
    PIONEER_Q38_SPAN_FAST='1', PIONEER_QWEN35_RECOVERY_COMPILE_ONLY='1',
    PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY='1', PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY='1',
    PIONEER_QWEN35_SHARED_ROLE_OWNER_COMPILE_ONLY='1', PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY='1',
    PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_COMPILE_ONLY='1', PIONEER_QWEN35_RECOVERY_QUALIFY='1',
    PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY='1', PIONEER_QWEN35_ROLE_STAGE_QUALIFY='1',
    PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY='1', PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY='1',
    PIONEER_QWEN35_ROLE_SERVICE_QUALIFY='1')
cases = [
    {'PIONEER_QWEN35_ROLE_SERVICE_QUALIFY': '0'},
    {'PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY': '0'},
    {'PIONEER_QWEN35_ROLE_STAGE_QUALIFY': '0'},
    {'PIONEER_DRIVER_BUILD_TAG': '2026112300030011'},
    {'PIONEER_QWEN35_RECOVERY_QUALIFY': '0'},
    {'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL': '1'},
]
with tempfile.TemporaryDirectory(prefix='q35-service-guard-') as temp:
    for number, change in enumerate(cases):
        output = Path(temp)/str(number)
        result = subprocess.run(['sh', str(script), sys.argv[1], str(output)],
            env=dict(base, **change), stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, timeout=25)
        assert result.returncode != 0 and not output.exists(), change
    fault = dict(base, PIONEER_DRIVER_BUILD_TAG='2026112300030013', PIONEER_QWEN35_ROLE_FAULT_QUALIFY='1')
    fault_cases = [
        {'PIONEER_QWEN35_ROLE_FAULT_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_SERVICE_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_STAGE_QUALIFY': '0'},
        {'PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY': '0'},
        {'PIONEER_QWEN35_RECOVERY_QUALIFY': '0'},
        {'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL': '1'},
        {'PIONEER_DRIVER_BUILD_TAG': '2026112300030012'},
        {'PIONEER_DRIVER_BUILD_TAG': '2026112300030014'},
    ]
    for number, change in enumerate(fault_cases):
        output = Path(temp)/('fault-'+str(number))
        result = subprocess.run(['sh', str(script), sys.argv[1], str(output)],
            env=dict(fault, **change), stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, timeout=25)
        assert result.returncode != 0 and not output.exists(), change
    fleet = dict(base, PIONEER_DRIVER_BUILD_TAG='2026112300030019',
        PIONEER_QWEN35_ROLE_FLEET_SERVICE_QUALIFY='1')
    fleet_cases = [
        {'PIONEER_QWEN35_ROLE_FLEET_SERVICE_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_SERVICE_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_STAGE_QUALIFY': '0'},
        {'PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY': '0'},
        {'PIONEER_QWEN35_RECOVERY_QUALIFY': '0'},
        {'PIONEER_QWEN35_ROLE_FAULT_QUALIFY': '1'},
        {'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL': '1'},
        {'PIONEER_DRIVER_BUILD_TAG': '2026112300030012'},
        {'PIONEER_DRIVER_BUILD_TAG': '2026112300030018'},
    ]
    for number, change in enumerate(fleet_cases):
        output = Path(temp)/('fleet-'+str(number))
        result = subprocess.run(['sh', str(script), sys.argv[1], str(output)],
            env=dict(fleet, **change), stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, timeout=25)
        assert result.returncode != 0 and not output.exists(), change
print('PASS service, fault and fleet tags reject missing or conflicting qualification guards')
