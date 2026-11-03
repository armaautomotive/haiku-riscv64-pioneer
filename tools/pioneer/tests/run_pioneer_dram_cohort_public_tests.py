#!/usr/bin/env python3
"""Actual native/public refusal methods;synthetic successful receipt DATA only."""
import os
import pathlib
import subprocess
import sys
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out=pathlib.Path(sys.argv[1]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
binary=out/'public-test'
subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
 '-fsanitize=address,undefined',str(here/'pioneer_dram_cohort_public_test.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True)
