#!/usr/bin/env python3
"""Build an isolated OFFBOARD two-base diagnostic; no driver or device access."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
tool = str(args.haiku_build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
args.output.mkdir(mode=0o700)
commands = [
 [tool+'g++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
  '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
  '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
  '-msmall-data-limit=0', '-mno-relax', '-march=rv32imf', '-mabi=ilp32', '-fstack-usage', '-ffixed-s11',
  '-c', str(here/'blackhole_q38_srca_probe.cpp'), '-o', str(args.output/'program.o')],
 [tool+'as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax', str(here/'blackhole_q38_srca_probe_start.S'),
  '-o', str(args.output/'start.o')],
 [tool+'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', str(here/'blackhole_q38_srca_probe.ld'),
  str(args.output/'start.o'), str(args.output/'program.o'), '-Map', str(args.output/'program.map'),
  '-o', str(args.output/'program.elf')],
]
for section in ('boot', 'text'):
    commands.append([tool+'objcopy', '-O', 'binary', '-j', '.'+section, str(args.output/'program.elf'), str(args.output/(section+'.bin'))])
for command in commands:
    subprocess.run(command, check=True, cwd=args.output)
(args.output/'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
for command, name in (([tool+'nm', '-n', str(args.output/'program.elf')], 'program.symbols'),
                      ([tool+'objdump', '-d', str(args.output/'program.elf')], 'program.disassembly')):
    with (args.output/name).open('w') as output:
        subprocess.run(command, stdout=output, check=True)
for name in ('boot.bin', 'text.bin'):
    value = (args.output/name).read_bytes()
    assert len(value) == 4 if name == 'boot.bin' else 0 < len(value) <= 8192 and len(value) % 4 == 0
    print(hashlib.sha256(value).hexdigest(), len(value), name)
print('OFFBOARD_BUILD_ONLY: no lifecycle, driver, hardware, model quality or speed claim')
