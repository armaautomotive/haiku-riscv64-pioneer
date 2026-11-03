#!/usr/bin/env python3
"""Compile offboard RV32I operand functions; NOT a launchable image or driver."""
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
parser.add_argument('--inline', action='store_true', help='Offboard direct-Tensix encoding probe, not launch authority')
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(name.startswith(('PIONEER_', 'TT_PIONEER_')) for name in os.environ)
args.output.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
tool = str(args.haiku_build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
commands = [
    [tool + 'g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-march=rv32i', '-mabi=ilp32',
     '-mno-relax', '-ffreestanding', '-fno-exceptions', '-fno-rtti', '-fno-stack-protector',
     '-c', str(here / 'blackhole_q38_native_raw_operand.cpp'), '-o', str(args.output / 'operand.o')],
    [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '-Ttext=0x70000', '-e', 'pioneer_native_raw2',
     str(args.output / 'operand.o'), '-o', str(args.output / 'operand.elf')],
    [tool + 'objcopy', '-O', 'binary', '-j', '.text', str(args.output / 'operand.elf'), str(args.output / 'text.bin')],
]
if args.inline:
    commands[0].insert(1, '-DPIONEER_NATIVE_RAW_INLINE=1')
for command in commands:
    subprocess.run(command, check=True)
for command, name in (([tool + 'nm', '-n', str(args.output / 'operand.elf')], 'symbols.txt'),
                      ([tool + 'objdump', '-d', str(args.output / 'operand.elf')], 'disassembly.txt')):
    (args.output / name).write_bytes(subprocess.check_output(command))
(args.output / 'build-argv.txt').write_text('\n'.join('\t'.join(command) for command in commands) + '\n')
print('RV32I_NATIVE_RAW_OPERAND_SHA256', hashlib.sha256((args.output / 'text.bin').read_bytes()).hexdigest())
print('BUILD_ONLY functions;NO boot,main,driver,installation,unpack,lifecycle,hardware or performance authority')
