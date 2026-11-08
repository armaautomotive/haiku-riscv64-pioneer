#!/usr/bin/env python3
"""Inspect an already closed tag128 transcript; never use accelerator/CPU model."""
from decimal import Decimal
from pathlib import Path
import re
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: analyze_q38_qgw8_full_trial.py QGW8_TRIAL_STDOUT')
text = Path(sys.argv[1]).read_text(encoding='utf-8')
gate = Path(__file__).resolve().parent / 'pioneer_q38_qgw8_full_output.awk'
shape = subprocess.run(['awk', '-f', str(gate)], input=text, text=True, capture_output=True)
if shape.returncode:
    raise SystemExit('malformed or incomplete QGW8 transcript; no inference result')
rows = text.splitlines()
summary = next(re.fullmatch(
    r'HYBRID_RESULT prompt_tokens=1 generated_tokens=(\d+) inference_seconds=(\d+\.\d{3}) \(includes START and prompt processing\)',
    row) for row in rows if row.startswith('HYBRID_RESULT '))
count, seconds = int(summary.group(1)), Decimal(summary.group(2))
tokens = [int(row.removeprefix('HYBRID_TOKEN ')) for row in rows if row.startswith('HYBRID_TOKEN ')]
if count:
    rate = Decimal(next(row.split()[1] for row in rows if row.startswith('QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT ')))
    # The transcript displays elapsed time to milliseconds and rate to six
    # decimal places, so permit only the corresponding rounding uncertainty.
    tolerance = Decimal('0.0000005')
    lower = Decimal(count) / (seconds + Decimal('0.0005'))
    upper = Decimal(count) / (seconds - Decimal('0.0005'))
    if rate + tolerance < lower or rate - tolerance > upper:
        raise SystemExit('reported QGW8 rate inconsistent with displayed count/time')
    first = 'match' if tokens[0] == 70253 else 'mismatch'
    print('QGW8_FIRST_TOKEN_COMPARE %s expected=70253 IF_ACTUAL_INPUT=248319; ONE_TOKEN_NOT_QUALITY_PROOF' % first)
    print('QGW8_TRANSCRIPT_REPORTED_RATE %s tokens/s; INCLUDES_START_AND_PROMPT_NOT_STEADY_DECODE' % rate)
else:
    print('QGW8_FIRST_TOKEN_COMPARE unavailable; ZERO_NON_EOS_OUTPUTS')
print('TRANSCRIPT_ONLY: independently confirm process exit, device state, and actual input; no quality claim')
