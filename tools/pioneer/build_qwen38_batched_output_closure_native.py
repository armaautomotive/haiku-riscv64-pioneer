#!/usr/bin/env python3
"""Distinct corrected-ROOT PRIVATE object only; never install or execute."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA = 'b688d89b569495d4d027b3880f7e28f2e1ece15974c3d10823f0cfa6d7c36a30'
assert len(sys.argv) in (2, 3) and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
inspect = len(sys.argv) == 3
assert not inspect or sys.argv[2] == '--inspect-candidate'
assert inspect or PRIVATE_SHA
here = pathlib.Path(__file__).resolve().parent
out = pathlib.Path(sys.argv[1])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_frontier_failure_link_native.py'), str(out / 'unchanged-QFL1')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
hooks = parent['old']
tool = hooks['tool']
assert hooks['sha'](parent['obj']) == '3593260a72978b739120eb26a9ba866a622f7a6369b0ffbda4e4bce6c8b518e3'
sys.stdout.flush()
obj = out / 'batched-output-closure-NODRV.o'
command = parent['argv'][:]
command[command.index('-c') + 1] = str(here / 'tests/pioneer_q38_batched_output_closure_native_context_compile.cpp')
command[command.index('-o') + 1] = str(obj)
subprocess.run(command, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(command) + '\n')


def payload(path, name, symbols, sections, label):
    first, size = symbols[name]
    target = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', sections[name], str(path), str(target)], check=True)
    data = target.read_bytes()
    assert first + size <= len(data)
    return data[first:first + size]


before, before_sections = hooks['records'](parent['obj'])
symbols, sections = hooks['records'](obj)
header = (here / 'PioneerQ38TokenBatchedOutputClosurePrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == '1e844aca55fd4920a278fcd702a64b324fe13ea2c29e52d4cd75841e42f06891'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 14
for digest, label in arrays:
    names = [name for name in symbols if 'PioneerQ38TokenBatchedOutputClosurePrograms' in name
             and name.endswith(str(len(label)) + label + 'E')]
    old_names = [name for name in before if 'PioneerQ38TokenTwinBatchPrefetchPrograms' in name
                 and name.endswith(str(len(label)) + label + 'E')]
    assert len(names) == len(old_names) == 1
    data = payload(obj, names[0], symbols, sections, 'new-' + label)
    previous = payload(parent['obj'], old_names[0], before, before_sections, 'old-' + label)
    assert hashlib.sha256(data).hexdigest() == digest and len(data) == len(previous)
    if label == 'RootText':
        at = 0x504e8 - 0x40000
        assert previous[at:at + 4] == (0x01400613).to_bytes(4, 'little')
        assert data == previous[:at] + (0x00a00613).to_bytes(4, 'little') + previous[at + 4:]
    else:
        assert data == previous, label
rope = [name for name, (_, size) in symbols.items()
        if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size == 2097152]
assert len(rope) == 1 and rope[0] in before
assert payload(obj, rope[0], symbols, sections, 'new-rope') == \
    payload(parent['obj'], rope[0], before, before_sections, 'old-rope')
assert not set(hooks['abi']) & symbols.keys(), 'No conventional driver ABI in PRIVATE subject'
for name in ('init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'):
    for prefix in ('pioneer_q38_token_context_body_', 'pioneer_q38_token_context_only_'):
        symbol = prefix + name
        assert payload(obj, symbol, symbols, sections, 'new-' + symbol) == \
            payload(parent['obj'], symbol, before, before_sections, 'old-' + symbol)
table = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(obj)], text=True)
undefined = subprocess.check_output([tool + 'nm', '-u', str(obj)], text=True)
for name in ('PioneerQ38TokenBatchedOutputClosurePrograms', 'Q38TokenNativeControl(',
             'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
             'BeginQ38TokenNativeBackendScoped(', 'FinishQ38TokenNativeBackendScoped('):
    assert name in table, name
for name in ('PioneerQ38TokenTwinBatchPrefetchPrograms', 'PioneerQ38TokenBatchedPrefetchPrograms',
             'PioneerQ38TokenPrefetchPrograms', 'PioneerQ38TokenFlatMatrixPrograms',
             'ReferenceProducts', 'OriginalSource', 'ValidationSession', 'Q38WorkControl',
             'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace',
             'SyntheticIO', 'llama_', 'ggml_'):
    assert name not in table and name not in undefined, name
previous, current = hooks['frames'](parent['obj']), hooks['frames'](obj)
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size)
digest = hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:
    assert digest == PRIVATE_SHA
print('PASS old3593 QFL1+22a/feaa/a0f0 whole controls FIRST/all14 new package arrays authenticated/'
      'ONLY RootText one instruction changes/13 other arrays+2MiB RoPE+10 inert ABI bodies exact/'
      'no conventional ABI,old program namespace,CPU model reference or Work;'
      'static frames<=2048 or unchanged larger predecessor;NOT whole-kernel/combined-stack proof', flush=True)
print(digest + ' ' + obj.name, flush=True)
print('PRIVATE_BUILD_ONLY corrected FINAL-output ROOT;NO native lifecycle,installation,model,hardware,Work or TPS grant', flush=True)
