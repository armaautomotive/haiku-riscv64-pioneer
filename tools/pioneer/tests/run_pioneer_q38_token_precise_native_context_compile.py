#!/usr/bin/env python3
"""Compile actual QTP2 native bodies under QT2NODRV. NEVER link/install/run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
qualified, out = map(pathlib.Path, sys.argv[1:])
assert qualified.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here/'run_pioneer_q38_token_native_context_compile.py'), str(qualified), str(out/'legacy-control')]
    control = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
args = control['args'][:]
tag = [i for i, value in enumerate(args) if value.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tag) == 1 and args[tag[0]] == '-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL'
args[tag[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x5154324e4f445256ULL'
args += ['-DPIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT=1', '-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1']
obj = out/'precise-token-context-NODRV.o'
args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True)
frames = control['base']['frames'](obj.with_suffix('.su'))
assert frames
for name, size in frames.items():
    assert size <= 2048 or name in control['frames'] and size <= control['frames'][name], (name, size)
for name in control['required']:
    assert any(name in emitted for emitted in frames), (name, 'actual native body must be emitted')
sizes = [size for name, size in frames.items() if 'Token' in name or 'pioneer_token_context_' in name]
assert sizes and max(sizes) <= 2048
tool = control['tool']
table = subprocess.check_output([tool+'nm', '-S', '--defined-only', str(obj)], text=True)
undefined = subprocess.check_output([tool+'nm', '-u', str(obj)], text=True)
symbols = {row[-1]:(int(row[0], 16), int(row[1], 16), row[2])
           for row in map(str.split, table.splitlines()) if len(row) == 4}
normal = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
assert not normal & symbols.keys()
assert all('pioneer_q38_token_context_body_'+name in symbols for name in normal)
assert len([value for name, value in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and value[1] == 2097152]) == 1
expected = {'RootBoot': 4, 'RootText': 127880, 'RootTerminal': 8, 'RootAux': 32176,
            'PeerBoot': 4, 'PeerFront': 7124, 'PeerPrecise': 8392, 'PeerDense': 10332,
            'PeerFfn': 16456, 'PeerService': 9888, 'PeerAffine': 5360, 'PeerText': 5984, 'PeerGdn': 35564}
for label, size in expected.items():
    matches = [value for name, value in symbols.items()
               if 'PioneerQ38TokenPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(matches) == 1 and matches[0][1] == size, (label, matches)
for forbidden in ('ReferenceProducts', 'CompareFinal', 'OriginalSource', 'NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu', 'Q38NativeTraceTest', 'PioneerQ38ObservedTrace',
                  'CheckNativeFullEndFlow', 'Q38NativeStateTest', 'Q38MixedLiveValidationControl'):
    assert forbidden not in table and forbidden not in undefined, forbidden
assert not re.search(r'\b(?:cosf?|sinf?|powf?)\b', undefined)
# The actual conventional bodies remain renamed and independently inert.
# Inert controls contain no relocations;compare the literal instructions,
# not source comments or mere exported function presence.
sections = subprocess.check_output([tool+'objdump', '-t', str(obj)], text=True)
section = {row[-1]:row[-3] for row in map(str.split, sections.splitlines()) if len(row) >= 6 and row[-1] in symbols}


def words(name):
    start, size, kind = symbols[name]
    assert size and kind in ('T', 't')
    dis = subprocess.check_output([tool+'objdump', '-d', '-j', section[name], str(obj)], text=True)
    selected = [(int(match[1], 16), match[2]) for line in dis.splitlines()
                for match in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+', line)]
                if match and start <= int(match[1], 16) < start+size]
    address = start
    for at, value in selected:
        assert at == address
        address += len(value)//2
    assert address == start+size
    rel = subprocess.check_output([tool+'objdump', '-r', '-j', section[name], str(obj)], text=True)
    assert not any(start <= int(match[1], 16) < start+size for line in rel.splitlines()
                   for match in [re.match(r'^([0-9a-f]+)\s+R_', line)] if match)
    return [value for at, value in selected]


for name, kind in (('init_hardware', 'refusal'), ('init_driver', 'refusal'),
                   ('uninit_driver', 'inert'), ('publish_devices', 'null'), ('find_device', 'null')):
    assert words('pioneer_q38_token_context_only_'+name) == words('pioneer_token_context_expected_'+kind)
print('PASS ACTUAL QTP2 native context/ALL21 parents+QT2NODRV+complete QGW2 program and53MB journal/selected boot+status/real scoped native factories emitted/maxIndividualFrame'+str(max(sizes)))
print('PASS ALL13 exact-size program arrays+2MiB rawRoPE/no CPU model-reference or TEST engine imports/renamed conventional ABI+5 independent inert controls;COMPILE ONLY,NOT runtime or Work')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' precise-token-context-NODRV.o;NEVER link/install/invoke;combined stacks,original-model,lifecycle,physical and TPS qualification owed')
