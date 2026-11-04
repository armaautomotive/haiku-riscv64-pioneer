#!/usr/bin/env python3
"""Build the complete batched full-card ROOT with the real five-EOG policy.

This produces executable firmware DATA only. No accelerator is opened.
The complete original corrected-output ROOT is reproduced first; only the
token-controller TU changes, not the recurrent/dense components or STOP ABI.
"""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin', 129748, 'c9f60a2c5b34686b3c7a1e2d6b062d3588821128a09be2e24967a3748204b12c'),
    ('terminal.bin', 8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin', 32680, 'abc7bf03ad5668530331116092f3aac6c240230922d2b7bb955b0798c99d5ccf'),
    ('program.elf', 191492, 'e93c1a57c21de073233689741e00ddc9e55ad20b1fd74d0758aa3100ce313f66'),
)
assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
build, out = map(pathlib.Path, sys.argv[1:])
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_batched_output_closure_root.py'),
                str(build), str(out / 'unchanged-output-closure')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
tool = parent['tool']
commands = []


def run(command):
    commands.append(list(map(str, command)))
    subprocess.run(commands[-1], check=True, cwd=out)


command = (out / 'unchanged-output-closure/compile-token-argv.txt').read_text().splitlines()
assert command[0] == tool + 'g++' and command.count('-o') == command.count('-c') == 1
assert pathlib.Path(command[command.index('-c') + 1]).name == 'blackhole_qwen38_token_root.cpp'
command[command.index('-o') + 1] = str(out / 'token.o')
command.insert(1, '-DPIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE=1')
run(command)
run([tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
     here / 'blackhole_qwen38_global_chain.ld', parent['base']['entry_object'],
     out / 'token.o', out / 'unchanged-output-closure/components-NOBOOT.o',
     parent['components'] / 'unchanged-feedback/feedback.o',
     '-Map', out / 'program.map', '-o', out / 'program.elf'])
for section in ('boot', 'text', 'terminal', 'conv'):
    run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section,
         out / 'program.elf', out / (section + '.bin')])
for section in ('boot', 'terminal'):
    assert (out / (section + '.bin')).read_bytes() == (out / 'unchanged-output-closure' / (section + '.bin')).read_bytes()
for section, bound in (('text', 0x20000), ('conv', 0x8000)):
    data = (out / (section + '.bin')).read_bytes()
    assert data and len(data) <= bound and len(data) % 4 == 0
    assert data != (out / 'unchanged-output-closure' / (section + '.bin')).read_bytes()
assert not subprocess.check_output([tool + 'nm', '-u', str(out / 'program.elf')], text=True).strip()
symbols = subprocess.check_output([tool + 'nm', '-nC', str(out / 'program.elf')], text=True)
assert 'GlobalChain::FullOffloadEog(unsigned int)' in symbols
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name, address in (('_start', 0x6000), ('scalar_entry', 0x40000),
                      ('scalar_fault_park', 0x60000), ('q38_global_chain_stopped', 0x60004)):
    assert re.search('^' + format(address, '08x') + ' T ' + name + '$', symbols, re.M), name
frames = [r.split('\t') for r in (out / 'token.su').read_text().splitlines()]
assert frames and all(len(r) == 3 and r[2] == 'static' and int(r[1]) <= 2048 for r in frames)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(subprocess.check_output(
    [tool + 'objdump', '-d', str(out / 'program.elf')], text=True))
subprocess.run([sys.executable, str(here / 'tests/pioneer_q38_full_offload_eog_isa_test.py'),
                str(build), str(out)], check=True)
(out / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
for section in ('boot', 'text', 'terminal', 'conv'):
    data = (out / (section + '.bin')).read_bytes()
    print(section, len(data), hashlib.sha256(data).hexdigest(), flush=True)
for name, size, digest in PINS:
    data = (out / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
print('PASS BUILD_ONLY complete batched/corrected-output ROOT first;new full-card all5-EOG controller;'
      'boot+STOP exact;all four sections fit existing journal;NO device/numerical/performance result', flush=True)
