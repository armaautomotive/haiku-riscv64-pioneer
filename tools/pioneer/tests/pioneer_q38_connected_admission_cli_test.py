#!/usr/bin/env python3
"""Reject shortcuts/role contamination BEFORE opening any model/image inputs."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
runner = here/'run_pioneer_q38_token_original_flow_tests.py'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
profile = ['--cancel-boundary', '--unroll-dense-seals', '--boundary-seals', '--unroll-quiet',
           '--worker-local-source-guards', '--precise-activation', '--single-closed-audit']
mode = '--admission-source-faults'
checks = 0
with tempfile.TemporaryDirectory(prefix='pioneer-connected-cli-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    inputs = [str(root/name) for name in ('image', 'mixed', 'worker', 'model.gguf')]

    def reject(flags, expected, extra_env=None):
        global checks
        result = subprocess.run([sys.executable, str(runner), *inputs, *flags],
                                env={**os.environ, **(extra_env or {})}, capture_output=True, text=True, timeout=30)
        assert result.returncode != 0 and expected in result.stderr, (flags, result.stdout, result.stderr)
        assert 'FileNotFoundError' not in result.stderr and not any(root.iterdir()), 'refuse BEFORE input IO/output creation'
        checks += 1

    for candidate in (False, True):
        selected = [*profile, *(['--worker-sealed-admission'] if candidate else []), mode]
        reject([flag for flag in selected if flag != '--single-closed-audit'], 'requires the explicit QGW2 single-audit profile' if candidate
               else 'connected admission fault test requires')
        for option in ('--prepare-only', '--bootstrap-only', '--first-layer-only', '--fast-root-memory',
                       '--snapshot-first-layer', '--eos-after-prompt'):
            reject([*selected, option], 'connected admission fault test requires')
        reject([*selected, '--observed-trace', str(root/'observed')], 'connected admission fault test requires')
        reject([*selected, mode], 'AssertionError')
        reject([*selected, '--unknown-admission-mode'], 'AssertionError')
        reject([flag for flag in selected if flag != '--cancel-boundary'], 'explicitly cancellable FP32 image')
        reject([flag for flag in selected if flag != '--precise-activation'], 'single closed audit requires')
        for key in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE'):
            reject(selected, 'assert not any(k.startswith', {key:'0'})
print(f'PASS connected admission CLI/{checks} shortcut+missing-parent+duplicate+unknown+defined-zero-role refusals BEFORE image/model IO or output creation;NOT runtime/native/physical/Work/TPS qualification')
