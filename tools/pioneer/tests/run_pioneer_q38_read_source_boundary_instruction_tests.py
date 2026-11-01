#!/usr/bin/env python3
"""Execute authenticated read/write/full guards; synthetic ABI inputs only."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
candidate = pathlib.Path(sys.argv[1])
assert candidate.is_absolute() and not candidate.is_symlink()
here = pathlib.Path(__file__).resolve().parent
tree = ast.parse((here.parent / 'build_qwen38_read_source_boundary_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert pins
for name, size, digest in pins:
    data = (candidate / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
assert '-DPIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE=1' in (candidate / 'compile-argv.txt').read_text().splitlines()
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
symbols = subprocess.check_output([tool + 'nm', '-n', str(candidate / 'program.elf')], text=True)
assert symbols == (candidate / 'program.symbols').read_text()


def entry(name):
    found = re.findall(r'^([0-9a-f]+) T ' + name + r'$', symbols, re.M)
    assert len(found) == 1, name
    return int(found[0], 16)


full, packet, direction = map(entry, ('q38_layer_return_recurrent_seal', 'q38_layer_return_recurrent_packet_seal',
                                     'q38_layer_return_recurrent_direction_packet_seal'))
fault = entry('q38_token_fault_report')
text = (candidate / 'text.bin').read_bytes()
words = tuple(int.from_bytes(text[fault-0x40000+i:fault-0x40000+i+4], 'little') for i in range(0, 32, 4))
assert words == (0x000752b7, 0x4612a023, 0x51465337, 0x43130313,
                 0x0ff0000f, 0x4462a423, 0x0ff0000f, 0x0000006f)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',
                   (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-read-source-instructions-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    mapping, binary = out / 'entries', out / 'test'
    mapping.write_text(' '.join(format(value, 'x') for value in (full, packet, direction, full, fault+28)) + '\n')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags,
                    str(here / 'pioneer_q38_read_source_boundary_instruction_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(candidate), str(mapping)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
