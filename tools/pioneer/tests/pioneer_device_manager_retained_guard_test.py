#!/usr/bin/env python3
"""Check the actual device-manager retained-pin build guard without a kernel build."""
import os
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parents[3]
source = (root/'src/system/kernel/device_manager/device_manager.cpp').read_text()
start = source.index('#if defined(PIONEER_DEVICE_MANAGER_RETAINED_PIN_WITNESS)')
end = source.index('#ifdef PIONEER_DEVICE_MANAGER_PIN_WITNESS', start)
guard = source[start:end]
base = shlex.split(os.environ.get('CXX', 'c++')) + ['-x', 'c++', '-E']
retained = '-DPIONEER_DEVICE_MANAGER_RETAINED_PIN_WITNESS'
pin = '-DPIONEER_DEVICE_MANAGER_PIN_WITNESS'
compile_only = '-DPIONEER_DEVICE_MANAGER_RETAINED_PIN_COMPILE_ONLY'
for flags in ([retained], [retained, pin], [retained, compile_only]):
    result = subprocess.run(base + flags + ['-'], input=guard, text=True, capture_output=True)
    assert result.returncode != 0 and 'unexported compile witness' in result.stderr
for flags in ([], [pin], [retained, pin, compile_only]):
    result = subprocess.run(base + flags + ['-'], input=guard, text=True, capture_output=True)
    assert result.returncode == 0, result.stderr
print('PASS retained device-manager pin requires its unexported compile witness flags')
