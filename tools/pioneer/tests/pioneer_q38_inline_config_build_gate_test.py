#!/usr/bin/env python3
"""Reject unsupported native configurations BEFORE touching SDK/output;NO builds/IO."""
import os
import pathlib
import subprocess
import tempfile

script = pathlib.Path(__file__).resolve().parent.parent / 'build_qwen38_banks_driver.sh'
base = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
required = {
    'PIONEER_Q38_INLINE_CONFIG': '1',
    'PIONEER_Q38_WORK_DEFER_AFFINE': '1',
    'PIONEER_Q38_PANEL_DRIVER': '10',
    'PIONEER_DRIVER_RELOAD': '1',
    'PIONEER_Q38_WORK_MODE': '1',
    'PIONEER_Q38_NATIVE_OPERAND_STORE': '1',
    'PIONEER_Q38_WORK_COMPACT_OUTPUT': '1',
    'PIONEER_Q38_NORMAL_CLOCK': '1',
    'PIONEER_Q38_DISPATCH_PROFILE': '0',
    'PIONEER_Q38_ORDERED_EXTENT_FAST': '0',
}
cases = []
for key, normal in required.items():
    for invalid in ('0', '1', '-1', 'invalid'):
        if invalid != normal:
            cases.append({**required, key: invalid})
# Inline=0 is a supported older candidate,not an invalid configuration.
cases = [case for case in cases if case['PIONEER_Q38_INLINE_CONFIG'] != '0']
for value in ('1', '-1', '2', 'invalid'):
    cases.append({**required, 'PIONEER_Q38_PREPARED_Q4_AFFINE': value})
    cases.append({'PIONEER_Q38_PREPARED_Q4_AFFINE': value})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-inline-build-gate-') as temporary:
    root = pathlib.Path(temporary)
    sdk = root / 'MUST_NOT_OPEN_MISSING_SDK'
    for index, flags in enumerate(cases):
        output = root / ('MUST_NOT_CREATE_OUTPUT_' + str(index))
        result = subprocess.run(['sh', str(script), str(sdk), str(output)],
                                env={**base, **flags}, capture_output=True, text=True)
        assert result.returncode == 2, (flags, result.returncode, result.stderr)
        assert not sdk.exists() and not output.exists()
        assert str(sdk) not in result.stderr, result.stderr
print(f'PASS {len(cases)} inline native dependency/mixed-candidate/invalid-flag and prepared-layout refusals BEFORE SDK/output access;NO native build/device access')
