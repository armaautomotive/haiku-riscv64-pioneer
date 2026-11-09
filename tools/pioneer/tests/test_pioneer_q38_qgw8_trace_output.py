#!/usr/bin/env python3
"""Synthetic transcript controls; no device or numerical qualification."""
from pathlib import Path
import subprocess

gate=Path(__file__).resolve().parent.parent/'pioneer_q38_qgw8_trace_output.awk'
rows=['HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet',
      'HYBRID_PREFLIGHT 1351/1351','HYBRID_UPLOAD 21167624192/21167624192',
      'HYBRID_START_REQUEST diagnostic only; no completion implied','HYBRID_STARTED upload_and_start_seconds=100.000',
      'HYBRID_PENDING seconds=30.000 pc_first=1 pc_second=2; diagnostic only',
      'HYBRID_RESULT prompt_tokens=1 generated_tokens=1 inference_seconds=10.000 (includes START and prompt processing)',
      'QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.100000','HYBRID_TOKEN 108133',
      'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated',
      'QGW8_TRACE_SNAPSHOT sha256='+'a'*64+' bytes=1310720 tag=0x2026110700010131 generation=7; DATA_ONLY_AFTER_CLOSE']
def run(lines):return subprocess.run(['awk','-f',str(gate)],input='\n'.join(lines)+'\n',text=True,capture_output=True)
assert run(rows).returncode==0 and 'outputs=1 token=108133' in run(rows).stdout
zero=rows[:6]+[rows[6].replace('generated_tokens=1','generated_tokens=0')]+rows[9:]
assert run(zero).returncode==0
bad=[rows[:-1],rows[:1]+rows[2:],rows[:2]+rows[3:],rows[:3]+rows[4:],
     rows[:3]+[rows[3],rows[3]]+rows[4:],rows[:7]+rows[8:],rows[:8]+['HYBRID_TOKEN 248320']+rows[9:],
     rows[:8]+[rows[8],rows[8]]+rows[9:],rows[:10]+[rows[10].replace('a'*64,'a'*63)],
     rows[:10]+[rows[10].replace('bytes=1310720','bytes=1310719')],
     rows[:10]+[rows[10].replace('generation=7','generation=8')],
     rows[:10]+[rows[10].replace('0131','0129')],rows+['unexpected']]
for case in bad:assert run(case).returncode!=0,case
print('PASS tag131 trace transcript shape and malformed cases; synthetic only')
