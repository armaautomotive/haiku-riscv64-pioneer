#!/usr/bin/env python3
"""Exercise physical-run transcript gating; no model or device."""
from pathlib import Path
import subprocess

gate=Path(__file__).resolve().parent.parent/'pioneer_q38_hybrid_full_output.awk'
good='''HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet
HYBRID_PREFLIGHT 21/1344
HYBRID_PREFLIGHT 1344/1344
HYBRID_UPLOAD 268435456/21137039360
HYBRID_UPLOAD 21137039360/21137039360
HYBRID_STARTED upload_and_start_seconds=100.123
HYBRID_RESULT prompt_tokens=1 generated_tokens=2 post_start_seconds=3.250 (includes prompt processing)
HYBRID_TOKEN 70253
HYBRID_TOKEN 19
HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated
'''
def check(log):return subprocess.run(['awk','-f',str(gate)],input=log,text=True,capture_output=True)
r=check(good);assert r.returncode==0 and 'outputs=2 first_token=70253 post_start_seconds=3.250' in r.stdout
for bad in (good.replace('HYBRID_PREFLIGHT 1344/1344\n',''),
 good.replace('HYBRID_UPLOAD 21137039360/21137039360\n',''),
 good.replace('HYBRID_STARTED upload_and_start_seconds=100.123\n',''),
 good.replace('generated_tokens=2','generated_tokens=3'),
 good.replace('HYBRID_TOKEN 19','HYBRID_TOKEN 248320'),
 good.replace('post_start_seconds=3.250','post_start_seconds=0.000'),
 good.replace('HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated\n',''),
 good+'unexpected\n',good+'HYBRID_TOKEN 2\n'):
 assert check(bad).returncode!=0,bad
print('PASS hybrid physical output shape gate and nine corruptions; no quality or device proof')
