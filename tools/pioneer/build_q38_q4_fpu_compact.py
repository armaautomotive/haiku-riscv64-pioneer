#!/usr/bin/env python3
"""Build-only compact ORIGINAL-metadata FPU image;reproduce old7036B first."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

INSPECT_PINS = (
    ('boot.bin', 4, 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
    ('text.bin', 4948, '0dce59d83dd56c3c800bde8bbb2bcd4e32d26cf1f71e2292f9eca7490fe978df'),
    ('program.elf', 16632, '294911c76c35db871e2f27fadd14af7e0b32cd5d17e467f0da4937c2b9850633'),
)  # Binds current INSPECT bytes only,never a native/Work admission.

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
assert len(sys.argv) == 4 and sys.argv[3] == '--inspect-candidate'
build, out = map(Path, sys.argv[1:3])
assert build.is_absolute() and build.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
out.mkdir(mode=0o700)
control = out / 'unchanged-expanded'
sys.argv = [str(here / 'build_q38_q4_fpu_block.py'), str(build), str(control), '--direct']
old = runpy.run_path(sys.argv[0])
assert hashlib.sha256((control / 'text.bin').read_bytes()).hexdigest() == '7e3efc36b1447a3880081caf899175a87378f894ea684d7623572625242e04d4'
commands = [[arg.replace(str(control), str(out)).replace('blackhole_q38_q4_fpu_block', 'blackhole_q38_q4_fpu_compact') for arg in row]
            for row in old['commands']]
compile = commands[0]
compile.remove('-DPIONEER_Q4_FPU_BLOCK_DIRECT=1')
compile.insert(1, '-DPIONEER_Q38_COMPACT_Q4_FPU_IMAGE=1')
compile[compile.index('-O2')] = '-Os'
for command in commands:
    subprocess.run(command, check=True, cwd=out)
for name, size, digest in INSPECT_PINS:
    data = (out / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, 'exact complete candidate ' + name
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
assert not subprocess.check_output([tool + 'nm', '-u', str(out / 'program.elf')]).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(out / 'program.elf')], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M), 'no writable globals/GOT'
dis = subprocess.check_output([tool + 'objdump', '-d', str(out / 'program.elf')], text=True)
assert not re.search(r'\b(?:mul\w*|div\w*|rem\w*|ebreak)\b', dis)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(dis)
(out / 'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
frames = [row.split('\t') for row in (out / 'program.su').read_text().splitlines()]
assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 4096 for row in frames)
for name in ('boot.bin', 'text.bin'):
    data = (out / name).read_bytes()
    assert len(data) == 4 if name == 'boot.bin' else 0 < len(data) <= 8192 and not len(data) % 4
    print('COMPACT_INSPECT', name, len(data), hashlib.sha256(data).hexdigest(), flush=True)
print('BUILD_ONLY_COMPACT_FPU_old7036Exact_ORIGINAL8KiB_BOUND_NOT_EXECUTION_NATIVE_QUALITY_OR_SPEED', flush=True)
