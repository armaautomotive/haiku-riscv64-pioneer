#!/usr/bin/env python3
"""Check saved OFFBOARD receipts; never a hardware/completion grant."""
import hashlib
import json
from pathlib import Path
import re

here=Path(__file__).resolve().parent
data=json.loads((here/'comparison.json').read_text())
assert data['sources']==67 and len(data['artifacts'])==26 and len(data['children'])==14
for name,record in data['archived_main_files'].items():
    body=(here/name).read_bytes()
    assert len(body)==record['bytes'] and hashlib.sha256(body).hexdigest()==record['sha256'],name
exits=json.loads((here/'actual-exits.json').read_text())
assert set(exits)==set(data['children']) and set(exits.values())=={0}
for name,record in data['children'].items():
    assert record['main_exit']==record['clean_exit']==record['both_stderr_bytes']==0
    path=here/(name+'.stdout')
    if path.exists():assert hashlib.sha256(path.read_bytes()).hexdigest()==record['same_stdout_sha256']
for width,old,next_steps,words,port in ((5120,887866,840475,194668,36294),(17408,2744058,2603259,661228,123078)):
    lines=(here/f'pair_issue_image{width}.stdout').read_text().splitlines()
    assert lines==[
        f'PAIRED_RV32_OBSERVED width{width} oldSteps{old} pairedSteps{next_steps}',
        f'PASS_PAIRED_ACTUAL_RV32 width{width} sameTensixWords{words} oldSteps{old} pairedSteps{next_steps} oldPort{port} pairedPort{port} oldStack464 pairedStack432;COUNTS_NOT_CARD_CYCLES_OR_TPS',
        f'PASS_PAIRED_FAULTS width{width} refused26 numericalMismatches4 unavailableClocks3 noIOAfterPark1;NO_NATIVE_AUTHORITY']
    assert next_steps<old
    prefix=f'image{width}/'
    assert data['artifacts'][prefix+'text.bin']['bytes']==7820
    assert data['artifacts'][prefix+'unchanged-timing/text.bin']['bytes']==7972
    assert data['artifacts'][prefix+'unchanged-timing/unchanged-compact/text.bin']['bytes']==7644
model=(here/'model.stdout').read_text().splitlines()
assert len(model)==11 and model[0]=='COMPACT_FULL_K_AUTH_SAME_OPEN_FULL_ORIGINAL_READ_ONLY_NO_CACHE_OR_DEVICE'
for line in model[1:-1]:
    assert re.fullmatch(r'PASS_PRIVATE_ACTUAL_COMPACT_FULL_K tensor[678] width(5120|17408) first\d+ twoJobs_all64WordsExact_persistentAccumulator_actualNoC_L1_RV32;NO_REAL_PROMPT_QUALITY_OR_SPEED',line)
assert model[-1]=='PASS_PRIVATE_COMPACT_FULL_K_STREAM cases9_jobs18_words576_originalWeights2654208_sameOpenFullSHA_epochNanoIdentityStable_NO_NATIVE_MODEL_QUALITY_OR_SPEED'
assert (here/'refusals.txt').read_text()=='PASS pairedRoleRefusals28;NO_NATIVE_AUTHORITY\n'
assert len((here/'source-sha256.txt').read_text().splitlines())==67
print('PASS SAVED OFFBOARD pairedQ4/14childrenMainClean/67sources/26artifacts/576exactWords/5percentFewerRV32Steps_sameTensixStream;NOT_CARD_TIMING_OR_MODEL_TPS')
