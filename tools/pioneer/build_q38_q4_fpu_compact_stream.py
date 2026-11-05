#!/usr/bin/env python3
"""Build-only compact full-K stream;reproduce old7824B image FIRST."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

INSPECT_PINS = {
    5120: (('text.bin',7644,'f67b7b5bc07bddcf5f3ddfccfbe1013c2680650ea1994231565f050d1f0d89c5'),
           ('program.elf',19796,'1d4cc347c45f9fe4735becf3ba7bea3faeef2e118071e4356a29bcca93ecfacb')),
    17408: (('text.bin',7644,'c95e0cc41971b37dd89786a2c3747b6d8ef6ebbde3a2ec837c63198982373b92'),
            ('program.elf',19796,'a8bb45834ee5f5181e6f24c2ff21a918edd10befa911de809991bc69a6ab5a36')),
}  # Current INSPECT identity only,never hardware/native authority.

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
assert len(sys.argv) == 5 and sys.argv[4] == '--inspect-candidate'
build, out = map(Path, sys.argv[1:3]);width = int(sys.argv[3])
assert width in (5120, 17408) and build.is_absolute() and build.is_dir()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
out.mkdir(mode=0o700);control = out / 'unchanged-expanded'
sys.argv = [str(here / 'build_q38_q4_fpu_stream.py'), str(build), str(control), str(width), '--size']
old = runpy.run_path(sys.argv[0])
digest = {5120: '7e4765f9ec803cc5986534ec4f947b93e5a32b4711a01223e5f39741dfce8e03',
          17408: 'cc22a0cb005ff037d751c0b307cc74c76586260e1910789616cbc0719d912295'}[width]
assert len((control / 'text.bin').read_bytes()) == 7824 and hashlib.sha256((control / 'text.bin').read_bytes()).hexdigest() == digest
commands = [[arg.replace(str(control), str(out)).replace('blackhole_q38_q4_fpu_stream', 'blackhole_q38_q4_fpu_compact_stream')
             .replace('PIONEER_Q4_FPU_STREAM_K=', 'PIONEER_Q4_FPU_COMPACT_STREAM_K=') for arg in row] for row in old['commands']]
commands[0].insert(1, '-DPIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE=1')
for command in commands:
    subprocess.run(command, check=True, cwd=out)
for name, size, digest in INSPECT_PINS[width]:
    data = (out / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
assert not subprocess.check_output([tool + 'nm', '-u', str(out / 'program.elf')]).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(out / 'program.elf')], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_bytes(subprocess.check_output([tool + 'objdump', '-d', str(out / 'program.elf')]))
(out / 'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
frames = [r.split('\t') for r in (out / 'program.su').read_text().splitlines()]
assert frames and all(len(r) == 3 and r[2] == 'static' and int(r[1]) <= 4096 for r in frames)
for name in ('boot.bin', 'text.bin', 'program.elf'):
    data = (out / name).read_bytes()
    if name == 'boot.bin':
        assert len(data) == 4
    elif name == 'text.bin':
        assert 0 < len(data) <= 8192 and not len(data) % 4
    print('COMPACT_STREAM_INSPECT', width, name, len(data), hashlib.sha256(data).hexdigest(), flush=True)
print('BUILD_ONLY_COMPACT_FULL_K_ORIGINAL8KiB_BOUND;NOT_EXECUTION_NATIVE_QUALITY_OR_SPEED', flush=True)
