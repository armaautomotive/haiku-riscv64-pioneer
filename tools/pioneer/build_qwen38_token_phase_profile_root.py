#!/usr/bin/env python3
"""Full current ROOT phase timing BUILD only; no native/Work/launch grant."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin', 130892, '0f70850875df17399e233103aa9dd1bee5d5fa249a040bfdb6eb37c46962ad71'),
    ('terminal.bin', 8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin', 32704, '32570f96157e3bb28ea072a04b4ca04b09131c22eaed29af45b3ca95d49d9209'),
    ('program.elf', 191928, '91f389bb72e04019816a3f66c34a4ba87624f0a58f693e362e190d84ccd17bdf'),
)  # Independent whole BUILD identity, not numerical/native/Work authority.
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
    sys.argv = [str(here / 'build_qwen38_batched_output_closure_root.py'), str(sdk), str(out / 'unchanged-output-closure')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
assert hashlib.sha256((out / 'unchanged-output-closure/program.elf').read_bytes()).hexdigest() == \
    'c17658fd62d837c13068759e65b164a1eeffe4abc44c536187882215e917f349'
tool = parent['tool']


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


commands = {}
for kind, old in zip(('recurrent', 'dense', 'token'), parent['objects']):
    command = (out / 'unchanged-output-closure' / ('compile-' + kind + '-argv.txt')).read_text().splitlines()
    assert command[0] == tool + 'g++' and command.count('-c') == command.count('-o') == 1
    command[command.index('-c') + 1] = str(here / pathlib.Path(command[command.index('-c') + 1]).name)
    command[command.index('-o') + 1] = str(out / ('unchanged-' + kind + '.o'))
    run(*command)
    assert pathlib.Path(command[command.index('-o') + 1]).read_bytes() == old.read_bytes(), \
        'ENTIRE old ROOT TU must reproduce FIRST'
    commands[kind] = command

command = commands['token'][:]
command.insert(1, '-DPIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE=1')
token = out / 'phase-token.o'
command[command.index('-o') + 1] = str(token)
run(*command)
(out / 'compile-token-argv.txt').write_text('\n'.join(command) + '\n')
helper = out / 'phase-helper.o'
command[command.index('-c') + 1] = str(here / 'blackhole_qwen38_token_phase_profile.cpp')
command[command.index('-o') + 1] = str(helper)
command[command.index('-O2')] = '-Os'
run(*command)
(out / 'compile-helper-argv.txt').write_text('\n'.join(command) + '\n')
for obj in (token, helper):
    frames = [int(row.split('\t')[1]) for row in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and max(frames) <= 4096
elf = out / 'program.elf'
run(tool + 'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', here / 'blackhole_qwen38_global_chain.ld',
    parent['base']['entry_object'], token, helper, parent['capsule'],
    parent['components'] / 'unchanged-feedback/feedback.o', '-Map', out / 'program.map', '-o', elf)
assert not subprocess.check_output([tool + 'nm', '-u', str(elf)], text=True).strip()
symbols = subprocess.check_output([tool + 'nm', '-n', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name, address in (('_start', 0x6000), ('scalar_entry', 0x40000),
                      ('scalar_fault_park', 0x60000), ('q38_global_chain_stopped', 0x60004)):
    assert re.search('^' + format(address, '08x') + ' T ' + name + '$', symbols, re.M)
disassembly = subprocess.check_output([tool + 'objdump', '-d', str(elf)], text=True)
intervals = {}
for row in map(str.split, subprocess.check_output([tool + 'objdump', '-t', str(elf)], text=True).splitlines()):
    if len(row) == 6 and row[2] == 'F' and int(row[4], 16):
        first, size = int(row[0], 16), int(row[4], 16)
        assert not (first | size) & 3
        assert 0x40000 <= first < first + size <= 0x60000 or 0x64000 <= first < first + size <= 0x6c000
        intervals.setdefault((first, first + size), set()).add(row[5])
last = 0
for first, end in sorted(intervals):
    assert first >= last, 'different actual function intervals must not overlap'
    last = end
instructions, csr, calls = [], [], {}
for match in re.finditer(r'^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s.*$', disassembly, re.M):
    owner = [names for (first, last), names in intervals.items() if first <= int(match[1], 16) < last]
    if not owner:
        continue
    assert len(owner) == 1
    word = int(match[2], 16)
    assert word & 3 == 3
    assert not (word & 127 == 0x67 and not ((word >> 15) & 31))
    if word & 127 == 0x73:
        assert owner == [{'q38_token_phase_clock'}]
        assert (word >> 12) & 7 == 2 and not ((word >> 15) & 31) and (word >> 7) & 31
        csr.append(word >> 20)
    call = re.search(r'\bjalr\b.*# [0-9a-f]+ <(q38_token_phase_[a-z]+)>', match[0])
    if call:
        assert len(owner[0]) == 1
        key = (next(iter(owner[0])), call[1])
        calls[key] = calls.get(key, 0) + 1
    instructions.append(match[0])
assert csr == [0xb80, 0xb00, 0xb80], csr
assert calls == {
    ('q38_global_chain_main', 'q38_token_phase_begin'): 1,
    ('q38_global_chain_main', 'q38_token_phase_mark'): 3,
    ('_ZN11GlobalChainL6FinishEPKjj', 'q38_token_phase_mark'): 1,
    ('_ZN11GlobalChainL6FinishEPKjj', 'q38_token_phase_complete'): 1,
    ('_ZN11GlobalChainL7RunHeadEv', 'q38_token_phase_mark'): 9,
    ('q38_token_phase_begin', 'q38_token_phase_clock'): 1,
    ('q38_token_phase_mark', 'q38_token_phase_clock'): 1,
    ('q38_token_phase_complete', 'q38_token_phase_clock'): 1,
}, calls  # Emitted sites ONLY, not proof that a whole ROOT history executes.
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',
                     '\n'.join(instructions))
(out / 'program.symbols').write_text(symbols)
(out / 'program.disassembly').write_text(disassembly)
observed = []
for section in ('boot', 'text', 'terminal', 'conv'):
    run(tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, out / (section + '.bin'))
for name in ('boot.bin', 'text.bin', 'terminal.bin', 'conv.bin', 'program.elf'):
    data = (out / name).read_bytes()
    row = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(row)
    print(row, flush=True)
for name in ('boot.bin', 'terminal.bin'):
    assert (out / name).read_bytes() == (out / 'unchanged-output-closure' / name).read_bytes()
assert len((out / 'text.bin').read_bytes()) <= 131072 and len((out / 'conv.bin').read_bytes()) <= 32768
if not inspect:
    assert tuple(observed) == PINS
print('BUILD_ONLY full current ROOT phase timing/whole c176+ancestors+ALL3 unchanged TUs FIRST/'
      'only3 readCSR opcodes/no new ROM or stack extent;NO execution,native,physical,Work or TPS grant', flush=True)
