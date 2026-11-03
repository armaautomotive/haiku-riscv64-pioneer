#!/usr/bin/env python3
"""Actual eight-worker instructions + owner lifecycle OFFBOARD, not native IO."""
import os
import pathlib
import subprocess
import sys

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
image,out,old=map(pathlib.Path,sys.argv[1:])
assert image.is_absolute() and old.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
symbols={}
for row in (image/'program.symbols').read_text().splitlines():
    fields=row.split()
    if len(fields)==3:symbols[fields[2]]=int(fields[0],16)
good,bad=(symbols[k] for k in ('dram_sweep_park','dram_sweep_fault_idle'))
# Exact old-v4 counterexample first: the real captured P100A remapping makes
# original raw==logical self checking refuse, BEFORE any DRAM request.
subprocess.run([str(image.parent/'image-test'),str(old/'boot.bin'),str(old/'text.bin'),
                format(good,'x'),format(bad,'x'),'--old-raw-coordinate-control'],check=True)
binary=out/'owner-test'
bracket=out/'host-bracket-test'
subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined',str(here/'pioneer_dram_cohort_host_bracket_test.cpp'),'-o',str(bracket)],check=True)
subprocess.run([str(bracket)],check=True)
subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined',str(here/'pioneer_dram_cohort_read_sweep_owner_test.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary),str(image/'boot.bin'),str(image/'text.bin'),format(good,'x'),format(bad,'x')],check=True)
