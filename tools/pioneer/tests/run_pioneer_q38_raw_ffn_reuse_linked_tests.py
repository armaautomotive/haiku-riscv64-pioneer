#!/usr/bin/env python3
"""Audit the exact compact V2 ROOT ELF in the existing RV32 simulation."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


if not __debug__ or len(sys.argv) != 3:
    raise SystemExit('use SDK PINNED_COMPACT_V2_ELF (without -O)')
sdk, elf = map(Path, sys.argv[1:])
assert sdk.is_absolute() and elf.is_absolute() and elf.is_file() and not elf.is_symlink()
image = elf.read_bytes()
assert hashlib.sha256(image).hexdigest() == 'e59cdec3d494e7a264b6904fc6cca62fae49fb82d593ef070c5ff52b383b979d'
tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
here = Path(__file__).resolve().parent
with TemporaryDirectory(prefix='q38-raw-ffn-linked-', dir='/private/tmp') as folder:
    root = Path(folder)
    pinned = root/'checked.elf'
    pinned.write_bytes(image)
    for section in ('text','conv'):
        subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,pinned,root/(section+'.bin')],check=True)
    test = root/'test'
    subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-misleading-indentation','-fsanitize=address,undefined',
                    here/'pioneer_q38_raw_ffn_reuse_linked_test.cpp','-o',test],check=True)
    subprocess.run([test,root],check=True,timeout=60,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
