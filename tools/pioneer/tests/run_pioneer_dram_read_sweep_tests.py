#!/usr/bin/env python3
"""Execute the actual isolated read-sweep instructions, never an accelerator."""
import os
import pathlib
import subprocess
import sys

assert len(sys.argv) == 3
sdk, out = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
image = out/'image'
subprocess.run([sys.executable, str(here.parent/'build_dram_read_sweep.py'), str(sdk), str(image)], check=True)
symbols = {}
for row in (image/'program.symbols').read_text().splitlines():
    fields = row.split()
    if len(fields) == 3:
        assert fields[2] not in symbols
        symbols[fields[2]] = int(fields[0], 16)
for name in ('dram_sweep_park', 'dram_sweep_fault_idle'):
    assert 0x64000 <= symbols[name] < 0x6e000 and not symbols[name] % 4
assert symbols['dram_sweep_park'] != symbols['dram_sweep_fault_idle']
test = out/'image-test'
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(here/'pioneer_dram_read_sweep_image_test.cpp'), '-o', str(test)], check=True)
subprocess.run([str(test), str(image/'boot.bin'), str(image/'text.bin'),
                format(symbols['dram_sweep_park'], 'x'), format(symbols['dram_sweep_fault_idle'], 'x')], check=True)
print('CLOSED_OFFLINE_ONLY actual image+ASan/UBSan execution;NO installation/hardware/bandwidth/Work result', flush=True)
