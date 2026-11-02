#!/usr/bin/env python3
"""Actual emitted loop plus separate owner lifecycle tests; OFFLINE ONLY."""
import hashlib
import os
import pathlib
import subprocess
import sys

assert len(sys.argv) == 3
sdk, out = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
control = out/'unchanged-leaf'
subprocess.run([sys.executable, str(here/'run_pioneer_dram_read_sweep_tests.py'), str(sdk), str(control)], check=True)
image = control/'image'
assert hashlib.sha256((image/'program.elf').read_bytes()).hexdigest() == '7002ccac8cdaf7b4c0cef7890ebc538d666eedbf4b1f407b59ea5e7136cdda5f'
test = out/'owner-test'
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(here/'pioneer_dram_read_sweep_owner_test.cpp'), '-o', str(test)], check=True)
subprocess.run([str(test), str(image/'boot.bin'), str(image/'text.bin')], check=True)
print('CLOSED_OFFLINE_OWNER_ONLY actual shared-byte virgin RV32/7bank routes/snapshot/start/STOP/HOLD/'
      'last-chunk independent check/full restore/uncertainty retention;native scope+ARC are fixture hooks;'
      'NO native adapter/public ABI/hardware/peak bandwidth/inference/Work qualification', flush=True)
