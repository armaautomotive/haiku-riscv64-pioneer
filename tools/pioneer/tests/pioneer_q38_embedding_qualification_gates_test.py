#!/usr/bin/env python3
"""Exact new role build gates;reject old builder promotion before SDK reads."""
import os
import pathlib
import shlex
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parents[1]
if len(sys.argv) != 2:
    raise SystemExit('expected previously verified tag39 build directory')
build = pathlib.Path(sys.argv[1])
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-embedding-qualification-gates-') as temporary:
    base = pathlib.Path(temporary)
    for name in ('build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh', 'build_qwen38_inference.sh',
                 'build_qwen38_ffn_graph_image.sh', 'build_qwen38_ffn_matrix_image.sh', 'build_qwen38_ffn_persistent_image.sh',
                 'build_qwen38_local_image.sh', 'build_qwen38_projection_graph_image.sh',
                 'build_qwen38_projection_matrix_image.sh', 'build_qwen38_projection_persistent_image.sh'):
        for flag in ('PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_CLIENT'):
            for value in ('1', '2', '01', '-1', '', '0=1', '0\n1'):
                sdk, out = base / 'MUST_NOT_READ_SDK', base / 'MUST_NOT_CREATE_OUTPUT'
                args = ['sh', str(here / name), str(sdk), str(out)]
                result = subprocess.run(args, env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not out.exists() and str(sdk) not in result.stdout + result.stderr, (name, flag, value, result)
                count += 1
    for name in ('build_qwen38_embedding_native.py', 'build_qwen38_embedding_client.sh'):
        for flag in ('PIONEER_Q38_EMBEDDING_NATIVE', 'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_CLIENT',
                     'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNRECOGNIZED_ROLE'):
            for value in ('1', '2', '01', '-1', '', '0=1', '0\n1'):
                sdk, out = base / 'MUST_NOT_READ_SDK', base / 'MUST_NOT_CREATE_OUTPUT'
                args = [sys.executable if name.endswith('.py') else 'sh', str(here / name), str(sdk), str(out)]
                result = subprocess.run(args, env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not out.exists() and str(sdk) not in result.stdout + result.stderr, (name, flag, value, result)
                count += 1
    # Preprocess the ACTUAL SDK driver command,not a copied guard facade.
    commands = [shlex.split(line[2:]) for line in (build / 'controls/old-tag38.trace').read_text().splitlines() if line.startswith('+ ')]
    source = here.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
    candidates = [args for args in commands if '-c' in args and args[args.index('-c') + 1] == str(source) and '-DTT_PIONEER_DRIVER_RELOAD' in args]
    assert len(candidates) == 1
    original = candidates[0]
    args = [arg for arg in original if not arg.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE')
            and not arg.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF') and not arg.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
    args[args.index('-c')] = '-E'
    args[args.index('-o') + 1] = os.devnull
    native, role, tag = '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_QUALIFICATION=1', '-DPIONEER_DRIVER_BUILD_TAG=0x2026102200010039ULL'
    exact = [*args, native, role, tag]
    subprocess.run(exact, env=clean, check=True, capture_output=True, text=True)
    for remove, add in ((role, []), (native, []), (native, ['-DPIONEER_Q38_EMBEDDING_NATIVE=2']),
                        (role, ['-DPIONEER_Q38_EMBEDDING_QUALIFICATION=2']), (tag, ['-DPIONEER_DRIVER_BUILD_TAG=0x2026102200010038ULL']),
                        ('-DTT_PIONEER_DRIVER_RELOAD', []), (None, ['-DPIONEER_Q38_FFN_HOST_PROFILE=1']),
                        (None, ['-DPIONEER_Q38_PROJECTION_GRAPH_MEMORY=1']), (None, ['-DPIONEER_Q38_FFN_SEALED_PROOF=1'])):
        test = [arg for arg in exact if arg != remove] + add
        result = subprocess.run(test, env=clean, capture_output=True, text=True, timeout=30)
        assert result.returncode != 0 and '#error' in result.stderr, (remove, add, result.stderr)
        count += 1
    # No reference/old Work/native-role combination compiles the host header.
    for flag in ('PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_EMBEDDING_NATIVE'):
        result = subprocess.run(['c++', '-std=c++17', '-fsyntax-only', '-x', 'c++', '-DB_DEVICE_OP_CODES_END=1000',
                                 '-DPIONEER_Q38_EMBEDDING_CLIENT=1', '-D' + flag + '=1', '-I' + str(here), '-'],
                                input='#include "PioneerQ38EmbeddingSession.h"\n', env=clean, capture_output=True, text=True)
        assert result.returncode != 0 and 'Separate source-bound embedding client only' in result.stderr
        count += 1
print(f'PASS exact embedding qualification gates{count}:before-SDK old/new builders,actual SDK driver reserved tag/role rejection,host CPU-reference/oldWork/native exclusion;NO install/device IO')
