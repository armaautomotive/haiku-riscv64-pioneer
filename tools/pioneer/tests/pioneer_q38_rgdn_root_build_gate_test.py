#!/usr/bin/env python3
"""ROOT0 qualifier is separate from ordinary mutable/resident Work builds."""
import os
import pathlib
import subprocess
import tempfile
script = pathlib.Path(__file__).resolve().parent.parent / 'build_qwen38_banks_driver.sh'
base = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
required = {
 'PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC': '1', 'PIONEER_Q38_GDN_ZERO_DIAGNOSTIC': '1',
 'PIONEER_Q38_GDN_MUTABLE_STATE': '1', 'PIONEER_Q38_LOCAL_DELTA_GGML': '1',
 'PIONEER_Q38_INLINE_CONFIG': '1', 'PIONEER_Q38_WORK_DEFER_AFFINE': '1',
 'PIONEER_Q38_WORK_OUTPUT_DMA': '1', 'PIONEER_Q38_SPAN_FAST': '1', 'PIONEER_Q38_SPAN_PRELOAD': '1',
 'PIONEER_Q38_LOCAL_DELTA_PROBE': '1', 'PIONEER_Q38_NORMAL_CLOCK': '1',
 'PIONEER_Q38_PANEL_DRIVER': '10', 'PIONEER_DRIVER_RELOAD': '1', 'PIONEER_Q38_WORK_MODE': '1',
 'PIONEER_Q38_DISPATCH_PROFILE': '0', 'PIONEER_Q38_DISPATCH_BATCHED_PIO': '0',
 'PIONEER_Q38_NATIVE_OPERAND_STORE': '1', 'PIONEER_Q38_WORK_COMPACT_OUTPUT': '1',
}
cases = []
for key, correct in required.items():
 for wrong in ('0', '1', '-1', 'invalid'):
  if wrong != correct:
   cases.append({**required, key: wrong})
# root=0 is the supported fixed-zero-only candidate,not an invalid chain.
cases = [f for f in cases if f['PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC'] != '0']
for wrong in ('1', '-1', 'invalid'):
 cases += [{'PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC': wrong}, {'PIONEER_Q38_RESIDENT_GDN': wrong}]
with tempfile.TemporaryDirectory(prefix='pioneer-q38-rgdn-build-gate-') as directory:
 root = pathlib.Path(directory)
 sdk = root / 'MUST_NOT_OPEN_SDK'
 for index, flags in enumerate(cases):
  output = root / ('MUST_NOT_CREATE_' + str(index))
  result = subprocess.run(['sh', str(script), str(sdk), str(output)], env={**base, **flags}, capture_output=True, text=True)
  assert result.returncode == 2, (flags, result.returncode, result.stderr)
  assert not output.exists() and not sdk.exists() and str(sdk) not in result.stderr
print(f'PASS {len(cases)} resident ROOT0 dependency/invalid/ordinary-resident refusals BEFORE SDK/output;NO native access')
