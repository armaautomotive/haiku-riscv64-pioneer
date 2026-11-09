#!/usr/bin/env python3
"""Replay exact producer quantization on pinned CPU attention data only."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
reference=here.parents[2]/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/cpu-attention-input-v1.bin'
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('role override refused')
if reference.is_symlink() or reference.stat().st_size!=24576 or hashlib.sha256(reference.read_bytes()).hexdigest()!='972b0747f8b03d422a7c5315ccd0fd15bf227231b1c316d5e7026c53eaf0ebf0':
    raise SystemExit('CPU reference identity refused')
with tempfile.TemporaryDirectory(prefix='qgw8-input-quantizer-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_dense_input_quantizer_replay.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(reference)],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
