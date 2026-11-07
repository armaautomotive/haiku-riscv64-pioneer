#!/usr/bin/env python3
"""Numerical parsing and one-token comparison do not access the card."""
from pathlib import Path
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parent.parent
good='''HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet
HYBRID_PREFLIGHT 1344/1344
HYBRID_UPLOAD 21137039360/21137039360
HYBRID_STARTED upload_and_start_seconds=10.000
HYBRID_RESULT prompt_tokens=1 generated_tokens=2 post_start_seconds=4.000 (includes prompt processing)
HYBRID_TOKEN 70253
HYBRID_TOKEN 9
HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated
'''
with tempfile.TemporaryDirectory(prefix='hybrid-result-',dir='/private/tmp') as directory:
 path=Path(directory)/'trial.log'
 def run(text):
  path.write_text(text)
  return subprocess.run([sys.executable,str(root/'analyze_q38_hybrid_full_trial.py'),str(path)],capture_output=True,text=True)
 r=run(good);assert r.returncode==0 and 'COMPARE match' in r.stdout and 'outputs_per_second=0.500000' in r.stdout
 r=run(good.replace('HYBRID_TOKEN 70253','HYBRID_TOKEN 17'));assert r.returncode==0 and 'COMPARE mismatch' in r.stdout
 r=run(good.replace('generated_tokens=2','generated_tokens=0').replace('HYBRID_TOKEN 70253\nHYBRID_TOKEN 9\n',''))
 assert r.returncode==0 and 'COMPARE unavailable' in r.stdout and 'outputs_per_second=0.000000' in r.stdout
 for value in ('0.000','-2.000','garbage'):
  assert run(good.replace('post_start_seconds=4.000','post_start_seconds='+value)).returncode!=0
 assert run(good.replace('HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated\n','')).returncode!=0
print('PASS hybrid result analyzer: match, mismatch, empty and invalid transcripts; NO DEVICE')
