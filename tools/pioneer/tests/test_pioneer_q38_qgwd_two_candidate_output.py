#!/usr/bin/env python3
"""Tag142 transcript shape and fail-closed tests with fabricated data only."""
from pathlib import Path
import subprocess

gate=Path(__file__).resolve().parent.parent/'pioneer_q38_qgwd_two_candidate_output.awk'
rows=['HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet',
 'HYBRID_PREFLIGHT 21/1351','HYBRID_PREFLIGHT 1351/1351',
 'HYBRID_UPLOAD 268435456/21094223872','HYBRID_UPLOAD 21094223872/21094223872',
 'HYBRID_START_REQUEST diagnostic only; no completion implied','HYBRID_STARTED upload_and_start_seconds=24.000',
 'HYBRID_PENDING seconds=30.002 pc_first=456 pc_second=789; diagnostic only',
 'HYBRID_RESULT prompt_tokens=1 generated_tokens=2 inference_seconds=10.000 (includes START and prompt processing)',
 'QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.200000','HYBRID_TOKEN 42','HYBRID_TOKEN 43',
 'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated']
def run(lines):return subprocess.run(['awk','-f',str(gate)],input='\n'.join(lines)+'\n',text=True,capture_output=True)
assert run(rows).returncode==0 and 'outputs=2 first_token=42' in run(rows).stdout
one=rows[:8]+[rows[8].replace('generated_tokens=2','generated_tokens=1')]+rows[9:11]+rows[-1:]
assert run(one).returncode!=0
for eos in (248044,248046,248063,248064,248065):
    early=one[:10]+['HYBRID_TOKEN '+str(eos)]+one[11:]
    assert run(early).returncode==0
bad=[rows[:-1],rows[:2]+rows[3:],rows[:4]+rows[5:],rows[:5]+rows[6:],rows[:9]+rows[10:],
 rows[:10]+['HYBRID_TOKEN 248320']+rows[11:],rows[:8]+[rows[8].replace('generated_tokens=2','generated_tokens=0')]+rows[9:],
 rows[:8]+[rows[8].replace('generated_tokens=2','generated_tokens=3')]+rows[9:],rows[:11]+rows[12:],
 rows[:10]+['HYBRID_TOKEN 248044']+rows[11:],
 rows[:5]+[rows[5],rows[5]]+rows[6:],rows[:8]+[rows[8].replace('inference_seconds=10.000','inference_seconds=0.000')]+rows[9:],
 [s.replace('21094223872','21167624192') for s in rows],rows+['unexpected']]
for case in bad:assert run(case).returncode!=0,case
print('PASS QGWD two-token transcript shapes, early-EOS and malformed cases; synthetic only')
