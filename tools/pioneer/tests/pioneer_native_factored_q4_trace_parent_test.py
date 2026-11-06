#!/usr/bin/env python3
"""Pure DATA-gate tests;never execute the physical parent or a device."""
from pathlib import Path
import subprocess
import tempfile
source=(Path(__file__).resolve().parents[1]/'pioneer_native_factored_q4_trace_physical.sh').read_text()
gate=source.split('# FACTORED_TRACE_OUTPUT_GATE_BEGIN\n',1)[1].split('# FACTORED_TRACE_OUTPUT_GATE_END',1)[0]
assert gate.startswith("awk '") and gate.endswith('"$stage/factored-q4-trace.stdout" || exit 1\n')
program=gate[5:gate.rindex("' \"$stage/")]
rows=['FACTORED_TRACE_AUTHENTICATING_ORIGINAL_ALL_PAYLOADS_AND_STAGE_REFERENCE;NO_DEVICE_OPEN',
 'FACTORED_TRACE_AUTHENTICATED c600de03 all9_originalPayloads_fixedSample0_stages;CPU_DIAGNOSTIC_NOT_WORK',
 'FACTORED_TRACE_RETURNED_COMPLETED_DATA',
 'FACTORED_TRACE_DATA_ONLY words64 final32_exact_required hashes_diagnostic_only;NOT_END_OR_SPEED_AUTHORITY']
for i in range(64):
    kind='final' if i<32 else 'block' if i<52 else 'stage' if i<56 else 'first'
    slot=i if i<32 else i-32 if i<52 else i-52 if i<56 else i-56
    rows.append(f'FACTORED_TRACE_WORD index{i} kind{kind} slot{slot} got00000000 expected00000000 exact1')
rows+=['FACTORED_TRACE_DATA_DONE mismatches0 first64;DATA_ONLY_NOT_END_OR_SPEED_AUTHORITY',
 'PASS_NATIVE_FACTORED_TRACE sample0 width5120 exactFinal32_trace32 generation3 saved_l137136 saved_gddr25600 requests1040 flits3200;EXPLICIT_END_L1_GDDR_NIU_TLB_FIFO_CLOCK_PCI_RESTORED_NOT_MODEL_TPS']
def accepted(lines):
    result=subprocess.run(['awk',program],input='\n'.join(lines)+'\n',text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    assert not result.stdout and not result.stderr
    return result.returncode==0
assert accepted(rows);refused=0
for i in range(len(rows)):
    assert not accepted(rows[:i]+rows[i+1:]);refused+=1
for old,new in [('generation3','generation51'),('width5120','width17408'),('exact1','exact0'),('got00000000','got00000001'),('COMPLETED_DATA','UNPROVEN_DATA_NO_ACCEPTANCE'),('mismatches0','mismatches1'),('saved_l137136','saved_l126908'),('stage','final')]:
    altered=[r.replace(old,new) for r in rows]
    assert altered!=rows and not accepted(altered);refused+=1
assert not accepted(rows+[rows[-1]]);refused+=1
subprocess.run(['sh','-n',str(Path(__file__).resolve().parents[1]/'pioneer_native_factored_q4_trace_physical.sh')],check=True)
print('PASS_TRACE112_PARENT_DATA_GATE complete64_words_and_explicitClose_only;missingChangedDuplicateRefused'+str(refused)+';NO_HARDWARE')
