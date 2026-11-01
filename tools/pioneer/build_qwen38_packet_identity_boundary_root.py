#!/usr/bin/env python3
"""Isolated private-identity boundary BUILD;NO native/device/Work permission."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin', 130344, 'f9275e1c9cf31a9508522c5bab9a7645e8886e842cc664ad51a40edb4cc357e0'),
    ('terminal.bin', 8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin', 32684, '0756a64a8cfffd923ab5af14957a23e32aa8ca80304693f2350d23b340f660e6'),
    ('program.elf', 190760, '0c3f72d469164830670f5d880fe5c2f5df9a95609b09325dcddc1bea58c62e10'),
)  # Independently identical BUILD only;not native/device/Work authority.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_cohort_local_sources_root.py'), str(sdk), str(out / 'unchanged-cohort')]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
# ENTIRE cd54/83677/48b2/951 controls reproduce FIRST. Only returning
# recurrent packet/full-seal identity policy changes;worker math stays old.
old, commit = base['old'], base['base']
tool, components = base['tool'], base['components']


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


common = [*base['common'], '-DPIONEER_Q38_COHORT_LOCAL_SOURCES_IMAGE=1']
control = out / 'unchanged-cohort-recurrent.o'
run(*common, '-c', here / 'blackhole_qwen38_return_recurrent_layer.cpp', '-o', control)
assert control.read_bytes() == base['obj'].read_bytes()
obj = out / 'packet-identity-boundary-recurrent.o'
command = [*common, '-DPIONEER_Q38_PACKET_IDENTITY_BOUNDARY_IMAGE=1', '-c',
           str(here / 'blackhole_qwen38_return_recurrent_layer.cpp'), '-o', str(obj)]
run(*command)
(out / 'compile-argv.txt').write_text('\n'.join(command) + '\n')
assert obj.read_bytes() != control.read_bytes()
frames = [int(row.split('\t')[1]) for row in obj.with_suffix('.su').read_text().splitlines()]
assert frames and max(frames) <= 4096, max(frames)
raw, capsule = out / 'components-raw.o', out / 'components-NOBOOT.o'
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '-r', '--gc-sections',
    '-u', 'q38_return_recurrent_layer', '-u', 'q38_return_dense_layer',
    '-u', 'q38_global_chain_learned_rms', '-u', 'q38_global_chain_quantize14',
    obj, components / 'dense.o', components / 'unchanged-feedback/root_conv_sections.o', '-o', raw)
dead = ('mixed_recurrent_block_wait', 'mixed_recurrent_block_stopped',
        'mixed_live_block_wait', 'mixed_live_block_stopped')
relocations = subprocess.check_output([tool + 'objdump', '-r', str(raw)], text=True)
for name in dead:
    assert not re.search(r'\b' + name + r'\b', relocations), name
run(tool + 'objcopy', *['--strip-symbol=' + name for name in dead], raw, capsule)
assert subprocess.check_output([tool + 'nm', '-u', str(capsule)], text=True).split() == ['U', 'scalar_fault_park']
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections',
    '-T', here / 'blackhole_qwen38_global_chain.ld',
    old['out'] / 'unchanged-frontier/start.o', commit['obj'], capsule,
    components / 'unchanged-feedback/feedback.o', '-Map', out / 'program.map', '-o', out / 'program.elf')
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
             'MixedRecurrentCohort::StageGdnAfterClosedConv(', 'RecurrentBlock::PacketIdentity(',
             'q38_layer_return_recurrent_packet_seal', 'q38_layer_return_recurrent_seal', 'q38_token_fault_report'):
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
    assert (out / name).read_bytes() == (base['out'] / name).read_bytes()
assert len((out / 'text.bin').read_bytes()) <= 131072 and len((out / 'conv.bin').read_bytes()) <= 32768
if not inspect:
    assert tuple(observed) == PINS, 'candidate not independently frozen'
print('BUILD_ONLY duplicate24 private frame/boot identity comparisons move to EVERY full audit;'
      'packet pointer/phase/request/doorbell+ALL16 live source checks retained;'
      'NoC command/counter/queue/job/worker guards and math unchanged;'
      'metadata fault detection may defer to release/compute boundary;NO cached ready,native,physical,Work or TPS grant', flush=True)
