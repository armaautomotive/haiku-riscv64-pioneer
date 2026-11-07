#!/usr/bin/env python3
"""Run exact pinned candidate Gather-check instructions, no device access."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image = Path(sys.argv[1]); assert image.is_absolute()
control = Path(sys.argv[2]); assert control.is_absolute()
code = (image / 'text.bin').read_bytes()
assert len(code) == 130316 and hashlib.sha256(code).hexdigest() == '6a7fe5c763e930a6b762a274b46746918a12d691e0cbe1c65cfe9a177b787033'
old = (control / 'text.bin').read_bytes()
assert len(old) == 129748 and hashlib.sha256(old).hexdigest() == 'c9f60a2c5b34686b3c7a1e2d6b062d3588821128a09be2e24967a3748204b12c'
here = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-snapshot-rv32-', dir='/private/tmp') as tmp:
    binary = Path(tmp) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_early_job_snapshot_instruction_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image), str(control)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
