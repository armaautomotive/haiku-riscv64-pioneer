#!/usr/bin/env python3
"""Execute only the parent's DATA output gate,never its hardware actions."""
from pathlib import Path
import subprocess
parent=Path(__file__).resolve().parents[1]/'pioneer_native_factored_q4_compare_physical.sh'
source=parent.read_text()
gate=source.split('# FACTORED_COMPARE_OUTPUT_GATE_BEGIN\n',1)[1].split('# FACTORED_COMPARE_OUTPUT_GATE_END',1)[0]
assert gate.startswith("awk '\n") and gate.endswith("' \"$stage/factored-q4-compare.stdout\" || exit 1\n")
program=gate[len("awk '"):].rsplit("' \"$stage/",1)[0]
def receipt(state=1):
 rows=['FACTORED_COMPARE_AUTHENTICATING_ORIGINAL_AND_BOTH_CPU_REFERENCES;NO_DEVICE_OPEN',
 'FACTORED_COMPARE_AUTHENTICATED c600de03 all9_originalPayloads_bothArithmeticReferences;CPU_DIAGNOSTIC_NOT_WORK']
 for sample,width in ((0,5120),(6,17408)):
  for word in range(64):rows.append(f'FACTORED_COMPARE_WORD sample{sample} kind{word//32} row{word%32} got00000000 expected00000000')
  n=width//256
  rows.append(f'PASS_NATIVE_FACTORED_COMPARE sample{sample} width{width} exact64 generation51 saved_l137136 saved_gddr{n*1280} requests{2*n*26} flits{2*n*80};EXPLICIT_END_L1_GDDR_NIU_TLB_FIFO_CLOCK_PCI_RESTORED_NOT_MODEL_TPS')
  for kind in range(2):rows.append(f'FACTORED_COMPARE_TIMING sample{sample} width{width} kind{kind} '+
   ('state1 start1 end2 cycles1;0OLD_BF16_1NEW_FACTORED_NOT_MODEL_TPS' if state==1 else f'state{state};UNAVAILABLE_NOT_TIMING_OR_CLEANUP_AUTHORITY'))
 rows.append('PASS_NATIVE_FACTORED_COMPARE_ALL samples2 exact128 cleanEnds2;DIAGNOSTIC_NOT_WORK_NOT_MODEL_QUALITY_OR_TPS')
 return '\n'.join(rows)+'\n'
def run(data):return subprocess.run(['awk',program],input=data,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE).returncode
for state in (0,1,2):assert run(receipt(state))==0
good=receipt();bad=[]
lines=good.splitlines()
for i in range(len(lines)):bad.append('\n'.join(lines[:i]+lines[i+1:])+'\n')
bad += [good+lines[3]+'\n',good.replace('generation51','generation43'),good.replace('saved_l137136','saved_l126908'),
 good.replace('got00000000','got00000001',1),good.replace('kind1 row0','kind0 row0',1),
 good.replace('sample6','sample7'),good.replace('requests1040','requests1039'),good+'UNEXPECTED\n']
for value in bad:assert run(value)!=0
assert source.count('mkdir "$stage/.trial-once"')==1 and '--fixed-old-new-q4' in source
assert '50 51 "$candidate"' in source and '51 52 "$stable"' in source
assert not any(w in source for w in ('pioneer_power','shutdown','reboot','rm -'))
subprocess.run(['sh','-n',str(parent)],check=True)
print(f'PASS_COMPARE_PARENT_DATA_GATE positive3 missingOrChangedRefusals{len(bad)};NO_HARDWARE_EXECUTION')
