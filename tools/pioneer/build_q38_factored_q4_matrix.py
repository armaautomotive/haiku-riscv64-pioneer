#!/usr/bin/env python3
"""Build the new returning factored-Q4 component,NOT a native owner/driver."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv) == 3 and not os.environ.get('PYTHONOPTIMIZE')
sdk, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
source = here / 'blackhole_q38_factored_q4_matrix.cpp'
stem = 'blackhole_q38_factored_q4_matrix'
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
out.mkdir(mode=0o700)
compile = [tool + 'g++', '-DPIONEER_Q38_FACTORED_Q4_MATRIX_BUILD=1',
           '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1', '-std=c++17', '-Os', '-Wall', '-Wextra', '-Werror',
           '-ffreestanding', '-fno-builtin', '-fno-tree-loop-distribute-patterns', '-fno-exceptions',
           '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie', '-ffp-contract=off',
           '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
           '-msmall-data-limit=0', '-mno-relax', '-march=rv32i', '-mabi=ilp32', '-ffixed-s11',
           '-fomit-frame-pointer', '-fstack-usage', '-c', str(source), '-o', str(out / 'program.o')]
commands = [compile,
            [tool + 'as', '-march=rv32i', '-mabi=ilp32', '-mno-relax', str(here / (stem + '_start.S')), '-o', str(out / 'start.o')],
            [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', str(here / (stem + '.ld')),
             str(out / 'start.o'), str(out / 'program.o'), '-Map', str(out / 'program.map'), '-o', str(out / 'program.elf')]]
for section in ('boot', 'text'):
    commands.append([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(out / 'program.elf'), str(out / (section + '.bin'))])
for command in commands:
    subprocess.run(command, cwd=out, check=True)
returning = [a.replace(str(out / 'program.o'), str(out / 'return-only.o')) for a in compile]
returning.insert(1, '-DPIONEER_Q38_FACTORED_Q4_RETURN_ONLY=1')
subprocess.run(returning, cwd=out, check=True)
commands.append(returning)
unresolved = subprocess.check_output([tool + 'nm', '-u', str(out / 'return-only.o')], text=True)
assert set(row.split()[-1] for row in unresolved.splitlines()) <= {'factored_q4_fault', 'memset'}
assert not subprocess.check_output([tool + 'nm', '-u', str(out / 'program.elf')]).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(out / 'program.elf')], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_bytes(subprocess.check_output([tool + 'objdump', '-d', str(out / 'program.elf')]))
(out / 'build-argv.json').write_text(json.dumps(commands, indent=2) + '\n')
frames = [row.split('\t') for row in (out / 'program.su').read_text().splitlines()]
assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 4096 for row in frames)
for name in ('boot.bin', 'text.bin'):
    data = (out / name).read_bytes()
    assert len(data) == 4 if name == 'boot.bin' else 0 < len(data) <= 16384 and len(data) % 4 == 0
    print('FACTORED_Q4_RETURNING_BUILD', name, len(data), hashlib.sha256(data).hexdigest(), flush=True)
print('BUILD_ONLY_NO_NATIVE_OWNER_HARDWARE_QUALITY_OR_TPS_AUTHORITY')
