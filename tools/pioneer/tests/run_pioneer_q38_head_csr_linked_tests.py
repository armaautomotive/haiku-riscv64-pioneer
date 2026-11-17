#!/usr/bin/env python3
"""Run the pinned RAW/coordinator candidate ELF code in a simulated NoC."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


assert __debug__ and len(sys.argv) == 4
sdk, original, candidate = map(Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk, original, candidate))
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
here = Path(__file__).resolve().parent
identities = ((original, 'e93c1a57c21de073233689741e00ddc9e55ad20b1fd74d0758aa3100ce313f66'),
              (candidate, '0c78b9ef5035016ed1fd18d5eadf9f479ab728cdd7e57182c0046060a47ed962'))
with TemporaryDirectory(prefix='pioneer-head-csr-linked-', dir='/private/tmp') as temporary:
    stage = Path(temporary)
    folders = []
    for index, (elf, sha) in enumerate(identities):
        assert hashlib.sha256(elf.read_bytes()).hexdigest() == sha, elf
        folder = stage / str(index)
        folder.mkdir()
        for section in ('text', 'terminal', 'conv'):
            subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section,
                            elf, folder / (section + '.bin')], check=True)
        (folder / 'program.symbols').write_bytes(subprocess.check_output([tool + 'nm', '-nC', elf]))
        folders.append(folder)
    binary = stage / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-function', '-Wno-misleading-indentation', '-fsanitize=address,undefined',
                    here / 'pioneer_q38_head_csr_linked_test.cpp', '-o', binary], check=True)
    subprocess.run([binary, *folders], check=True, timeout=60, env=dict(os.environ, UBSAN_OPTIONS='halt_on_error=1'))
