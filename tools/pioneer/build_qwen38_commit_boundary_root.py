#!/usr/bin/env python3
"""Isolated commit-copy numeric-seal BUILD;NO DATA/native/device authority."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin', 130936, 'ab6c14eb6935613c70b08d879cf3f435d945175ab4279a5e065a989eb6b95739'),
    ('terminal.bin', 8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin', 32684, '0cbd70719967cbc43b1353186cad5f422a9da045bdc1d51efb88a0e775d52e99'),
    ('program.elf', 190704, '83677f78f548b8fc11d874a858b670310d1dad859982e737c674b2ff7329db00'),
)  # Independent BUILD identity ONLY;not runtime/native/physical authority.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or PINS)
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_retained_cohort_root.py'), str(sdk), str(out / 'unchanged-retained')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
# ENTIRE frozen48b2/9515589d controls reproduce from source FIRST.
tool = old['tool']
defines = ['PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK',
           'PIONEER_Q38_TOKEN_FRONTIER_IMAGE', 'PIONEER_Q38_JOB_BOOT_FOLD_IMAGE',
           'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION', 'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
           'PIONEER_Q38_TOKEN_CANCEL_BOUNDARY']
common = [tool + 'g++', *old['flags'], *['-D' + key + '=1' for key in defines]]


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


control = out / 'unchanged-root.o'
run(*common, '-c', here / 'blackhole_qwen38_token_root.cpp', '-o', control)
assert control.read_bytes() == (old['parent']['out'] / 'program.o').read_bytes()
obj = out / 'commit-boundary-root.o'
command = [*common, '-DPIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE=1',
           '-c', str(here / 'blackhole_qwen38_token_root.cpp'), '-o', str(obj)]
run(*command)
(out / 'compile-argv.txt').write_text('\n'.join(command) + '\n')
assert obj.read_bytes() != control.read_bytes(), 'policy must change actual emitted ROOT'
frames = [int(line.split('\t')[1]) for line in obj.with_suffix('.su').read_text().splitlines()]
assert frames and max(frames) <= 4096, max(frames)
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections',
    '-T', here / 'blackhole_qwen38_global_chain.ld',
    old['out'] / 'unchanged-frontier/start.o', obj, old['capsule'],
    old['components'] / 'unchanged-feedback/feedback.o', '-Map', out / 'program.map', '-o', out / 'program.elf')
elf = out / 'program.elf'
assert not subprocess.check_output([tool + 'nm', '-u', str(elf)], text=True).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(elf)], text=True)
demangled = subprocess.check_output([tool + 'nm', '-nC', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name, address in (('_start', 0x6000), ('scalar_entry', 0x40000),
                      ('scalar_fault_park', 0x60000), ('q38_global_chain_stopped', 0x60004)):
    assert re.search('^' + format(address, '08x') + ' T ' + name + '$', symbols, re.M), name
for name in ('GlobalChain::CommitState(', 'GlobalChain::StateSeal(', 'GlobalChain::InputSeal(',
             'GlobalChain::FeedbackSeal(', 'GlobalChain::PublishFrontier(',
             'MixedRecurrentCohort::StageGdnAfterClosedConv(', 'q38_token_fault_report'):
    assert name in demangled, name
disassembly = subprocess.check_output([tool + 'objdump', '-d', str(elf)], text=True)
intervals = []
for line in subprocess.check_output([tool + 'objdump', '-t', str(elf)], text=True).splitlines():
    row = line.split()
    if len(row) == 6 and row[2] == 'F' and int(row[4], 16):
        start, size = int(row[0], 16), int(row[4], 16)
        assert not (start | size) & 3
        assert 0x40000 <= start < start + size <= 0x60000 or 0x64000 <= start < start + size <= 0x6c000
        intervals.append((start, start + size))
instructions = []
for line in disassembly.splitlines():
    match = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s', line)
    if not match or not any(a <= int(match[1], 16) and int(match[1], 16) + 4 <= b for a, b in intervals):
        continue
    word = int(match[2], 16)
    assert word & 3 == 3 and word & 127 != 0x73, 'existing32bit RV32IMF only,noCSR'
    assert not (word & 127 == 0x67 and not ((word >> 15) & 31)), 'no absolute JALR'
    instructions.append(line)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',
                     '\n'.join(instructions))
assert len([line for line in instructions if re.search(r'\bmv\s+s11,\w+\s*$', line)]) == 5
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(disassembly)
for section in ('boot', 'text', 'terminal', 'conv'):
    run(tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, out / (section + '.bin'))
observed = []
for name in ('boot.bin', 'text.bin', 'terminal.bin', 'conv.bin', 'program.elf'):
    data = (out / name).read_bytes()
    row = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(row)
    print(row, flush=True)
for name in ('boot.bin', 'terminal.bin'):
    assert (out / name).read_bytes() == (old['out'] / name).read_bytes()
assert len((out / 'text.bin').read_bytes()) <= 131072 and len((out / 'conv.bin').read_bytes()) <= 32768
if not inspect:
    assert tuple(observed) == PINS, 'candidate not independently frozen'
print('BUILD_ONLY numeric input+feedback scans bracket actual CommitState;ALL per-packet '
      'HeadSeal,descriptor,source,counter,command,bounds+readback checks retained;'
      'geometry proves copy/descriptor/private-stack buffers disjoint from numeric seals;'
      'entire48b2+oldROOT object controls unchanged;NO runtime,native,physical,Work or TPS qualification', flush=True)
