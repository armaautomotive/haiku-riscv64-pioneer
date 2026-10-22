#!/usr/bin/env python3
"""No inherited FFN load-only flag may leak into driver/Assistant/runtime."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-prime-gates-') as temporary:
    root = pathlib.Path(temporary)
    clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
    for name in ['build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh', 'build_qwen38_inference.sh']:
        for value in ['1', '01', '-1', '2', 'bad']:
            output = root / f'never-output-{cases}'
            sdk = root / 'MUST_NOT_OPEN_SDK'
            args = ['sh', str(directory / name), str(sdk), str(output)]
            if name == 'build_qwen38_inference.sh':
                args.append(str(root / 'MUST_NOT_OPEN_LIBRARIES'))
            r = subprocess.run(args, env={**clean, 'PIONEER_Q38_FFN_PRIME': value}, capture_output=True, text=True, timeout=5)
            assert r.returncode == 2 and not output.exists() and str(sdk) not in r.stdout+r.stderr
            cases += 1
    for flag in ['PIONEER_Q38_FFN_PRIME', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_NATIVE_Q8_LAYOUT',
                 'PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC', 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT', 'PIONEER_Q38_MODEL_THREAD_SWEEP']:
        for value in ['1', '01', '-1', 'bad']:
            output = root / f'never-output-{cases}'
            sdk = root / 'MUST_NOT_OPEN_SDK'
            r = subprocess.run(['sh', str(directory / 'build_qwen38_ffn_prime.sh'), str(sdk), str(output)],
                               env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
            assert r.returncode == 2 and not output.exists() and str(sdk) not in r.stdout+r.stderr
            cases += 1
print(f'PASS {cases} FFN source-prime mixed/production/driver/Assistant-role refusals BEFORE SDK/output;NO native IO')
