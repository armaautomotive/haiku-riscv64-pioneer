#!/usr/bin/env python3
"""Reject incomplete batched native histories before source/compiler/device IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
runner = here / 'run_pioneer_q38_token_native_initialization_tests.py'
# Preserve all unchanged prefetch/root-frontend CLI controls FIRST.
subprocess.run([sys.executable, str(here / 'pioneer_q38_prefetch_native_cli_test.py')], check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-batched-native-cli-', dir='/private/tmp') as temporary:
    base = pathlib.Path(temporary)
    root, worker, other = (base / name for name in ('root', 'worker', 'other'))
    for directory in (root, worker, other):
        directory.mkdir()
    prefix = base / 'MUST_NOT_EXIST.bin'
    common = ['/unopened/zero', '--execution-end', '/unopened/model', '/unopened/old', str(worker), str(prefix)]
    selectors = ['--pushed-context', '--precise-beta', '--posted-cancel', '--root-frontend-snapshots',
                 '--held-hidden-readout', '--job-boot-fold', '--token-frontier', '--retained-conv-closure',
                 '--commit-boundary-seals', '--cohort-local-sources', '--packet-identity-boundary', '--tid-boundary']
    tail = ['--flat-matrix-root', str(root), '--prefetch-worker', str(worker), '--batched-prefetch']
    cases = [common + [v for v in selectors if v != flag] + tail for flag in selectors]
    for mode in ('--execution-first-layer', '--execution-end-reload', '--execution-full-flow',
                 '--execution-end-full-flow', '--execution-end-state-full-flow', '--bootstrap-peer-engines',
                 '--execution-readout-empty'):
        changed = common[:]
        changed[1] = mode
        cases.append(changed + selectors + tail)
    changed = common[:]
    changed[4] = str(other)
    cases.append(changed + selectors + tail)
    cases += [common + selectors + tail + ['--batched-prefetch'],
              common + selectors + tail + ['--prefetch-worker', str(worker)],
              common + selectors + tail + ['--compile-link-only'],
              common + selectors + tail + ['--root-instruction-profile', str(root)],
              common + selectors + tail + ['--counter-gather-root', str(root)],
              common + selectors + tail + ['--empty-receipt-root', str(root)],
              common + selectors + ['--batched-prefetch'],
              common + selectors + ['--flat-matrix-root', str(root), '--batched-prefetch'],
              common + selectors + ['--prefetch-worker', str(worker), '--batched-prefetch'],
              common + selectors + tail + ['--batched-prefetch=0']]
    before = sorted(p.relative_to(base).as_posix() for p in base.rglob('*'))
    for args in cases:
        result = subprocess.run([sys.executable, str(runner), *args], capture_output=True, text=True)
        assert result.returncode and 'AssertionError' in result.stderr and not result.stdout, (args, result.stderr)
        assert not prefix.exists() and sorted(p.relative_to(base).as_posix() for p in base.rglob('*')) == before
    print(f'PASS batched native CLI/{len(cases)} missing+duplicate+wrongworker+shortened+profile+compile-only+'
          'incompatible-ROOT refusals/ENTIRE old controls FIRST;NO source,compiler,runtime or output creation;'
          'NOT native execution or device qualification')

# Syntax checks of this HOST observer's binding are deliberately separate
# from the actual full native execution. The template is NOT instantiated
# and the declarations below cannot supply any runtime bytes or products.
prefix = ('#include <cassert>\n#include <cstdint>\n#include <string>\n#include <vector>\n'
          'std::vector<uint8_t> Read(const char*);\nuint32_t Word(const std::vector<uint8_t>&,unsigned);\n'
          'namespace M {enum {Job=1,Status=2};}\nnamespace G {enum {GlobalMatrix=3};}\n')
required = {'PIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST': '1',
            'PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST': '1',
            'PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE': '1',
            'PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY': '1'}
positive = negative = 0


def syntax(values, okay):
    global positive, negative
    source = prefix + ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
    source += '#include "' + str(here / 'PioneerQ38NativeBatchedPrefetchObserver.h') + '"\n'
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                             '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, (values, result.stderr)
    if okay:
        positive += 1
    else:
        assert 'error:' in result.stderr
        negative += 1


syntax(required, True)
# Legacy arithmetic-definition selection is required by the real HOST ISA
# model,not a deployed/native Assistant Work capability.
syntax({**required, 'TT_PIONEER_Q38_WORK_MODE': '1'}, True)
for key in required:
    bad = required.copy()
    del bad[key]
    syntax(bad, False)
    if key != 'PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY':
        for value in ('0', '2'):
            syntax({**required, key: value}, False)
for key in ('PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST', '_KERNEL_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
            'PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT', 'PIONEER_Q38_CPU_VALIDATION',
            'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE'):
    for value in ('0', '1'):
        syntax({**required, key: value}, False)
print(f'PASS batched HOST observer/{positive} syntax positives/{negative} missing+nonexact+old+kernel+'
      'native+CPU+runtime-Work refusals;NOT instantiated execution or device qualification')
