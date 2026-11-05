#!/usr/bin/env python3
"""Pair qualified prepared ROOT/worker code DATA; never device access."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from generate_qwen38_token_programs import authenticated, pins

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Refuse optimized Python; whole paired firmware checks required')
args=sys.argv[1:];counter_unroll='--counter-unroll' in args
if counter_unroll:
    assert args.count('--counter-unroll')==1;args.remove('--counter-unroll')
phase_profile='--phase-profile' in args
if phase_profile:
    assert counter_unroll and args.count('--phase-profile')==1;args.remove('--phase-profile')
two_product_replay='--two-product-replay' in args
if two_product_replay:
    assert phase_profile and args.count('--two-product-replay')==1;args.remove('--two-product-replay')
assert len(args) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root, worker, output = map(pathlib.Path, args)
assert all(p.is_absolute() for p in (root, worker, output)) and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
elf = (root / 'program.elf').read_bytes()
assert (len(elf),hashlib.sha256(elf).hexdigest()) == ((198204,'74cae45049174dd53c96203104a0b4b501f757b2210b2de86da5507334ebacc5') if phase_profile else (197724,'38a8f64f9bf75946953d87ec08d3bba07b44ea36303936ab912c89771f68d039') if counter_unroll else (194980,'0306100598ee1aa4e20bf3399165f5fbc2714ab2a694021fa98d63f4f64bb8f1'))
sections = {}
with tempfile.TemporaryDirectory(prefix='pioneer-prepared-programs-', dir='/private/tmp') as temporary:
    private = pathlib.Path(temporary)
    (private / 'program.elf').write_bytes(elf)
    for name in ('boot', 'text', 'terminal', 'conv', 'prepared'):
        target = private / (name + '.bin')
        subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + name, str(private / 'program.elf'), str(target)], check=True)
        data = target.read_bytes()
        assert data and not len(data) % 4 and data == (root / target.name).read_bytes()
        sections[name] = data
peer = authenticated(worker, pins(here / ('build_qwen38_two_product_q4_worker.py' if two_product_replay else 'build_qwen38_prepared_exact_q4_worker.py'), 'REPLAY_PINS' if two_product_replay else 'PINS'))
source = (here / 'PioneerQ38FullOffloadBatchedPrograms.h').read_bytes()
assert hashlib.sha256(source).hexdigest() == '91624761b95d281cb9dd4493790b27a1c5d88d7c327d6df894711561b76b4c84'
text = source.decode('ascii')


def array(label, data):
    words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
    body = '// SHA256 ' + hashlib.sha256(data).hexdigest() + '\nstatic const uint32_t ' + label + '[]={\n'
    body += '\n'.join(' ' + ','.join('0x%08xU' % w for w in words[i:i+8]) + ',' for i in range(0, len(words), 8))
    return body + '\n};\nstatic_assert(sizeof(' + label + ')==' + str(len(data)) + ',"exact complete ' + label + ' DATA");'


mapping = {'RootBoot': sections['boot'], 'RootText': sections['text'], 'RootTerminal': sections['terminal'], 'RootAux': sections['conv']}
mapping.update({label: peer[name + '.bin'] for label, name in {
    'PeerBoot': 'boot', 'PeerFront': 'globalworker', 'PeerPrecise': 'precise', 'PeerDense': 'dense', 'PeerFfn': 'ffn',
    'PeerService': 'service', 'PeerFetch': 'fetch', 'PeerAffine': 'affine', 'PeerText': 'text', 'PeerGdn': 'gdn'}.items()})
for label, data in mapping.items():
    pattern = r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ' + label + r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\(' + label + r'\)==[0-9]+,"exact complete ' + label + r' DATA"\);'
    if label in ('RootBoot', 'RootTerminal'):
        old = re.search(pattern, text, re.S)
        assert old and hashlib.sha256(data).hexdigest() in old[0], 'unchanged boot/STOP bytes'
    text, count = re.subn(pattern, lambda _: array(label, data), text, flags=re.S)
    assert count == 1
text = text.replace('#include "PioneerQ38FullOffloadBatchedProgramsRole.h"', '#include "PioneerQ38FullOffloadPreparedQ4ProgramsRole.h"')
text = text.replace('PIONEER_Q38_FULL_OFFLOAD_BATCHED_PROGRAMS_H', 'PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H')
text = text.replace('PioneerQ38FullOffloadBatchedPrograms', 'PioneerQ38FullOffloadPreparedQ4Programs')
anchor = '// SHA256 ' + hashlib.sha256(mapping['PeerBoot']).hexdigest() + '\nstatic const uint32_t PeerBoot[]={'
assert text.count(anchor) == 1
text = text.replace(anchor, array('RootPrepared', sections['prepared']) + '\n' + anchor)
text = text.replace('static inline unsigned CodeSpans(unsigned worker){return worker>=120?0:worker?10:4;}',
                    'static inline unsigned CodeSpans(unsigned worker){return worker>=120?0:worker?10:5;}')
for old, new in (
    ('case 3:return {0x64000,unsigned(sizeof(RootAux)/4)};',
     'case 3:return {0x64000,unsigned(sizeof(RootAux)/4)};\n  case 4:return {0x6c000,unsigned(sizeof(RootPrepared)/4)};'),
    ('case 3:data=RootAux;break;', 'case 3:data=RootAux;break;\n   case 4:data=RootPrepared;break;'),
    (' if(target>=reinterpret_cast<uintptr_t>(PeerBoot)',
     ' if(target>=reinterpret_cast<uintptr_t>(RootPrepared) && target-reinterpret_cast<uintptr_t>(RootPrepared)<sizeof(RootPrepared))return false;\n if(target>=reinterpret_cast<uintptr_t>(PeerBoot)')):
    assert text.count(old) == 1
    text = text.replace(old, new)
anchor = 'enum {Family=0x33505451,Version=1,CancelBoundary=1,EogCount=5};'
assert text.count(anchor) == 1
text = text.replace(anchor, anchor + '\nenum {PreparedQ4=1,NativeQ4Exact=1,RootSections=5,PreparedCodeAddress=0x6c000};')
text = text.replace('Generated complete five-EOG ROOT and ten byte-exact returning-wide peer sections.',
                    'Generated executed prepared ROOT and exact prepared Q4 workers:five+ten sections.')
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ', text)) == 15
assert len(sections['prepared']) <= 8192 and len(peer['service.bin']) <= 24576
if counter_unroll:
    text=text.replace('PioneerQ38FullOffloadPreparedQ4','PioneerQ38FullOffloadPreparedCounterQ4')
    text=text.replace('PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_PREPARED_COUNTER_Q4_PROGRAMS_H')
    text=text.replace('enum {PreparedQ4=1,','enum {CounterUnroll=1,PreparedQ4=1,')
if phase_profile:
    text=text.replace('PioneerQ38FullOffloadPreparedCounterQ4','PioneerQ38FullOffloadPreparedProfileQ4')
    text=text.replace('PIONEER_Q38_FULL_OFFLOAD_PREPARED_COUNTER_Q4_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H')
    text=text.replace('enum {CounterUnroll=1,','enum {TokenPhaseProfile=1,PhaseProfileAddress=0x75710,PhaseProfileWords=60};\nenum {CounterUnroll=1,')
if two_product_replay:
    text=text.replace('PioneerQ38FullOffloadPreparedProfileQ4','PioneerQ38FullOffloadTwoProductReplayQ4')
    text=text.replace('PIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H','PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY_Q4_PROGRAMS_H')
    text=text.replace('enum {CounterUnroll=1,','enum {TwoProductQ4=1,AffineReplayWords=13};\nenum {CounterUnroll=1,')
    text=text.replace('Generated executed prepared ROOT and exact prepared Q4 workers:five+ten sections.',
                      'Generated qualified ROOT and exact replay13 two-product Q4 worker:five+ten sections.')
with output.open('x', encoding='ascii') as stream:
    stream.write(text)
print(hashlib.sha256(output.read_bytes()).hexdigest(), output.name, flush=True)
print('PAIRED_CODE_DATA_ONLY fifteen sections/extra ROOT6c000..6e000 journal REQUIRED;NO source owner,installation or speed grant', flush=True)
