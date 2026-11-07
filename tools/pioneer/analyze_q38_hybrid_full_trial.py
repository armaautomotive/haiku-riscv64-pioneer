#!/usr/bin/env python3
"""Analyze a completed one-shot transcript without using the accelerator."""
from decimal import Decimal
from pathlib import Path
import re
import subprocess
import sys

if len(sys.argv)!=2:
    raise SystemExit('use HYBRID_TRIAL_STDOUT')
path=Path(sys.argv[1]);text=path.read_text(encoding='utf-8')
gate=Path(__file__).resolve().parent/'pioneer_q38_hybrid_full_output.awk'
result=subprocess.run(['awk','-f',str(gate)],input=text,text=True,capture_output=True,check=False)
if result.returncode:
    raise SystemExit('unclosed or malformed trial transcript; no result')
rows=text.splitlines()
summary=[re.fullmatch(r'HYBRID_RESULT prompt_tokens=1 generated_tokens=(\d+) post_start_seconds=(\d+\.\d{3}) \(includes prompt processing\)',row)
         for row in rows if row.startswith('HYBRID_RESULT ')]
tokens=[int(row.removeprefix('HYBRID_TOKEN ')) for row in rows if row.startswith('HYBRID_TOKEN ')]
if len(summary)!=1 or not summary[0] or int(summary[0].group(1))!=len(tokens):
    raise SystemExit('inconsistent output count; no result')
seconds=Decimal(summary[0].group(2));count=len(tokens)
# The external, CPU-only comparison saved for input token 248319 selected
# 70253 for both original and candidate. This does not validate logits or a
# full answer, and does not enter the on-device Work/inference path.
first='unavailable' if not count else 'match' if tokens[0]==70253 else 'mismatch'
print(f'HYBRID_FIRST_TOKEN_COMPARE {first}; expected=70253; ONE_TOKEN_NOT_QUALITY_PROOF')
print(f'HYBRID_OBSERVED_POST_START count={count} seconds={seconds} outputs_per_second={Decimal(count)/seconds:.6f}; '
      'INCLUDES_PROMPT_MAY_MISS_START_WORK_NOT_STEADY_DECODE_RATE')
