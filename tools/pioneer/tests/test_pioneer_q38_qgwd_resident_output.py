#!/usr/bin/env python3
"""Adversarial transcript shape only; no driver, card or model."""
from pathlib import Path
import subprocess

gate = Path(__file__).resolve().parents[1] / 'pioneer_q38_qgwd_resident_candidate_output.awk'
prefix = [
    'HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet',
    'HYBRID_PREFLIGHT 1351/1351',
    'HYBRID_UPLOAD 21094223872/21094223872',
    'HYBRID_START_REQUEST diagnostic only; no completion implied',
    'HYBRID_STARTED upload_and_start_seconds=100.000',
    'HYBRID_PENDING seconds=0.001 pc_first=1 pc_second=2; diagnostic only',
    'HYBRID_RESULT prompt_tokens=1 generated_tokens=17 inference_seconds=20.000 (includes START and prompt processing)',
    'QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.850000',
] + ['HYBRID_TOKEN 70253'] * 17 + [
    'QGWD_RESIDENT_FIRST_REQUEST_CLOSED weights retained; output quality not validated',
]
unavailable = 'QGWF_OBSERVED_DECODE_UNAVAILABLE insufficient_or_invalid_count_window;NOT_A_SPEED_RESULT'
bounds = ('QGWF_OBSERVED_DECODE_BOUNDS tokens=16 first=1 last=17 min_us=1000000 '
          'max_us=1000010 lower_tps=15.999840000 upper_tps=16.000000000;COUNT_WINDOW_NOT_QUALITY')
suffix = [
    'QGWD_RESIDENT_SECOND_STARTED no second upload; diagnostic only',
    'QGWD_RESIDENT_SECOND_PENDING seconds=0.001 pc_first=1 pc_second=2',
    'QGWD_RESIDENT_SECOND_MATCH count=17 seconds_including_start=20.000; QUALITY_NOT_VALIDATED',
    'QGWD_RESIDENT_TWO_REQUESTS_UNLOADED; QUALITY_NOT_VALIDATED',
]
def check(lines, expected):
    result = subprocess.run(['awk', '-f', str(gate)], input='\n'.join(lines)+'\n', text=True, capture_output=True)
    assert (result.returncode == 0) == expected, (result.stdout, result.stderr)

for observation in (unavailable, bounds):
    valid = prefix + [observation] + suffix
    check(valid, True)
    for i in range(len(valid)):
        if valid[i].startswith(('HYBRID_PENDING ', 'QGWD_RESIDENT_SECOND_PENDING ')):
            continue  # Progress records are optional, not authority.
        check(valid[:i] + valid[i+1:], False)
    for i in (0, 1, 2, 3, 4, 6, 7, len(prefix)-1, len(valid)-4, len(valid)-2, len(valid)-1):
        check(valid[:i] + [valid[i]] + valid[i:], False)
    check(valid[:-1] + ['HYBRID_UPLOAD 21094223872/21094223872'] + valid[-1:], False)
    check([line.replace('HYBRID_TOKEN 70253', 'HYBRID_TOKEN 248044', 1) if i == 8 else line
           for i, line in enumerate(valid)], False)
    check([line.replace('SECOND_MATCH count=17', 'SECOND_MATCH count=16') for line in valid], False)
    check([line.replace('seconds_including_start=20.000', 'seconds_including_start=0.000') for line in valid], False)
print('PASS resident transcript completion, single-upload, ordered transitions and corrupted/missing/duplicate refusal; NO DEVICE')
