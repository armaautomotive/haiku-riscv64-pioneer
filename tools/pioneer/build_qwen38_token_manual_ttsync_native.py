#!/usr/bin/env python3
"""New manual-completion PRIVATE native BUILD;never install or run a card."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA = '0925bb1287c704b4c8841d6f2b7379dade76d258e852987b07526b1ab168e2c2'  # Independent whole BUILD identity,not lifecycle/launch authority.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 1 and (inspect or PRIVATE_SHA)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
out = pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_token_constant_q4_native.py'), str(out / 'unchanged-constant-q4')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
hooks, tool = parent['hooks'], parent['tool']
assert hooks['sha'](parent['obj']) == '5e2c95a3887920990a3ef0b80cfd0057a68bc95b645f03476c7869d9dd622e63'
obj = out / 'token-manual-ttsync-NODRV.o'
command = parent['command'][:]
command[command.index('-c') + 1] = str(here / 'tests/pioneer_q38_token_manual_ttsync_native_context_compile.cpp')
command[command.index('-o') + 1] = str(obj)
subprocess.run(command, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(command) + '\n')

def payload(path, name, values, mapping, label):
    first, size = values[name]
    target = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', mapping[name], str(path), str(target)], check=True)
    data = target.read_bytes()
    assert first + size <= len(data)
    return data[first:first + size]

before, before_sections = hooks['records'](parent['obj'])
symbols, sections = hooks['records'](obj)
header = (here / 'PioneerQ38TokenManualTTSyncPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == 'eaaa4974db72e416f13b2bdd66a4eb5bcdf4d4243a27c990037eea022e009dd1'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 14
changed = []
for digest, label in arrays:
    names = [n for n in symbols if 'PioneerQ38TokenManualTTSyncPrograms' in n and n.endswith(str(len(label)) + label + 'E')]
    old_names = [n for n in before if 'PioneerQ38TokenConstantQ4Programs' in n and n.endswith(str(len(label)) + label + 'E')]
    assert len(names) == len(old_names) == 1
    current = payload(obj, names[0], symbols, sections, 'new-' + label)
    previous = payload(parent['obj'], old_names[0], before, before_sections, 'old-' + label)
    assert hashlib.sha256(current).hexdigest() == digest
    if current != previous:
        changed.append(label)
assert changed == ['PeerFetch', 'PeerAffine', 'PeerText'], changed
rope = [n for n, (_, size) in symbols.items() if 'PioneerQ38TokenRope' in n and n.endswith('DataE') and size == 2097152]
assert len(rope) == 1 and rope[0] in before
assert payload(obj, rope[0], symbols, sections, 'new-rope') == payload(parent['obj'], rope[0], before, before_sections, 'old-rope')
assert not set(hooks['abi']) & symbols.keys()
for name in ('init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'):
    for prefix in ('pioneer_q38_token_context_body_', 'pioneer_q38_token_context_only_'):
        symbol = prefix + name
        assert payload(obj, symbol, symbols, sections, 'new-' + symbol) == payload(parent['obj'], symbol, before, before_sections, 'old-' + symbol)
table = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(obj)], text=True)
undefined = subprocess.check_output([tool + 'nm', '-u', str(obj)], text=True)
for name in ('PioneerQ38TokenManualTTSyncPrograms', 'PioneerQ38TokenNativeOwner::SelectedSources<',
             'ReadQ38TokenNativePhaseProfile(', 'Q38TokenNativeControl(',
             'BeginQ38TokenNativeBackendScoped(', 'FinishQ38TokenNativeBackendScoped('):
    assert name in table, name
for name in ('PioneerQ38TokenConstantQ4Programs', 'PioneerQ38TokenMatrixPhasePrograms', 'PioneerQ38TokenPhaseProfilePrograms',
             'ReferenceProducts', 'OriginalSource', 'ValidationSession', 'Q38WorkControl',
             'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace', 'SyntheticIO', 'llama_', 'ggml_'):
    assert name not in table and name not in undefined, name
previous, current = hooks['frames'](parent['obj']), hooks['frames'](obj)
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size)
digest = hooks['sha'](obj)
if not inspect:
    assert digest == PRIVATE_SHA
print(digest + ' ' + obj.name, flush=True)
print('PRIVATE_BUILD_ONLY original5e2c+ALL ancestors FIRST/all14 arrays+2MiB RoPE authenticated/'
      'ONLY3 new worker arrays/actual override register rechecked before release and throughout selected-owner guards/'
      'NO conventional ABI,CPU model reference or Work;NO native lifecycle,watchdog fault,original-model,'
      'hardware,physical timing or speed grant', flush=True)
