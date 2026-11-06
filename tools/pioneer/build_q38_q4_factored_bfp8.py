#!/usr/bin/env python3
"""Build separate factored-Q4 RV32 functions;NO launchable service/native grant."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

if not __debug__:
    raise SystemExit('Checks required')
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('haiku_build', type=Path)
p.add_argument('output', type=Path)
p.add_argument('--inline', action='store_true')
a = p.parse_args()
assert a.haiku_build.is_absolute() and a.haiku_build.is_dir()
assert a.output.is_absolute() and not a.output.exists() and not a.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
sources = [here / n for n in ('blackhole_q38_q4_factored_bfp8.cpp', 'PioneerQ38Q4FactoredBfp8.h', 'PioneerSfpuDivide.h')]
before = {str(f): hashlib.sha256(f.read_bytes()).hexdigest() for f in sources}
a.output.mkdir(mode=0o700)
t = str(a.haiku_build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
commands = [
    [t + 'g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-march=rv32i', '-mabi=ilp32',
     '-mno-relax', '-ffreestanding', '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fstack-usage',
     '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1', *(['-DPIONEER_Q38_Q4_FACTORED_BFP8_INLINE=1'] if a.inline else []),
     '-c', str(sources[0]), '-o', str(a.output / 'operand.o')],
    [t + 'ld', '-m', 'elf32lriscv', '--no-relax', '-Ttext=0x70000', '-e', 'pioneer_q4_factored_init',
     str(a.output / 'operand.o'), '-o', str(a.output / 'operand.elf')],
    [t + 'objcopy', '-O', 'binary', '-j', '.text', str(a.output / 'operand.elf'), str(a.output / 'text.bin')],
]
for command in commands:
    subprocess.run(command, check=True)
for command, name in (([t + 'nm', '-n', str(a.output / 'operand.elf')], 'symbols.txt'),
                      ([t + 'objdump', '-d', str(a.output / 'operand.elf')], 'disassembly.txt')):
    (a.output / name).write_bytes(subprocess.check_output(command))
binary = (a.output / 'text.bin').read_bytes()
assert 0 < len(binary) <= 4096 and len(binary) % 4 == 0
assert before == {str(f): hashlib.sha256(f.read_bytes()).hexdigest() for f in sources}
(a.output / 'source-sha256.json').write_text(json.dumps(before, sort_keys=True, indent=2) + '\n')
(a.output / 'build-argv.json').write_text(json.dumps(commands, indent=2) + '\n')
print('RV32_FACTORED_Q4_FUNCTIONS_SHA256', hashlib.sha256(binary).hexdigest(), 'bytes', len(binary))
print('BUILD_ONLY_NO_PACK_UNPACK_MATRIX_NATIVE_OWNER_MODEL_QUALITY_OR_SPEED_AUTHORITY')
