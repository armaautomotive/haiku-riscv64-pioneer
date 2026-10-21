#!/usr/bin/env python3
"""Keep TC/SM7 Q8 matrix timing isolated from full-model/Assistant paths."""
import os
import pathlib
import subprocess
import tempfile

script = pathlib.Path(__file__).resolve().parent.parent / 'build_qwen38_inference.sh'
base = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
required = {
    'PIONEER_Q38_Q8_REPEAT_BENCH': '1',
    'PIONEER_Q38_EXECUTION_MODE': 'work',
    'PIONEER_Q38_DISPATCH_INFERENCE': '1',
    'PIONEER_Q38_VECTOR_INFERENCE': '1',
    'PIONEER_Q38_SPAN_PRELOAD': '1',
    'PIONEER_Q38_SPAN_FAST': '1',
    'PIONEER_Q38_CPU_POOL_PROBE': '0',
    'PIONEER_Q38_ASSISTANT_WORKER': '0',
    'PIONEER_Q38_FFN_REPEAT_BENCH': '0',
}
cases = []
for native in ('0', '1'):
    valid = {**required, 'PIONEER_Q38_NATIVE_Q8_LAYOUT': native,
             'PIONEER_Q38_NATIVE_Q8_GRAPH_QUALIFICATION': native}
    for key, correct in valid.items():
        for wrong in ('0', '1', '01', 'invalid', '-1'):
            if wrong == correct or (key == 'PIONEER_Q38_Q8_REPEAT_BENCH'
                                    and wrong == '0' and native == '0'):
                continue
            cases.append({**valid, key: wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-q8-repeat-gate-') as name:
    root = pathlib.Path(name)
    sdk, libs = root / 'MUST_NOT_OPEN_SDK', root / 'MUST_NOT_READ_LIBS'
    for i, flags in enumerate(cases):
        out = root / ('MUST_NOT_CREATE_' + str(i))
        result = subprocess.run(['sh', str(script), str(sdk), str(libs), str(out)],
                                env={**base, **flags}, capture_output=True, text=True)
        assert result.returncode == 2, (flags, result.returncode, result.stderr)
        assert not out.exists() and str(sdk) not in result.stderr and str(libs) not in result.stderr
    # Both valid isolated selectors reach SDK resolution, not a hidden flag refusal.
    for native in ('0', '1'):
        flags = {**required, 'PIONEER_Q38_NATIVE_Q8_LAYOUT': native,
                 'PIONEER_Q38_NATIVE_Q8_GRAPH_QUALIFICATION': native}
        result = subprocess.run(['sh', str(script), str(sdk), str(libs), str(root / 'VALID_NO_BUILD')],
                                env={**base, **flags}, capture_output=True, text=True)
        assert result.returncode != 2 and str(sdk) in result.stderr, result.stderr
print(f'PASS {len(cases)} Q8 repeat/native producer isolation refusals BEFORE SDK/libs/output;'
      '2 valid isolated selectors reach SDK resolution;NO native/device access')
