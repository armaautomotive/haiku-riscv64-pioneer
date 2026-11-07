#!/usr/bin/env python3
"""Build-only paired RAW returning ROOT experiment; no device authority."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

from generate_qwen38_token_programs import authenticated, pins

assert __debug__ and len(sys.argv) == 4
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, control, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (sdk, control, out))
assert control.is_dir() and not control.is_symlink() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
old = authenticated(control, pins(here / 'build_qwen38_full_offload_batched_root.py', 'PINS'))
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
commands = []

def run(*values):
    command = list(map(str, values))
    commands.append(command)
    subprocess.run(command, check=True)

def rebase(command):
    command = list(command)
    assert command[0] == tool + 'g++' and command.count('-c') == command.count('-o') == 1
    command[command.index('-c') + 1] = str(here / Path(command[command.index('-c') + 1]).name)
    for at, value in enumerate(command[:-1]):
        if value == '-include':
            command[at + 1] = str(here / Path(command[at + 1]).name)
    return command

base = control / 'unchanged-output-closure'
saved = (control / 'build-argv.txt').read_text().splitlines()
link = saved[1].split('\t')
assert link[0] == tool + 'ld' and link.count('-T') == 1
entry = Path(link[link.index('-T') + 2])
feedback = next(Path(p) for p in link if Path(p).name == 'feedback.o')
conv = feedback.with_name('root_conv_sections.o')
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
    here / 'blackhole_qwen38_global_chain.ld', entry, control / 'token.o',
    base / 'components-NOBOOT.o', feedback, '-o', out / 'entire-control.elf')
assert (out / 'entire-control.elf').read_bytes() == old['program.elf'], 'whole control ELF'

objects = []
for kind, original in (('recurrent', base / 'output-closure-recurrent.o'),
                       ('dense', base / 'output-closure-dense.o'), ('token', control / 'token.o')):
    saved_command = saved[0].split('\t') if kind == 'token' else (base / ('compile-' + kind + '-argv.txt')).read_text().splitlines()
    command = rebase(saved_command)
    unchanged = out / ('unchanged-' + kind + '.o')
    command[command.index('-o') + 1] = str(unchanged)
    run(*command)
    assert unchanged.read_bytes() == original.read_bytes(), 'whole unchanged ' + kind + ' TU'
    if kind == 'token':
        objects.append(unchanged)
        continue
    candidate = out / ('snapshot-' + kind + '.o')
    command[command.index('-o') + 1] = str(candidate)
    command[1:1] = ['-DPIONEER_Q38_EARLY_JOB_SNAPSHOT_IMAGE=1', '-MMD', '-MF', str(out / (kind + '.d'))]
    run(*command)
    objects.append(candidate)
    (out / ('compile-' + kind + '-argv.txt')).write_text('\n'.join(command) + '\n')

raw, capsule = out / 'components-raw.o', out / 'components-NOBOOT.o'
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '-r', '--gc-sections',
    '-u', 'q38_return_recurrent_layer', '-u', 'q38_return_dense_layer',
    '-u', 'q38_global_chain_learned_rms', '-u', 'q38_global_chain_quantize14',
    objects[0], objects[1], conv, '-o', raw)
dead = ('mixed_recurrent_block_wait', 'mixed_recurrent_block_stopped', 'mixed_live_block_wait', 'mixed_live_block_stopped')
relocations = subprocess.check_output([tool + 'objdump', '-r', str(raw)], text=True)
for name in dead:
    assert not re.search(r'\b' + name + r'\b', relocations)
run(tool + 'objcopy', *['--strip-symbol=' + name for name in dead], raw, capsule)
unresolved = subprocess.check_output([tool + 'nm', '-u', str(capsule)], text=True).splitlines()
assert {row.split()[-1] for row in unresolved} == {'scalar_fault_park'}
elf = out / 'program.elf'
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
    here / 'blackhole_qwen38_global_chain.ld', entry, objects[2], capsule, feedback,
    '-Map', out / 'program.map', '-o', elf)
assert not subprocess.check_output([tool + 'nm', '-u', str(elf)]).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name, address in (('_start', 0x6000), ('scalar_entry', 0x40000),
                      ('scalar_fault_park', 0x60000), ('q38_global_chain_stopped', 0x60004)):
    assert re.search('^' + format(address, '08x') + ' T ' + name + '$', symbols, re.M), name
sections = {'boot': 4, 'text': 131072, 'terminal': 8, 'conv': 32768}
table = subprocess.check_output([tool + 'objdump', '-h', str(elf)], text=True).splitlines()
for i, row in enumerate(table):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+', row)
    if match and 'ALLOC' in table[i + 1]:
        assert match[1][1:] in sections, row
for section, limit in sections.items():
    run(tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, out / (section + '.bin'))
    data = (out / (section + '.bin')).read_bytes()
    assert data and not len(data) % 4 and len(data) <= limit
    if section in ('boot', 'terminal'):
        assert data == old[section + '.bin']
    print(section, len(data), hashlib.sha256(data).hexdigest(), flush=True)
for obj in objects:
    frames = [row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 4096 for row in frames)
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(subprocess.check_output([tool + 'objdump', '-d', str(elf)], text=True))
(out / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
print('BUILD ONLY: unchanged complete control and TUs reproduced; no device, native, correctness or speed qualification')
