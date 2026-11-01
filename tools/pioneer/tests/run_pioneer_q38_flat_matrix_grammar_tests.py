#!/usr/bin/env python3
"""UNSELECTED pure DATA predicate equivalence,not model/native/performance proof."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
        '-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-flat-matrix-grammar-',dir='/private/tmp') as name:
    for folded in (False,True):
        binary=pathlib.Path(name)/('folded' if folded else 'legacy')
        flags=(['-DPIONEER_Q38_JOB_BOOT_FOLD_IMAGE=1','-DPIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST=1',
                '-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
                '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'] if folded else [])
        subprocess.run([*common,*flags,str(here/'pioneer_q38_flat_matrix_grammar_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
        print('PASS unchanged '+('job-boot-fold' if folded else 'legacy')+' reference and direct DATA predicate',flush=True)
