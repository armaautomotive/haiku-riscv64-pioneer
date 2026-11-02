#!/usr/bin/env python3
"""BUILD CLI refusals before compiler/output IO; no native/model/device test."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) == 1 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='pioneer-q38-output-closure-build-cli-', dir='/private/tmp') as name:
    base = pathlib.Path(name)
    output = base / 'MUST_NOT_EXIST'
    existing = base / 'existing'
    existing.mkdir()
    sentinel = existing / 'sentinel'
    sentinel.write_bytes(b'unchanged existing output\n')
    alias = base / 'alias'
    alias.symlink_to(existing, target_is_directory=True)
    before = sorted(p.relative_to(base).as_posix() for p in base.rglob('*'))
    refused = 0
    for script in ('build_qwen38_batched_output_closure_native.py',
                   'build_qwen38_batched_output_closure_benchmark_native.py'):
        cases = [([], {}), (['relative-output'], {}), ([str(existing)], {}), ([str(alias)], {}),
                 ([str(output), '--wrong-mode'], {}), ([str(output), '--inspect-candidate=0'], {}),
                 ([str(output), '--inspect-candidate', '--inspect-candidate'], {}),
                 ([str(output), '--public', '--inspect-candidate'], {}),
                 ([str(output), '--apply'], {}), ([str(output), '--install'], {})]
        for key in ('PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS',
                    'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT',
                    'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_BENCHMARK_NATIVE',
                    'PIONEER_Q38_CPU_VALIDATION', 'TT_PIONEER_Q38_WORK_MODE'):
            for value in ('0', '1'):
                cases.append(([str(output)], {key: value}))
        for args, environment in cases:
            result = subprocess.run([sys.executable, str(here / script), *args], capture_output=True,
                                    env={**os.environ, **environment})
            assert result.returncode != 0 and not result.stdout and b'AssertionError' in result.stderr, (script, args, result.stderr)
            assert not output.exists() and sorted(p.relative_to(base).as_posix() for p in base.rglob('*')) == before
            assert sentinel.read_bytes() == b'unchanged existing output\n' and alias.is_symlink()
            refused += 1
    print(f'PASS corrected-output BUILD CLI/{refused} missing,relative,existing,symlink,unknown,duplicate,apply,install,role-environment refusals/'
          'ZERO compiler or output creation/existing sentinel+alias unchanged;NOT native lifecycle,model,hardware or TPS proof')
