#!/usr/bin/env python3
"""Compile unchanged MatrixJob source into a host-only descriptor/receipt test."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parent
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
source = (root / 'blackhole_qwen38_mixed_projection_root.cpp').read_text()
start = 'static void MatrixJob(const uint32_t* d,unsigned w,unsigned sequence){'
end = '\n#ifndef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION\nstatic void MatrixWait'
assert source.count(start) == source.count(end) == 1
body = source[source.index(start):source.index(end)]
with tempfile.TemporaryDirectory(prefix='pioneer-snapshot-data-', dir='/private/tmp') as tmp:
    temp = Path(tmp)
    (temp / 'extracted-matrix-job.inc').write_text(body + '\n')
    binary = temp / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', str(root), '-I', str(temp),
                    str(here / 'pioneer_q38_early_job_snapshot_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
