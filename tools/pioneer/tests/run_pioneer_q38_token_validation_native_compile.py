#!/usr/bin/env python3
"""Compile/inspect tag74 and its qualified NODRV controls. NEVER link/install.

This object has actual conventional driver entries. It is NOT deployment
authority: full-model runtime, combined stack and safe lifecycle remain gates.
"""
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
project = here.parents[2]
saved = sys.argv[:]
try:
    sys.argv = [str(here/'run_pioneer_q38_token_pushed_native_context_compile.py'), str(qualified), str(out/'nodrv-controls')]
    control = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
nodrv = out/'nodrv-controls/pushed-token-context-NODRV.o'
# Intentional current ROOT boot/receipt/code seal;independent main/clean SDK builds
# agree. Firmware arrays,old65 and inert ABI controls remain separately exact.
assert hashlib.sha256(nodrv.read_bytes()).hexdigest() == 'c78a84c7ec8f74c0f73f74e67579dfa94bcc71399145508fcb062619844c9b80'
args = [a for a in control['args'] if not a.startswith('-DPIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY=')]
role = (project/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenValidationRole.h').read_text()
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(required) == 23
for name in sorted(required):
    selected = [a for a in args if a == '-D'+name or a.startswith('-D'+name+'=')]
    assert not selected or selected == ['-D'+name] or selected == ['-D'+name+'=1'], (name, selected)
    if not selected:
        args.append('-D'+name+'=1')
tags = [i for i, value in enumerate(args) if value.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1 and args[tags[0]] == '-DPIONEER_DRIVER_BUILD_TAG=0x5154334e4f445256ULL'
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026103000010074ULL'
obj = out/'token-validation-tag74-INSPECT-ONLY.o'
args[args.index('-c')+1] = str(project/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp')
args[args.index('-o')+1] = str(obj)
assert '-shared' not in args and '-c' in args
subprocess.run(args, check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
tool = control['tool']
table = subprocess.check_output([tool+'nm', '-S', '--defined-only', str(obj)], text=True)
undefined = subprocess.check_output([tool+'nm', '-u', str(obj)], text=True)
symbols = {row[-1]:(int(row[0], 16), int(row[1], 16), row[2])
           for row in map(str.split, table.splitlines()) if len(row) == 4}
normal = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
assert normal <= symbols.keys()
assert not any('pioneer_q38_token_context_' in name or 'pioneer_token_context_' in name for name in symbols)
for name in normal-{'api_version'}:
    assert symbols[name][1] > 8 and symbols[name][2] == 'T', (name, 'actual conventional body must not be inert')
for name in ('Q38TokenNativeControl', 'BeginQ38TokenNativeBackendScoped', 'FinishQ38TokenNativeBackendScoped', 'ProbeControl'):
    matches = [info for key, info in symbols.items() if name in key]
    assert matches and max(info[1] for info in matches) > 32, (name, 'real reachable native implementation required')
assert len([value for name, value in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and value[1] == 2097152]) == 1
assert not any('PioneerQ38TokenProgramsL' in name for name in symbols)
sections = subprocess.check_output([tool+'objdump', '-t', str(obj)], text=True)
section = {row[-1]:row[-3] for row in map(str.split, sections.splitlines()) if len(row) >= 6 and row[-1] in symbols}
# A large emitted private handler alone is not proof it is reachable from
# the actual public entry. Require its relocation INSIDE ProbeControl.
route = [(name, info) for name, info in symbols.items() if name == '_ZL12ProbeControlPvjS_m']
assert len(route) == 1
name, (start, size, _) = route[0]
relocations = subprocess.check_output([tool+'objdump', '-r', '-j', section[name], str(obj)], text=True)
assert any(start <= int(match[1], 16) < start+size
           and match[2] == '_ZL21Q38TokenNativeControlP18PioneerProbeCookiePvm'
           for line in relocations.splitlines()
           for match in [re.match(r'^([0-9a-f]+)\s+R_RISCV_CALL_PLT\s+(\S+)', line)] if match)
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
for forbidden in ('Q38WorkControl', 'ReferenceProducts', 'CompareFinal', 'OriginalSource', 'NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace', 'CheckNativeFullEndFlow', 'Q38NativeStateTest',
                  'Q38InstructionStackObserver', 'TokenControlTestCopy'):
    assert forbidden not in table and forbidden not in undefined, forbidden
assert not re.search(r'\b(?:cosf?|sinf?|powf?)\b', undefined)
frames = control['control']['control']['base']['frames'](obj.with_suffix('.su'))
old_physical_frames = control['control']['control']['base']['previous']
inherited_large_frames = 0
for name, size in frames.items():
    if size > 2048:
        # The private NODRV entry is inert and never emits the normal
        # hardware-init probe. Compare that existing path with the actual
        # authenticated old65 physical-body compile, not an absent frame.
        assert (name in control['frames'] and size <= control['frames'][name]
                or name in old_physical_frames and size <= old_physical_frames[name]), (name, size)
        inherited_large_frames += 1
token_frames = [size for name, size in frames.items() if 'Token' in name]
assert token_frames and max(token_frames) <= 2048
print('PASS actual tag74 conventional driver object/bounded validation control reachable/all13 G3 arrays SHA-exact/2MiB RoPE/no CPU oracle+Work+TEST imports/qualified byte-exact G3 NODRV controls/maxIndividualTokenFrame'+str(max(token_frames))+'/inheritedLargeFrames'+str(inherited_large_frames)+' bounded by authenticated old physical-body controls')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' token-validation-tag74-INSPECT-ONLY.o;COMPILE+INSPECT ONLY,NEVER link/install/invoke;full-model numerical/lifecycle/combined stack/physical/TPS qualification STILL OWED')
