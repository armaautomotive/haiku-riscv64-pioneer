#!/usr/bin/env python3
"""Synthetic tag133 transcript controls; no device or numerical proof."""
from pathlib import Path
import subprocess
from test_pioneer_q38_qgw8_trace_output import rows

gate=Path(__file__).resolve().parent.parent/'pioneer_q38_qgw8_dense_input_output.awk'
rows=rows[:-1]+['QGW8_DENSE_INPUT_SNAPSHOT sha256='+'a'*64+
                ' bytes=1310720 tag=0x2026110700010133 generation=11; DATA_ONLY_AFTER_CLOSE']
def run(lines):return subprocess.run(['awk','-f',str(gate)],input='\n'.join(lines)+'\n',text=True,capture_output=True)
good=run(rows);assert good.returncode==0 and 'outputs=1 token=108133' in good.stdout
bad=[rows[:-1],rows[:1]+rows[2:],rows[:2]+rows[3:],rows[:3]+rows[4:],
     rows[:3]+[rows[3],rows[3]]+rows[4:],rows[:7]+rows[8:],
     rows[:6]+[rows[6].replace('generated_tokens=1','generated_tokens=0')]+rows[9:],
     rows[:8]+['HYBRID_TOKEN 248320']+rows[9:],rows[:8]+[rows[8],rows[8]]+rows[9:],
     rows[:10]+[rows[10].replace('a'*64,'a'*63)],
     rows[:10]+[rows[10].replace('bytes=1310720','bytes=1310719')],
     rows[:10]+[rows[10].replace('generation=11','generation=7')],
     rows[:10]+[rows[10].replace('0133','0131')],rows+['unexpected']]
for case in bad:assert run(case).returncode!=0,case
print('PASS tag133 dense-input transcript and malformed cases; synthetic only')
