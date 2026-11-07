#!/usr/bin/env python3
"""Build/inspect distinct tag116 timing driver; NEVER install or run it."""
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
root = here.parents[1]
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_qwen38_full_offload_native.py'), str(sdk), str(out/'unchanged-vector92'), '--batched', '--metrics', '--vector-q4']
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(out/'unchanged-vector92/tenstorrent_pci_probe-native') == 'c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014'
tool, cxx, common, includes = (old[k] for k in ('tool', 'cxx', 'common', 'includes'))
flags = ['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in
         ('BANKS', 'PANEL', 'AFFINE', 'BLOCK', 'STREAM', 'ROWS', 'COHORT', 'COHORT_SEALED', 'COHORT_PAYLOAD', 'DISPATCH', 'DISPATCH_VECTOR')]
flags += ['-DTT_PIONEER_DRIVER_RELOAD=1', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010116ULL',
          '-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1', '-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1',
          '-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1']
source = root/'src/add-ons/kernel/drivers/misc/pioneer_q38_full_offload_driver.cpp'
binary = out/'tenstorrent_pci_probe-fast-decode116'
commands = []
def run(command):
    command = list(map(str, command))
    commands.append(command)
    subprocess.run(command, check=True, cwd=out)

dependency_text = subprocess.check_output([*common, *flags, *includes, '-MM', str(source)], text=True)
dependencies = {Path(p).resolve() for p in dependency_text.replace('\\\n', '').split(':', 1)[1].split()
                if Path(p).resolve().is_relative_to(root)}
dependencies.update((Path(__file__).resolve(), here/'build_qwen38_full_offload_native.py'))
frozen = {p: sha(p) for p in dependencies}
run([*common, *flags, *includes, '-MMD', '-MF', out/'native.d', '-c', source, '-o', out/'native.o'])
run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', old['compiler_path']('crtbeginS.o'),
     old['release']/'system/glue/haiku_version_glue.o', out/'native.o', out/'unchanged-vector92/profile.o',
     old['release']/'system/kernel/kernel.so', old['compiler_path']('libgcc.a'), old['compiler_path']('crtendS.o'), '-o', binary])
symbols = subprocess.check_output([tool+'nm', '-C', '--defined-only', str(binary)], text=True)
undefined = subprocess.check_output([tool+'nm', '-u', str(binary)], text=True)
for name in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device',
             'Q38TokenNativeControl(', 'BeginQ38TokenNativeBackendScoped(', 'FinishQ38TokenNativeBackendScoped(',
             'Q38FullOffloadCounter(', 'Q38TokenPhaseProfileControl(', 'ReadQ38TokenNativePhaseProfile(', 'ReadPhaseProfile<'):
    assert name in symbols, name
for name in ('llama_', 'ggml_', 'Q38WorkControl', 'ReferenceProducts', 'OriginalSource', 'SyntheticIO', 'NativeQ38PeerInstructionModel',
             'Q38PreparedSourceControl(', 'Q38FullResidentControl('):
    assert name not in symbols and name not in undefined, name
namespace = 'PioneerQ38FullOffloadFastDecodePrograms'
header = (here/(namespace+'.h')).read_bytes()
assert hashlib.sha256(header).hexdigest() == '3fd94232de0b045e5a74c41d59e0f80b8dba0369e54974a77e62800bf5106f8a'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode('ascii'))
assert len(arrays) == 15
raw = subprocess.check_output([tool+'nm', '-S', '--defined-only', str(out/'native.o')], text=True)
entries = {r[-1]: (int(r[0], 16), int(r[1], 16)) for r in map(str.split, raw.splitlines()) if len(r) == 4}
rows = subprocess.check_output([tool+'objdump', '-t', str(out/'native.o')], text=True)
sections = {r[-1]: r[-3] for r in map(str.split, rows.splitlines()) if len(r) >= 6 and r[-1] in entries}
for digest, label in arrays:
    names = [n for n in entries if namespace in n and n.endswith(str(len(label))+label+'E')]
    assert len(names) == 1, label
    name = names[0]
    first, size = entries[name]
    target = out/(label+'.raw')
    run([tool+'objcopy', '-O', 'binary', '-j', sections[name], out/'native.o', target])
    assert hashlib.sha256(target.read_bytes()[first:first+size]).hexdigest() == digest, label
    if label == 'RootFastDecode':
        assert size == 1496
for previous in ('PioneerQ38FullOffloadVectorQ4Programs', 'PioneerQ38FullOffloadPreparedProfileQ4Programs',
                 'PioneerQ38TokenPushedPrograms', 'PioneerQ38TokenBf16Programs'):
    assert previous not in raw
frames = [r.split('\t') for r in (out/'native.su').read_text().splitlines()]
assert frames and all(len(r) == 3 and r[2] == 'static' for r in frames)
token_frames = [int(r[1]) for r in frames if 'Token' in r[0] or 'FullOffload' in r[0]]
assert token_frames and max(token_frames) <= 2048
assert all(sha(p) == digest for p, digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'build-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
(out/'native.symbols').write_text(symbols)
(out/'native.undefined').write_text(undefined)
print(binary.name, binary.stat().st_size, sha(binary), flush=True)
print('PASS BUILD_ONLY tag116/all15 exact arrays/53915328B journal/v3 held read/vector92+default43 byte-unchanged/maxIndividualTokenFrame'+str(max(token_frames)), flush=True)
print('NO installed driver, hardware, model quality, speed or whole-kernel stack qualification', flush=True)
