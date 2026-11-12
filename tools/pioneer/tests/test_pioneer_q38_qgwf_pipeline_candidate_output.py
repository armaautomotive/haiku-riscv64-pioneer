#!/usr/bin/env python3
"""Fabricated pipelined tag144 transcript lifecycle and rate-output gate tests."""
from pathlib import Path
import subprocess

gate = Path(__file__).resolve().parent.parent / 'pioneer_q38_qgwf_pipeline_candidate_output.awk'
prefix = ['HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet',
          'HYBRID_PREFLIGHT 1351/1351', 'HYBRID_UPLOAD 21094223872/21094223872',
          'HYBRID_START_REQUEST diagnostic only; no completion implied',
          'HYBRID_STARTED upload_and_start_seconds=24.000',
          'HYBRID_RESULT prompt_tokens=1 generated_tokens=17 inference_seconds=50.000 (includes START and prompt processing)',
          'QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.340000']
complete = 'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated'
unavailable = 'QGWF_OBSERVED_DECODE_UNAVAILABLE insufficient_or_invalid_count_window;NOT_A_SPEED_RESULT'
bounded = ('QGWF_OBSERVED_DECODE_BOUNDS tokens=16 first=1 last=17 min_us=1000000 '
           'max_us=1100000 lower_tps=14.545454545 upper_tps=16.000000000;COUNT_WINDOW_NOT_QUALITY')
rows = ['QGWF_PIPELINED_UPLOAD_EXPERIMENT; NO PERFORMANCE OR QUALITY CLAIM'] + prefix + ['HYBRID_TOKEN 42'] * 17 + [complete, bounded]
def run(lines):
    return subprocess.run(['awk', '-f', str(gate)], input='\n'.join(lines)+'\n', text=True, capture_output=True)
assert run(rows).returncode == 0
assert run(rows[:-1]+[unavailable]).returncode == 0
early = ['QGWF_PIPELINED_UPLOAD_EXPERIMENT; NO PERFORMANCE OR QUALITY CLAIM'] + prefix[:5]+[prefix[5].replace('=17 ', '=2 ')]+prefix[6:]+['HYBRID_TOKEN 42','HYBRID_TOKEN 248044',complete,unavailable]
assert run(early).returncode == 0
bad = [rows[:-2], rows[:-1], rows+['unexpected'], rows[:2]+rows[3:],
       rows[1:],
       rows[:8]+['HYBRID_TOKEN 248044']+rows[9:], rows[:8]+['HYBRID_TOKEN 248320']+rows[9:],
       early[:-2]+['HYBRID_TOKEN 42',complete,unavailable],
       early[:-1]+[bounded], rows[:-1]+[bounded.replace('tokens=16','tokens=15')],
       rows[:-1]+[bounded.replace('last=17','last=16')],
       rows[:-1]+[bounded.replace('min_us=1000000','min_us=0')],
       rows[:-1]+[bounded.replace('max_us=1100000','max_us=1')],
       rows[:-1]+[bounded.replace('upper_tps=16.000000000','upper_tps=1.000000000')]]
for case in bad:
    assert run(case).returncode != 0, case
print('PASS pipelined tag144 lifecycle, early-EOS and count-window transcript gates; synthetic only')
