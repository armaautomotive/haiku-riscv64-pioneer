#!/usr/bin/env python3
"""Post-dot LOCAL candidate dependency refusals before SDK/output access."""
import os
import pathlib
import subprocess
import tempfile

script = pathlib.Path(__file__).resolve().parent.parent / 'build_qwen38_banks_driver.sh'
base = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
required = {
    'PIONEER_Q38_LOCAL_DELTA_GGML': '1',
    'PIONEER_Q38_LOCAL_DELTA_PROBE': '1',
    'PIONEER_Q38_NORMAL_CLOCK': '1',
    'PIONEER_Q38_PANEL_DRIVER': '10',
    'PIONEER_DRIVER_RELOAD': '1',
    'PIONEER_Q38_WORK_MODE': '1',
    'PIONEER_Q38_DISPATCH_PROFILE': '0',
    'PIONEER_Q38_DISPATCH_BATCHED_PIO': '0',
}
cases = []
for key, normal in required.items():
    for invalid in ('0', '1', '-1', 'invalid'):
        if invalid != normal:
            cases.append({**required, key: invalid})
cases = [case for case in cases if case['PIONEER_Q38_LOCAL_DELTA_GGML'] != '0']
for invalid in ('1', '-1', 'invalid'):
    cases.append({'PIONEER_Q38_RESIDENT_GDN': invalid})
    cases.append({'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC': invalid})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-postscale-build-gate-') as temporary:
    root = pathlib.Path(temporary)
    sdk = root / 'MUST_NOT_OPEN_MISSING_SDK'
    for index, flags in enumerate(cases):
        output = root / ('MUST_NOT_CREATE_OUTPUT_' + str(index))
        result = subprocess.run(['sh', str(script), str(sdk), str(output)],
                                env={**base, **flags}, capture_output=True, text=True)
        assert result.returncode == 2, (flags, result.returncode, result.stderr)
        assert not sdk.exists() and not output.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} post-dot LOCAL dependency/invalid/mixed-profile refusals BEFORE SDK/output;NO native access')
