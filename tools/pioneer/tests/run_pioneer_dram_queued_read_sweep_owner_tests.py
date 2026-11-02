#!/usr/bin/env python3
"""Actual queued RV32 bytes and owner histories; never opens hardware."""
import os
import pathlib
import subprocess
import sys
assert len(sys.argv)==3
sdk,out=map(pathlib.Path,sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
leaf=out/'leaf'
subprocess.run([sys.executable,str(here/'run_pioneer_dram_queued_read_sweep_tests.py'),str(sdk),str(leaf)],check=True)
binary=out/'owner-test'
subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
 '-fsanitize=address,undefined',str(here/'pioneer_dram_queued_read_sweep_owner_test.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary),str(leaf/'image/boot.bin'),str(leaf/'image/text.bin')],check=True)
