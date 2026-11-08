#!/usr/bin/env python3
"""Exercise post-hoc transcript analysis using synthetic lines only."""
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile

here = Path(__file__).resolve().parent
rows = runpy.run_path(str(here / 'test_pioneer_q38_qgw8_full_output.py'))['rows']
analyzer = here.parent / 'analyze_q38_qgw8_full_trial.py'

def analyze(lines):
    with tempfile.TemporaryDirectory(prefix='qgw8-analysis-') as directory:
        path = Path(directory) / 'trial.stdout'
        path.write_text('\n'.join(lines) + '\n')
        return subprocess.run([sys.executable, str(analyzer), str(path)], text=True, capture_output=True)

good = analyze(rows)
assert good.returncode == 0 and 'match expected=70253 IF_ACTUAL_INPUT=248319' in good.stdout
assert '0.200000 tokens/s; INCLUDES_START_AND_PROMPT_NOT_STEADY_DECODE' in good.stdout
mismatch = analyze(rows[:8] + ['HYBRID_TOKEN 42'] + rows[9:])
assert mismatch.returncode == 0 and 'mismatch expected=70253' in mismatch.stdout
wrong_rate = analyze(rows[:7] + [rows[7].replace('0.200000', '20.000000')] + rows[8:])
assert wrong_rate.returncode != 0 and 'inconsistent' in wrong_rate.stderr
zero = rows[:6] + [rows[6].replace('generated_tokens=2', 'generated_tokens=0'), rows[-1]]
assert 'ZERO_NON_EOS_OUTPUTS' in analyze(zero).stdout
assert analyze(rows[:-1]).returncode != 0
print('PASS QGW8 post-hoc analysis and rate consistency; synthetic only, no accelerator')
