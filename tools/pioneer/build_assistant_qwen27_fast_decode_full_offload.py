#!/usr/bin/env python3
"""Build vocabulary-only tag116 benchmark worker/client; NEVER install/run."""
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv) == 4 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, libraries, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
root = here.parents[1]
assert sdk.is_absolute() and sdk.is_dir() and libraries.is_absolute() and libraries.is_dir()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_assistant_qwen27_full_offload.py'), str(sdk), str(libraries), str(out/'unchanged-vector92'),
                '--batched', '--metrics', '--vector-q4']
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(old['binary']) == '1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9'
assert sha(old['rate']) == '9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af'
source = here/'pioneer_assistant_qwen27_fast_decode_full_offload.cpp'
obj = out/'worker.o'
binary = out/'HaikuAssistantQwen27FastDecodeFullOffload'
compile = old['compile'][:]
compile.remove('-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1')
compile.insert(1, '-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1')
compile[compile.index(str(old['source']))] = str(source)
compile[compile.index(str(old['obj']))] = str(obj)
dependency_text = subprocess.check_output([*compile[:compile.index('-c')], '-MM', str(source)], text=True)
dependencies = {Path(p).resolve() for p in dependency_text.replace('\\\n', '').split(':', 1)[1].split()
                if Path(p).resolve().is_relative_to(root)}
dependencies.update((Path(__file__).resolve(), here/'build_assistant_qwen27_full_offload.py',
                     here/'pioneer_q38_full_offload_rate_smoke.cpp', here/'pioneer_q38_full_offload_smoke.cpp',
                     here/'PioneerQ38FullOffloadRateData.h'))
frozen = {p: sha(p) for p in dependencies}
commands = []
def run(command):
    command = list(map(str, command))
    commands.append(command)
    subprocess.run(command, check=True, cwd=out)
run(compile)
link = old['link'][:]
link[link.index(str(old['obj']))] = str(obj)
link[link.index(str(old['binary']))] = str(binary)
run(link)
frames = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 32768 for row in frames)
imports = subprocess.check_output([old['tool']+'nm', '-u', str(binary)], text=True)
llama = {r.split()[-1] for r in imports.splitlines() if r.split() and r.split()[-1].startswith('llama_')}
assert llama == old['expected']
for name in ('llama_decode', 'llama_init_from_model', 'llama_get_logits', 'ggml_backend', 'ggml_threadpool', 'PioneerQ38Ggml'):
    assert name not in imports, name
for marker in (b'Full-card fast decode profile valid', b'Full-card fast decode profile bucket', b'Full-card startup timing '):
    assert marker in binary.read_bytes(), marker
refusals = []
for macro in ('PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT',
              'PIONEER_Q38_FULL_OFFLOAD_BATCHED', 'PIONEER_Q38_FULL_OFFLOAD_METRICS'):
    for value in (None, '0', '2'):
        refusals.append((macro+str(value), ['-U'+macro]+([] if value is None else ['-D'+macro+'='+value])))
for macro in ('_KERNEL_MODE', 'PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4', 'PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4',
              'PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE', 'PIONEER_Q38_BF16_FULL_PROFILE', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE',
              'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE'):
    refusals.append((macro+'defined0', ['-D'+macro+'=0']))
for label, flags in refusals:
    result = subprocess.run([*compile[:compile.index('-c')], *flags, '-fsyntax-only', str(source)], capture_output=True, text=True)
    assert result.returncode and ('requires' in result.stderr or 'Full-offload client is separate' in result.stderr), label
client = out/'pioneer-q38-fast-decode-rate-smoke'
client_command = old['rate_command'][:]
previous = [arg for arg in client_command if arg.startswith('-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER=')]
assert len(previous) == 1
client_command[client_command.index(previous[0])] = '-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-fast-decode-116-a/HaikuAssistantQwen27FastDecodeFullOffload"'
client_command[client_command.index(str(old['rate']))] = str(client)
client_command.insert(1, '-DPIONEER_FULL_OFFLOAD_STARTUP_TIMING=1')
run(client_command)
client_imports = subprocess.check_output([old['tool']+'nm', '-u', str(client)], text=True)
assert 'llama_' not in client_imports and 'ggml_' not in client_imports
assert all(sha(p) == digest for p, digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'build-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
(out/'worker.imports').write_text(imports)
(out/'client.imports').write_text(client_imports)
(out/'role-refusals.txt').write_text(''.join('PASS '+label+'\n' for label, _ in refusals))
for path in (obj, binary, client):
    print(path.name, path.stat().st_size, sha(path), flush=True)
print('PASS BUILD_ONLY tag116 USER/exact11 vocabulary imports/no CPU model context/decode/fallback/new8807 held read/v3 generation window/startup DATA/'+str(len(refusals))+' role refusals/vector92 worker+rate client byte-unchanged;NO hardware or speed result', flush=True)
