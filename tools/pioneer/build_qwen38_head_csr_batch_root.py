#!/usr/bin/env python3
"""Build paired RAW Qwen ROOT variants; no native owner or device access."""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from generate_qwen38_token_programs import authenticated, pins


assert __debug__ and len(sys.argv) == 4
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, control, out = (Path(p) for p in sys.argv[1:])
here = Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (sdk, control, out))
assert control.is_dir() and not control.is_symlink() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
old = authenticated(control, pins(here / 'build_qwen38_full_offload_batched_root.py', 'PINS'))
# Only the two bound coordinator paths and their opt-in helper come from
# the worktree. Keep unrelated GDN and other helpers at the RAW baseline.
# The subsequent unchanged-object and linked-ELF checks are mandatory.
frozen = out / 'source'
frozen.mkdir(mode=0o700)
source = frozen / 'tools/pioneer'
archive = subprocess.Popen(['git', '-C', str(here.parents[1]), 'archive',
                            'df48ea751a', 'tools/pioneer', 'src/add-ons/kernel/drivers/misc'],
                           stdout=subprocess.PIPE)
with archive.stdout:
    unpack = subprocess.run(['tar', '-x', '-C', str(frozen)], stdin=archive.stdout)
assert archive.wait() == 0 and unpack.returncode == 0
for name in ('blackhole_qwen38_token_root.cpp', 'blackhole_qwen38_global_head_control.inc',
             'PioneerQ38HeadCsrBatch.h', 'PioneerQ38Qgw8CommitBoundaryRole.h',
             'PioneerQ38Qgw8CoarseProfileRole.h', 'PioneerQ38Qgw8LayerTraceRole.h',
             'PioneerQ38FastDecodeProfileBuildRole.h'):
    shutil.copyfile(here / name, source / name)
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
build = (control / 'build-argv.txt').read_text().splitlines()
compile_token, link = (build[i].split('\t') for i in range(2))
assert compile_token[0] == tool + 'g++' and compile_token.count('-c') == compile_token.count('-o') == 1
assert link[0] == tool + 'ld' and link.count('-T') == link.count('-Map') == link.count('-o') == 1
assert str(control / 'token.o') in link
assert Path(compile_token[compile_token.index('-c') + 1]).name == 'blackhole_qwen38_token_root.cpp'
compile_token[compile_token.index('-c') + 1] = str(source / 'blackhole_qwen38_token_root.cpp')
for i, value in enumerate(compile_token[:-1]):
    if value == '-include':
        compile_token[i + 1] = str(source / Path(compile_token[i + 1]).name)
link[link.index('-T') + 1] = str(source / 'blackhole_qwen38_global_chain.ld')
commands = []


def run(command):
    command = list(map(str, command))
    commands.append(command)
    subprocess.run(command, check=True, cwd=out)


def build_variant(name, extra=()):
    obj, elf = out / (name + '.o'), out / (name + '.elf')
    command = [compile_token[0], *extra, *compile_token[1:]]
    command[command.index('-o') + 1] = str(obj)
    run(command)
    command = link.copy()
    command[command.index(str(control / 'token.o'))] = str(obj)
    command[command.index('-o') + 1] = str(elf)
    command[command.index('-Map') + 1] = str(out / (name + '.map'))
    run(command)
    return obj, elf


baseline, original = build_variant('control')
assert original.read_bytes() == old['program.elf'], 'unchanged linked ROOT not reproduced'
assert baseline.read_bytes() == (control / 'token.o').read_bytes(), 'unchanged token object not reproduced'
selector = '-DPIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE=1'
serial_selector = '-DPIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE=1'
serial = out / 'serial'
serial.mkdir(mode=0o700)
for name, option, directory in (('candidate', selector, out), ('serial', serial_selector, serial)):
    obj, elf = build_variant(name, (option,))
    assert not subprocess.check_output([tool + 'nm', '-u', str(elf)]).strip()
    symbols = subprocess.check_output([tool + 'nm', '-nC', str(elf)], text=True)
    assert ('GlobalChain::HeadRegisterBatch(' in symbols) == (name == 'candidate')
    assert ('PioneerQ38HeadCsrBatch::Verify<' in symbols) == (name == 'candidate')
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
    for symbol, address in (('_start', 0x6000), ('scalar_entry', 0x40000),
                            ('scalar_fault_park', 0x60000), ('q38_global_chain_stopped', 0x60004)):
        assert re.search(f'^{address:08x} T {symbol}$', symbols, re.M), symbol
    for section, limit in (('boot', 4), ('text', 0x20000), ('terminal', 8), ('conv', 0x8000)):
        path = directory / (section + '.bin')
        run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, path])
        data = path.read_bytes()
        assert data and len(data) <= limit and not len(data) % 4
        if section in ('boot', 'terminal'):
            assert data == old[path.name]
        print(name, section, len(data), hashlib.sha256(data).hexdigest())
    frames = [row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 2048 for row in frames)
    (directory / 'program.symbols').write_text(symbols)
    print(elf.name, hashlib.sha256(elf.read_bytes()).hexdigest(), flush=True)
before_compile = compile_token[:compile_token.index('-c')]
refusals = [
    (option, '-D' + forbidden + '=1') for option in (selector, serial_selector)
    for forbidden in ('PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
                      'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE')]
refusals += [(selector, serial_selector), ('-DPIONEER_Q38_HEAD_CSR_COMPACT_HELPERS=1',)]
for flags in refusals:
    trial = subprocess.run([*before_compile, *flags, '-E',
                            str(source / 'blackhole_qwen38_token_root.cpp')],
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert trial.returncode and ('Queued head CSR check' in trial.stderr or
                                 'Head helper size selection is private' in trial.stderr), (flags, trial.stderr)
(out / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
print('PASS BUILD_ONLY original ROOT byte-exact, matched serial and queued images fit, '
      '10 role refusals; no native or device authority')
