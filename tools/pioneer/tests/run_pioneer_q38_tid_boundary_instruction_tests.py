#!/usr/bin/env python3
"""Execute authenticated FIFO/TID helpers;synthetic inputs,not model IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
candidate = pathlib.Path(sys.argv[1])
here = pathlib.Path(__file__).resolve().parent
tree = ast.parse((here.parent / 'build_qwen38_tid_boundary_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert candidate.is_absolute() and pins
for name, size, digest in pins:
    data = (candidate / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
symbols = subprocess.check_output([tool + 'nm', '-nC', str(candidate / 'program.elf')], text=True)
entries = []
for suffix in ('PacketQuiet()', 'FullQuiet()', 'InitiatorsQuiet()'):
    names = [(int(row[0], 16), row[2]) for line in symbols.splitlines()
             if len(row := line.split()) == 3 and row[2] == 'PioneerQ38Tid' + suffix]
    assert len(names) == 1, (suffix, names)
    entries.append(names[0][0])
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',
                   (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-tid-instructions-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp) / 'test'
    mapping = pathlib.Path(tmp) / 'entries'
    mapping.write_text(' '.join(format(a, 'x') for a in entries) + '\n')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags,
                    str(here / 'pioneer_q38_tid_boundary_instruction_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(candidate), str(mapping)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
