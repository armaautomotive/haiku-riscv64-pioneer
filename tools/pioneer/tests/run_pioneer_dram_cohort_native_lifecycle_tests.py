#!/usr/bin/env python3
"""Actual old/new Native.inc endpoint contracts,not hardware or engine proof."""
import os
import pathlib
import subprocess
import sys
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out=pathlib.Path(sys.argv[1]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
for stem in ('pioneer_dram_sweep_native_lifecycle_test','pioneer_dram_cohort_native_lifecycle_test'):
    binary=out/stem
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/(stem+'.cpp')),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
