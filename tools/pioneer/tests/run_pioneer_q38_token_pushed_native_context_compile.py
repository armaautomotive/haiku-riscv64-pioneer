#!/usr/bin/env python3
"""Compile/inspect actual private G3 bodies. NEVER link/install/invoke."""
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
    sys.argv = [str(here/'run_pioneer_q38_token_precise_native_context_compile.py'), str(qualified), str(out/'legacy-controls')]
    control = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
args = [value for value in control['args'] if not value.startswith(('-DPIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT=', '-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS='))]
tags = [i for i, value in enumerate(args) if value.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1 and args[tags[0]] == '-DPIONEER_DRIVER_BUILD_TAG=0x5154324e4f445256ULL'
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x5154334e4f445256ULL'
obj = out/'pushed-token-context-NODRV.o'
args[args.index('-c')+1] = str(here/'pioneer_q38_token_pushed_native_context_compile.cpp')
args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True)
frames = control['control']['base']['frames'](obj.with_suffix('.su'))
assert frames
for name, size in frames.items():
    assert size <= 2048 or name in control['frames'] and size <= control['frames'][name], (name, size)
for name in control['control']['required']:
    assert any(name in emitted for emitted in frames), (name, 'actual native implementation must be emitted')
token_frames = [size for name, size in frames.items() if 'Token' in name or 'pioneer_token_context_' in name]
assert token_frames and max(token_frames) <= 2048
tool = control['tool']
table = subprocess.check_output([tool+'nm', '-S', '--defined-only', str(obj)], text=True)
undefined = subprocess.check_output([tool+'nm', '-u', str(obj)], text=True)
symbols = {row[-1]:(int(row[0], 16), int(row[1], 16), row[2])
           for row in map(str.split, table.splitlines()) if len(row) == 4}
normal = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
assert not normal & symbols.keys() and all('pioneer_q38_token_context_body_'+name in symbols for name in normal)
assert len([value for name, value in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and value[1] == 2097152]) == 1
assert not any('PioneerQ38TokenProgramsL' in name for name in symbols), 'no legacy program arrays alongside the selected G3 family'
sections = subprocess.check_output([tool+'objdump', '-t', str(obj)], text=True)
section = {row[-1]:row[-3] for row in map(str.split, sections.splitlines()) if len(row) >= 6 and row[-1] in symbols}
header = (here.parent/'PioneerQ38TokenPushedPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == '692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f'
expected = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(expected) == 13
for digest, label in expected:
    size = int(re.search(r'static_assert\(sizeof\('+label+r'\)==([0-9]+),', header.decode())[1])
    matches = [(name, value) for name, value in symbols.items()
               if 'PioneerQ38TokenPushedPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(matches) == 1 and matches[0][1][1] == size, (label, matches)
    name, (start, _, _) = matches[0]
    raw = out/(label+'.raw')
    subprocess.run([tool+'objcopy', '-O', 'binary', '-j', section[name], str(obj), str(raw)], check=True)
    data = raw.read_bytes()
    assert start+size <= len(data) and hashlib.sha256(data[start:start+size]).hexdigest() == digest, label
for forbidden in ('ReferenceProducts', 'CompareFinal', 'OriginalSource', 'NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace', 'CheckNativeFullEndFlow', 'Q38NativeStateTest'):
    assert forbidden not in table and forbidden not in undefined, forbidden
assert not re.search(r'\b(?:cosf?|sinf?|powf?)\b', undefined)

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
    return [value for _, value in selected]

for name, kind in (('init_hardware', 'refusal'), ('init_driver', 'refusal'),
                   ('uninit_driver', 'inert'), ('publish_devices', 'null'), ('find_device', 'null')):
    assert words('pioneer_q38_token_context_only_'+name) == words('pioneer_token_context_expected_'+kind)
    assert words('pioneer_q38_token_context_body_'+name) == words('pioneer_token_context_expected_'+kind), ('actual renamed entry is not inert', name)
print('PASS ACTUAL G3 private native body/ALL21 parents+QT3NODRV/actual53.9MB journal+55-word boot/all13 compiled arrays SHA-exact/2MiB RoPE/renamed ABI+ACTUAL5 body entries and5 wrappers byte-identical to independent inert controls/maxIndividualTokenFrame'+str(max(token_frames)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' pushed-token-context-NODRV.o;COMPILE ONLY,never link/install/invoke;full-model/native runtime,combined stack,lifecycle,Work and physical TPS still owed')
