#!/usr/bin/env python3
"""Build ONLY LOAD-ONLY82 or explicit C64 control83;NO device/byte/math proof."""
import argparse
import hashlib
import os
import pathlib
import struct
import subprocess

if not __debug__:
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
parser.add_argument('--alignment64-control', action='store_true', help='separate LOAD-ONLY83 C64 placement,never a fix/pass claim')
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
tool = str(args.haiku_build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
args.output.mkdir(mode=0o700)
commands = [
    [tool + 'g++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
     '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
     '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
     '-msmall-data-limit=0', '-mno-relax', '-march=rv32i', '-mabi=ilp32', '-fstack-usage',
     '-DPIONEER_NATIVE_DRAM_LOAD_ONLY=1', '-c', str(here / 'blackhole_q38_native_dram_load.cpp'), '-o', str(args.output / 'program.o')],
    [tool + 'as', '-march=rv32i', '-mabi=ilp32', '-mno-relax', str(here / 'blackhole_q38_native_dram_load_start.S'), '-o', str(args.output / 'start.o')],
    [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', str(here / 'blackhole_q38_native_dram_load.ld'),
     str(args.output / 'start.o'), str(args.output / 'program.o'), '-Map', str(args.output / 'program.map'), '-o', str(args.output / 'program.elf')],
]
if args.alignment64_control:
    commands[0].insert(1, '-DPIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64=1')
    commands[1].insert(1, '--defsym=PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64=1')
for section in ('boot', 'text'):
    commands.append([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(args.output / 'program.elf'), str(args.output / (section + '.bin'))])
for command in commands:
    subprocess.run(command, check=True, cwd=args.output)
(args.output / 'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
for command, name in (([tool + 'nm', '-n', str(args.output / 'program.elf')], 'program.symbols'),
                      ([tool + 'objdump', '-d', str(args.output / 'program.elf')], 'program.disassembly')):
    (args.output / name).write_bytes(subprocess.check_output(command))
symbols = {row.split()[2]: int(row.split()[0], 16) for row in (args.output / 'program.symbols').read_text().splitlines()}
for name, value in {'entry': 0x70000, 'done': 0x7000c, 'park': 0x70038, 'fault': 0x70040, 'fault_park': 0x70058}.items():
    assert symbols['native_dram_load_' + name] == value
for name in ('boot.bin', 'text.bin'):
    data = (args.output / name).read_bytes()
    assert len(data) == 4 if name == 'boot.bin' else 0 < len(data) <= 8192 and len(data) % 4 == 0
    if name == 'text.bin':
        assert data[0x38:0x40] == struct.pack('<II', 0x6f, 0xffdff06f)
    print(hashlib.sha256(data).hexdigest(), len(data), name)
print('OFFBOARD_LOAD_ONLY%d_BUILD;NO FPU/unpack/model/Work path,NO native ownership/byte/hardware/speed authority' % (83 if args.alignment64_control else 82))
