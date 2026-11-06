#!/usr/bin/env python3
"""Verify saved108 receipts and comparison DATA;not live hardware authority."""
import hashlib
import json
from pathlib import Path
import sys

here=Path(__file__).resolve().parent
sys.path.insert(0,str(here.parents[1]/'tests'))
from pioneer_native_compact_q4_timing_output import verify,verify_parent
from pioneer_native_compact_q4_stream_parent_output import health,metadata

data=json.loads((here/'manifest.json').read_text())
assert data['hardware_source']=='bebac899260f1b97e922d4d512440f2c21392d7e'
assert data['actual_primary_exit']==data['actual_independent_postflight_exit']==0
assert len(data['files'])==11
for name,pin in data['files'].items():
    body=(here/name).read_bytes()
    assert len(body)==pin['bytes'] and hashlib.sha256(body).hexdigest()==pin['sha256'],name
child=verify((here/'compact-q4-stream.stdout').read_text(),True)
parent=verify_parent((here/'direct-parent.stdout').read_text(),True)
assert dict(child,restored_generation=46)==parent==data['observed']
assert parent['words']==128 and parent['ends']==2 and parent['polls']==[1,1]
assert [r['cycles'] for r in parent['timing']]==[643321,643229,1994431,1994512]
for name in ('direct-parent.exit','compact-q4-stream.exit'):assert (here/name).read_text()=='0\n'
for name in ('direct-parent.stderr','compact-q4-stream.stderr'):assert not (here/name).stat().st_size
assert (here/'state').read_text()=='STABLE041_RESTORED_COMPACT_Q4_PAIR_ISSUE_EXACT128\n'
assert (here/'parent.pid').read_text()=='38712\n' and (here/'direct-launcher.pid').read_text()=='38711\n'
post=(here/'postflight.stdout').read_text().splitlines()
assert len(post)==35 and post[0]=='ALL108_PIDS_ABSENT_NO_MATCHING_OWNER'
assert post[11]==metadata('2026102100010012',46)
health(post[12:21])
assert post[-1]=='PASS_INDEPENDENT108_NORMAL041_GEN46_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED'
for name,pin in data['files'].items():
    if name.startswith('postflight'):continue
    assert pin['sha256']+'  /boot/home/pioneer-native-compact-q4-pair-issue108-a/'+name in post
old=json.loads((here.parent/'qwen38-q4-kernel-timing-physical-20261106'/'manifest.json').read_text())['observed']['timing']
for record in data['comparison']:
    sample=record['sample'];previous=[r['cycles'] for r in old if r['sample']==sample]
    now=[r['cycles'] for r in parent['timing'] if r['sample']==sample]
    assert previous==record['baseline_cycles'] and now==record['candidate_cycles'] and max(now)<min(previous)
    assert record['mean_cycle_reduction_percent']==100*(1-sum(now)/sum(previous))
print('PASS SAVED actual108/128exactWords/2cleanENDs/6.55..6.81percentFewerKernelCycles/normal041-gen46/64CPU-fullRAM-AssistantPreserved;NOT_MODEL_TPS_OR20TPS')
