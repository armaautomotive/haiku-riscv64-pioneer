#!/usr/bin/env python3
"""Exact generated code and complete fixed L1 footprint, no native IO."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 or len(sys.argv)==4 and sys.argv[3]=='--sigmoid-candidate'
sigmoid=len(sys.argv)==4
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-live-footprint-',dir='/private/tmp') as temp:
 generated=pathlib.Path(temp)/'Programs.h'
 subprocess.run([sys.executable,str(here.parent/'generate_qwen38_mixed_live_programs.py'),*sys.argv[1:3],str(generated),
  *(['--sigmoid-root-only'] if sigmoid else [])],check=True)
 actual=here.parents[2]/('src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveSigmoidPrograms.h' if sigmoid else 'src/add-ons/kernel/drivers/misc/PioneerQ38MixedLivePrograms.h')
 assert generated.read_bytes()==actual.read_bytes(),'committed registry must reproduce from COMPLETE authenticated artifacts'
 binary=pathlib.Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *(['-DPIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS=1'] if sigmoid else []),str(here/'pioneer_q38_mixed_live_footprint_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:3]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
