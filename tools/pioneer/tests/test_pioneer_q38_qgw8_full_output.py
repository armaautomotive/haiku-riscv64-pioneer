#!/usr/bin/env python3
"""Check candidate transcript shape; fixtures are synthetic, no device."""
from pathlib import Path
import subprocess

gate = Path(__file__).resolve().parent.parent / 'pioneer_q38_qgw8_full_output.awk'
rows = [
    'HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet',
    'HYBRID_PREFLIGHT 21/1351',
    'HYBRID_PREFLIGHT 1351/1351',
    'HYBRID_UPLOAD 268435456/21167624192',
    'HYBRID_UPLOAD 21167624192/21167624192',
    'HYBRID_STARTED upload_and_start_seconds=24.000',
    'HYBRID_RESULT prompt_tokens=1 generated_tokens=2 post_start_seconds=10.000 (includes prompt processing)',
    'QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_PROMPT 0.200000',
    'HYBRID_TOKEN 70253',
    'HYBRID_TOKEN 17',
    'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated',
]

def run(lines):
    return subprocess.run(['awk', '-f', str(gate)], input='\n'.join(lines) + '\n', text=True, capture_output=True)

good = run(rows)
assert good.returncode == 0 and 'outputs=2 first_token=70253' in good.stdout, good.stderr
zero = rows[:6] + [rows[6].replace('generated_tokens=2', 'generated_tokens=0'), rows[-1]]
assert run(zero).returncode == 0
bad = [
    rows[:-1],                              # No successful FINISH/close marker.
    rows[:2] + rows[3:],                   # Incomplete preflight.
    rows[:4] + rows[5:],                   # Incomplete upload.
    rows[:7] + rows[8:],                   # Missing rate for nonzero output.
    rows[:8] + [rows[8], rows[8], rows[9]] + rows[10:], # Extra token.
    rows[:8] + ['HYBRID_TOKEN 248320'] + rows[9:],      # Vocabulary limit.
    rows[:3] + [rows[3], rows[3]] + rows[4:],           # Duplicate progress.
    rows[:7] + [rows[7], rows[7]] + rows[8:],           # Duplicate rate.
    rows[:6] + [rows[6].replace('post_start_seconds=10.000', 'post_start_seconds=0.000')] + rows[7:],
    rows + ['unexpected output'],
    rows[:4] + [rows[4].replace('21167624192', '21137039360')] + rows[5:],
]
for case in bad:
    assert run(case).returncode != 0, case
print('PASS QGW8 transcript shape and malformed cases; synthetic only, no device or speed result')
