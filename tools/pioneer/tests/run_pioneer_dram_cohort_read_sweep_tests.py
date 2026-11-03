#!/usr/bin/env python3
"""Build and execute actual eight-worker RV32 cohort bytes OFFBOARD ONLY."""
import hashlib
import os
import pathlib
import subprocess
import sys

assert len(sys.argv)==3
sdk,out=map(pathlib.Path,sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent;image=out/'image'
subprocess.run([sys.executable,str(here.parent/'build_dram_cohort_read_sweep.py'),str(sdk),str(image)],check=True)
symbols={}
for row in (image/'program.symbols').read_text().splitlines():
    fields=row.split()
    if len(fields)==3:
        assert fields[2] not in symbols
        symbols[fields[2]]=int(fields[0],16)
good,bad=(symbols[key] for key in ('dram_sweep_park','dram_sweep_fault_idle'))
assert 0x64000<=good<0x6e000 and 0x64000<=bad<0x6e000 and good!=bad and not(good&3 or bad&3)
binary=out/'image-test'
subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined',str(here/'pioneer_dram_cohort_read_sweep_image_test.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary),str(image/'boot.bin'),str(image/'text.bin'),format(good,'x'),format(bad,'x')],check=True)
print('CLOSED_OFFBOARD_ONLY cohort ELF SHA256 '+hashlib.sha256((image/'program.elf').read_bytes()).hexdigest(),flush=True)
