#!/usr/bin/env python3
"""Build-only original-Q6/Q8 SFPU operand functions,no boot/driver/launch."""
import argparse
import hashlib
import os
import pathlib
import subprocess

assert __debug__
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
parser.add_argument('--inline', action='store_true')
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
args.output.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
tool = str(args.haiku_build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
commands = [
    [tool + 'g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-march=rv32i', '-mabi=ilp32',
     '-mno-relax', '-ffreestanding', '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fstack-usage',
     '-c', str(here / 'blackhole_q38_mixed_fpu_decode.cpp'), '-o', str(args.output / 'operand.o')],
    [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '-Ttext=0x70000', '-e', 'pioneer_q6_fpu_prepare',
     str(args.output / 'operand.o'), '-o', str(args.output / 'operand.elf')],
    [tool + 'objcopy', '-O', 'binary', '-j', '.text', str(args.output / 'operand.elf'), str(args.output / 'text.bin')],
]
if args.inline:
    commands[0].insert(1, '-DPIONEER_MIXED_FPU_DECODE_INLINE=1')
for command in commands:
    subprocess.run(command, check=True)
for command, name in (([tool + 'nm', '-n', str(args.output / 'operand.elf')], 'symbols.txt'),
                      ([tool + 'objdump', '-d', str(args.output / 'operand.elf')], 'disassembly.txt')):
    (args.output / name).write_bytes(subprocess.check_output(command))
(args.output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
data = (args.output / 'text.bin').read_bytes()
assert 0 < len(data) <= 4096 and not len(data) % 4
print('RV32I_MIXED_FPU_OPERAND_SHA256', hashlib.sha256(data).hexdigest(), 'bytes', len(data))
print('BUILD_ONLY seven functions;NO boot/owner/NoC/Dst/unpack/driver/hardware/quality/speed authority')
