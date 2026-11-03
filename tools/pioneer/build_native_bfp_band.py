#!/usr/bin/env python3
"""Build only:distinct fixed full-K native BFP8/broadcast/fidelity image."""
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
parser.add_argument('width', type=int, choices=(5120, 6144, 17408))
parser.add_argument('phases', type=int, choices=(1, 4))
parser.add_argument('--direct', action='store_true', help='Separate direct-issue code-generation variant')
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
     '-DPIONEER_NATIVE_BFP_BAND_K=' + str(args.width), '-DPIONEER_NATIVE_BFP_BAND_PHASES=' + str(args.phases),
     '-c', str(here / 'blackhole_q38_native_bfp_band.cpp'), '-o', str(args.output / 'program.o')],
    [tool + 'as', '-march=rv32i', '-mabi=ilp32', '-mno-relax', str(here / 'blackhole_q38_native_bfp_band_start.S'), '-o', str(args.output / 'start.o')],
    [tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', str(here / 'blackhole_q38_native_bfp_band.ld'),
     str(args.output / 'start.o'), str(args.output / 'program.o'), '-Map', str(args.output / 'program.map'), '-o', str(args.output / 'program.elf')],
]
if args.direct:
    commands[0].insert(1, '-DPIONEER_NATIVE_BFP_BAND_DIRECT=1')
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
    assert len(data) == 4 if name == 'boot.bin' else 0 < len(data) <= 4096 and len(data) % 4 == 0
    print(hashlib.sha256(data).hexdigest(), len(data), name)
print('OFFBOARD_NATIVE_BFP_BAND_BUILD_ONLY K', args.width, 'PHASES', args.phases,
      'DIRECT', int(args.direct), ';NO staging,NoC,GDDR,owner,driver,hardware,model quality or speed permission')
