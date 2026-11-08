#!/usr/bin/env python3
"""Test debug transcript shape, not device state, model output or quality."""
from pathlib import Path
import subprocess

gate = Path(__file__).resolve().parent.parent / 'pioneer_q38_qgw8_hidden_output.awk'
rows = [
    'HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet',
    'HYBRID_PREFLIGHT 1351/1351',
    'HYBRID_UPLOAD 21167624192/21167624192',
    'HYBRID_START_REQUEST diagnostic only; no completion implied',
    'HYBRID_STARTED upload_and_start_seconds=100.000',
    'HYBRID_PENDING seconds=30.000 pc_first=1 pc_second=2; diagnostic only',
    'HYBRID_RESULT prompt_tokens=1 generated_tokens=1 inference_seconds=10.000 (includes START and prompt processing)',
    'QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.100000',
    'HYBRID_TOKEN 108133',
    'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated',
    'QGW8_HIDDEN_SNAPSHOT sha256=' + 'a' * 64 + ' bytes=20548; DATA_ONLY_AFTER_CLOSE',
]
def run(lines):
    return subprocess.run(['awk', '-f', str(gate)], input='\n'.join(lines) + '\n', text=True, capture_output=True)
good = run(rows)
assert good.returncode == 0 and 'outputs=1 token=108133' in good.stdout
zero = rows[:6] + [rows[6].replace('generated_tokens=1', 'generated_tokens=0')] + rows[9:]
assert run(zero).returncode == 0
bad = [rows[:-1], rows[:1] + rows[2:], rows[:2] + rows[3:], rows[:3] + rows[4:],
       rows[:3] + [rows[3], rows[3]] + rows[4:], rows[:7] + rows[8:],
       rows[:8] + ['HYBRID_TOKEN 248320'] + rows[9:], rows[:8] + [rows[8], rows[8]] + rows[9:],
       rows[:10] + [rows[10].replace('a' * 64, 'a' * 63)],
       rows[:10] + [rows[10].replace('bytes=20548', 'bytes=20547')], rows + ['unexpected']]
for case in bad:
    assert run(case).returncode != 0, case
print('PASS tag129 transcript shape and malformed cases; synthetic only, no device or quality')
