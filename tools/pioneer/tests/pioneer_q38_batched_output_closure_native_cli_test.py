#!/usr/bin/env python3
"""New native TEST CLI boundary plus immutable peer-ROM DATA bridge checks."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
runner = here / 'run_pioneer_q38_token_native_initialization_tests.py'
subprocess.run([sys.executable, str(here / 'pioneer_q38_frontier_failure_link_native_cli_test.py')], check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-output-closure-native-cli-', dir='/private/tmp') as temporary:
    base = pathlib.Path(temporary)
    root, worker, other = (base / name for name in ('root', 'worker', 'other'))
    for directory in (root, worker, other):
        directory.mkdir()
    output = base / 'MUST_NOT_EXIST.bin'
    common = ['/unopened/zero', '--execution-end', '/unopened/model', '/unopened/old', str(worker), str(output)]
    selectors = ['--pushed-context', '--precise-beta', '--posted-cancel', '--root-frontend-snapshots',
                 '--held-hidden-readout', '--job-boot-fold', '--token-frontier', '--retained-conv-closure',
                 '--commit-boundary-seals', '--cohort-local-sources', '--packet-identity-boundary', '--tid-boundary']
    tail = ['--flat-matrix-root', str(root), '--prefetch-worker', str(worker), '--twin-batch-prefetch',
            '--frontier-failure-detail', '--frontier-failure-link', '--batched-output-closure']
    cases = [common + [value for value in selectors if value != flag] + tail for flag in selectors]
    for flag in ('--twin-batch-prefetch', '--frontier-failure-detail', '--frontier-failure-link'):
        cases.append(common + selectors + [value for value in tail if value != flag])
    for mode in ('--execution-first-layer', '--execution-end-reload', '--execution-end-full-flow',
                 '--execution-end-state-full-flow', '--bootstrap-peer-engines', '--execution-readout-empty',
                 '--preparation-endpoint-contract', '--execution-backend-first-matrix'):
        changed = common[:]
        changed[1] = mode
        cases.append(changed + selectors + tail)
    changed = common[:]
    changed[4] = str(other)
    cases.append(changed + selectors + tail)
    cases += [common + selectors + tail + [flag] for flag in ('--batched-output-closure', '--frontier-failure-link',
              '--frontier-failure-detail', '--compile-link-only', '--batched-prefetch', '--twin-batch-prefetch',
              '--batched-output-closure=0')]
    cases += [common + selectors + tail + ['--root-instruction-profile', str(root)],
              common + selectors + tail + ['--prefetch-worker', str(worker)],
              common + selectors + tail + ['--counter-gather-root', str(root)],
              common + selectors + tail + ['--empty-receipt-root', str(root)], common + ['--batched-output-closure']]
    before = sorted(p.relative_to(base).as_posix() for p in base.rglob('*'))
    for args in cases:
        result = subprocess.run([sys.executable, str(runner), *args], capture_output=True)
        assert result.returncode != 0 and b'AssertionError' in result.stderr and not result.stdout, (args, result.stderr)
        assert not output.exists() and sorted(p.relative_to(base).as_posix() for p in base.rglob('*')) == before
    print(f'PASS corrected-output native CLI/{len(cases)} missing,duplicate,wrong-worker,shortened,old,profile,compile-only+'
          'incompatible-ROOT refusals/ALL unchanged QFL1+ancestors FIRST;NO source,compiler,runtime or output creation;'
          'NOT native lifecycle,model or hardware proof', flush=True)
    cpp = here / 'pioneer_q38_batched_output_closure_program_data_bridge.cpp'
    binary = base / 'bridge-test'
    common_flags = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined']
    subprocess.run([*common_flags, '-DPIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_OUTPUT_CLOSURE=1', str(cpp),
                    str(here / 'pioneer_q38_twin_batch_prefetch_program_data_bridge_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'})
    refusals = 0
    for key in ('_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_WORK_INFERENCE',
                'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT',
                'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_BENCHMARK_NATIVE',
                'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT',
                'PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE'):
        for value in ('0', '1'):
            result = subprocess.run([*common_flags, '-D' + key + '=' + value, '-fsyntax-only', str(cpp)], capture_output=True)
            assert result.returncode != 0 and b'error:' in result.stderr
            refusals += 1
    print(f'PASS corrected-output immutable peer-ROM bridge/20 role-isolation refusals/{refusals} checked/'
          'all119 peer code maps tested with unchanged output on rejection;NOT ROOT execution,native lifetime,model or TPS proof')
