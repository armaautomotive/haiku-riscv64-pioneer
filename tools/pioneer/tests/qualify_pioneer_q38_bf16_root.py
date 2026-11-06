#!/usr/bin/env python3
"""BUILD persistent BF16 ROOT and execute worker regressions OFFBOARD only."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv) == 5 and not os.environ.get('PYTHONOPTIMIZE')
sdk, original, private, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
root = here.parents[2]
assert all(p.is_absolute() for p in (sdk, original, private, out))
assert sdk.is_dir() and original.is_file() and private.is_dir()
assert not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700)
tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
dependencies = {Path(__file__).resolve(), here/'qualify_pioneer_q38_bf16_worker.py',
                here/'run_pioneer_q38_bf16_root_role_tests.py', here/'run_pioneer_q38_precise_root_role_tests.py',
                here/'pioneer_q38_full_offload_eog_isa_test.py'}
for name in ('build_qwen38_token_root.py', 'build_qwen38_global_token_components.py',
             'build_qwen38_global_chain_root.py', 'blackhole_qwen38_global_chain.ld',
             'blackhole_qwen38_global_chain_start.S'):
    dependencies.add(here.parent/name)
common = ['-DPIONEER_Q38_GLOBAL_CHAIN_BF16_FPU=1', '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1']
components = ['-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1', '-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
              '-DPIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT=1', '-DPIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION=1',
              '-DPIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS=1', '-DPIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL=1',
              '-DPIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS=1', '-DPIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN=1',
              '-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1', '-DPIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL=1']
units = [('blackhole_qwen38_token_root.cpp', ['-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1',
          '-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK=1', '-DPIONEER_Q38_TOKEN_CANCEL_BOUNDARY=1']),
         ('blackhole_qwen38_return_recurrent_layer.cpp', [*components,
          '-DPIONEER_Q38_LAYER_COMPONENT_RECURRENT=1', '-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1',
          '-DPIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT=1']),
         ('blackhole_qwen38_return_dense_layer.cpp', [*components,
          '-DPIONEER_Q38_LAYER_COMPONENT_DENSE=1', '-DPIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE=1',
          '-DPIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND=1'])]
for name, flags in units:
    data = subprocess.check_output([tool+'g++', '-std=c++11', '-march=rv32imf', '-mabi=ilp32',
                                    *common, *flags, '-MM', str(here.parent/name)], text=True)
    dependencies.update(Path(p).resolve() for p in data.replace('\\\n', '').split(':', 1)[1].split())
assert all(p.is_relative_to(root) for p in dependencies)
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
exits = {}
commands = []


def run(name, argv):
    commands.append([name, *map(str, argv)])
    with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
        result = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr, check=False)
    exits[name] = result.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2)+'\n')
    assert result.returncode == 0 and not (out/(name+'.stderr')).stat().st_size, name
    print('CLOSED_BF16_ROOT_OFFBOARD', name, result.returncode, flush=True)


run('root-build', [sys.executable, here.parent/'build_qwen38_token_root.py', sdk, out/'root',
                  '--inspect-candidate', '--cancel-boundary', '--f32-hidden', '--unroll-dense-seals',
                  '--boundary-seals', '--unroll-quiet', '--worker-local-source-guards',
                  '--precise-activation', '--single-closed-audit', '--worker-sealed-admission', '--bf16-fpu'])
run('bf16-roles', [sys.executable, here/'run_pioneer_q38_bf16_root_role_tests.py', sdk])
run('old-precise-roles', [sys.executable, here/'run_pioneer_q38_precise_root_role_tests.py'])
run('worker-suite', [sys.executable, here/'qualify_pioneer_q38_bf16_worker.py', sdk, original, private, out/'worker-suite'])
assert all(hashlib.sha256(p.read_bytes()).hexdigest() == digest for p, digest in frozen.items())
for line in (out/'worker-suite/source-sha256.txt').read_text().splitlines():
    digest, relative = line.split('  ', 1)
    path = root/relative
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    if path in frozen:
        assert frozen[path] == digest
    frozen[path] = digest
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
artifacts = {}
for prefix in ('root', 'worker-suite/worker', 'worker-suite/cached-regression/services'):
    for path in sorted((out/prefix).rglob('*')):
        if path.is_file() and path.suffix in ('.bin', '.elf'):
            artifacts[str(path.relative_to(out))] = [path.stat().st_size, hashlib.sha256(path.read_bytes()).hexdigest()]
(out/'artifact-sha256.json').write_text(json.dumps(artifacts, sort_keys=True, indent=2)+'\n')
print('PASS_BF16_ROOT_BUILD_AND_WORKER_OFFBOARD children'+str(len(exits))+'_stderr0_source'+str(len(frozen))+
      '_ROOT_EXECUTION_NATIVE_OWNERSHIP_ORIGINAL_FULL_MODEL_QUALITY_HARDWARE_TPS_STILL_OWED', flush=True)
