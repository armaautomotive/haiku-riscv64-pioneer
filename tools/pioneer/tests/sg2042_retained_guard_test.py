#!/usr/bin/env python3
"""Ensure the retained ECAM lease cannot enter a loadable configuration."""
import os
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parents[3]
compiler = shlex.split(os.environ.get('CXX', 'c++'))
source = '#include "src/add-ons/kernel/busses/pci/ecam/Sg2042RecoveryLease.h"\n'
base = compiler + ['-std=c++17', '-x', 'c++', '-fsyntax-only', '-I', str(root)]
retained = '-DPIONEER_SG2042_RETAINED_LEASE_WITNESS'
witness = '-DPIONEER_SG2042_RECOVERY_WITNESS'
for flags in ([retained], [witness, retained, '-DPIONEER_SG2042_RECOVERY_QUALIFICATION'],
              [witness, retained, '-DPIONEER_SG2042_RECOVERY_GATEWAY_WITNESS']):
    result = subprocess.run(base + flags + ['-'], input=source, text=True, capture_output=True)
    assert result.returncode != 0 and 'isolated non-loadable controller witness' in result.stderr
result = subprocess.run(base + [witness, retained, '-'], input=source, text=True, capture_output=True)
assert result.returncode == 0, result.stderr
print('PASS retained lease refuses unguarded, qualification and gateway builds')
