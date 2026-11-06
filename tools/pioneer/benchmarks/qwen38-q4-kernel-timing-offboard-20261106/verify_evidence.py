#!/usr/bin/env python3
"""Check saved OFFBOARD receipts only. Never accesses Pioneer or a device."""
import hashlib
import json
from pathlib import Path
import re

assert __debug__
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
manifest=json.loads((HERE/'evidence-sha256.json').read_text())
for name,expected in manifest.items():
    data=(HERE/name).read_bytes()
    assert len(data)==expected['bytes'] and hashlib.sha256(data).hexdigest()==expected['sha256'],name
comparison=json.loads((HERE/'comparison.json').read_text())
main=json.loads((HERE/'actual-exits.json').read_text())
clean=json.loads((HERE/'clean.actual-exits.json').read_text())
assert main==clean and len(main)==14 and set(main.values())=={0}
assert set(main)==set(comparison['children'])
for name,value in comparison['children'].items():
    assert value['actual_exit_main']==value['actual_exit_clean']==value['stderr_bytes']==0
    if (HERE/(name+'.stdout')).exists():
        assert hashlib.sha256((HERE/(name+'.stdout')).read_bytes()).hexdigest()==value['stdout_sha256']
sources=(HERE/'source-sha256.txt').read_text().splitlines()
assert len(sources)==comparison['source_files']==64
for line in sources:
    digest,name=line.split('  ',1)
    assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest,name
assert len(comparison['identical_device_artifacts_main_clean'])==12
for width in (5120,17408):
    body=(HERE/('build'+str(width)+'.stdout')).read_text()
    for name in ('boot.bin','text.bin','program.elf'):
        record=comparison['identical_device_artifacts_main_clean'][f'image{width}/{name}']
        assert f'Q4_KERNEL_TIMING_INSPECT {width} {name} {record["bytes"]} {record["sha256"]}' in body
    assert comparison['identical_device_artifacts_main_clean'][f'image{width}/text.bin']['bytes']==7972
    assert comparison['identical_device_artifacts_main_clean'][f'image{width}/unchanged-compact/text.bin']['bytes']==7644
    image=(HERE/('image'+str(width)+'.stdout')).read_text().splitlines()
    assert len(image)==4
    for mode,line in enumerate(image):
        reads=48 if mode==1 else 12
        assert line==f'PASS_Q4_TIMING_ACTUAL_LINKED_RV32 width{width} clockCase{mode} exactWords64 csrReads{reads} timerStores16 stack464;SYNTHETIC_CLOCK_NOT_CARD_TIMING'
assert (HERE/'data.stdout').read_text()=='PASS_Q4_KERNEL_TIMING_DATA cases57 wrap1 unavailableNeverInventsCycles1 bounds1;NO_HARDWARE_AUTHORITY\n'
assert (HERE/'fault.stdout').read_text()=='PASS_Q4_TIMING_LINKED_FAULTS refused26 numericalMismatches4 noIOAfterPark1;SYNTHETIC_NO_HARDWARE_AUTHORITY\n'
model=(HERE/'model.stdout').read_text().splitlines()
assert len(model)==11 and model[0]=='COMPACT_FULL_K_AUTH_SAME_OPEN_FULL_ORIGINAL_READ_ONLY_NO_CACHE_OR_DEVICE'
for line in model[1:10]:
    assert re.fullmatch(r'PASS_PRIVATE_ACTUAL_COMPACT_FULL_K tensor[678] width(5120|17408) first[0-9]+ twoJobs_all64WordsExact_persistentAccumulator_actualNoC_L1_RV32;NO_REAL_PROMPT_QUALITY_OR_SPEED',line)
assert model[-1]=='PASS_PRIVATE_COMPACT_FULL_K_STREAM cases9_jobs18_words576_originalWeights2654208_sameOpenFullSHA_epochNanoIdentityStable_NO_NATIVE_MODEL_QUALITY_OR_SPEED'
print('PASS SAVED Q4 timing OFFBOARD:14main+14clean children0/64sources/12sameGPUartifacts/576originalWords/8syntheticClockCases/57DATA/26fault+4numeric/34roleCSRrefusals;NO_NATIVE_OR_CARD_TIMING_OR_MODEL_TPS')
