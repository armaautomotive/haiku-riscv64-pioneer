#!/usr/bin/env python3
"""Frozen preoptimization reference vs per-call8-row lookup;DATA ONLY."""
import hashlib
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
reference=here/'reference/pioneer_q38_mixed_live_block_plan_before_lookup.h'
assert hashlib.sha256(reference.read_bytes()).hexdigest()=='2e8e94c0fe9213c04c6b171301d5f741d81d4d2480057f126076cbd5b2527446'
with tempfile.TemporaryDirectory(prefix='pioneer-plan-lookup-',dir='/private/tmp') as build:
 binary=pathlib.Path(build)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  '-I'+str(here.parent),str(here/'pioneer_q38_plan_lookup_equivalence_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
