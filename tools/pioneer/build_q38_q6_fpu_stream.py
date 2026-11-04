#!/usr/bin/env python3
"""Build only:NEW full-K double-buffered Q6 GDDR/SFPU/FPU streaming image."""
import argparse
import hashlib
import os
import pathlib
import subprocess

if not __debug__:
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
parser.add_argument('width', type=int, choices=(5120,6144))
parser.add_argument('--size', action='store_true', help='Use size optimization;fixed8KiB ROM is NEVER increased')
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
args.output.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
tool = str(args.haiku_build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
commands = [
    [tool + 'g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
     '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
     '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
     '-msmall-data-limit=0', '-mno-relax', '-march=rv32i', '-mabi=ilp32', '-fstack-usage',
     '-c', str(here / 'blackhole_q38_q6_fpu_stream.cpp'), '-o', str(args.output / 'program.o')],
    [tool + 'as', '-march=rv32i', '-mabi=ilp32', '-mno-relax', str(here / 'blackhole_q38_q6_fpu_stream_start.S'), '-o', str(args.output / 'start.o')],
    [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', str(here / 'blackhole_q38_q6_fpu_stream.ld'),
     str(args.output / 'start.o'), str(args.output / 'program.o'), '-Map', str(args.output / 'program.map'), '-o', str(args.output / 'program.elf')],
]
commands[0].insert(1, '-DPIONEER_Q6_FPU_STREAM_K=' + str(args.width))
if args.size:
    commands[0][commands[0].index('-O2')]='-Os'
for section in ('boot', 'text'):
    commands.append([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(args.output / 'program.elf'), str(args.output / (section + '.bin'))])
for command in commands:
    subprocess.run(command, check=True, cwd=args.output)
(args.output / 'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
for command, name in (([tool + 'nm', '-n', str(args.output / 'program.elf')], 'program.symbols'),
                      ([tool + 'objdump', '-d', str(args.output / 'program.elf')], 'program.disassembly')):
    (args.output / name).write_bytes(subprocess.check_output(command))
for name in ('boot.bin', 'text.bin'):
    data = (args.output / name).read_bytes()
    assert len(data) == 4 if name == 'boot.bin' else 0 < len(data) <= 8192 and not len(data) % 4
    print(hashlib.sha256(data).hexdigest(), len(data), name)
print('OFFBOARD_Q6_FPU_STREAM_BUILD_ONLY;NEW Q6S1 marker/two buffers/full-K/8KiB bound;NO native owner/stage/driver/hardware/quality/speed authority')
