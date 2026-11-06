#!/usr/bin/env python3
"""Saved107 physical receipts only. No Pioneer/device access or replay."""
import hashlib
import json
from pathlib import Path
import re
import sys

assert __debug__
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[1]/'tests'))
from pioneer_native_compact_q4_timing_output import verify,verify_parent
record=json.loads((HERE/'manifest.json').read_text())
assert record['hardware_source']=='e83a3bf83e294e2f2e4ee05db61677ed4358bd78'
assert record['actual_primary_exit']==record['actual_independent_postflight_exit']==record['actual_captured_postflight_exit']==0
for name,expected in record['files'].items():
    data=(HERE/name).read_bytes()
    assert len(data)==expected['bytes'] and hashlib.sha256(data).hexdigest()==expected['sha256'],name
for name in ('direct-parent.exit','compact-q4-stream.exit'):assert (HERE/name).read_text()=='0\n'
for name in ('direct-parent.stderr','compact-q4-stream.stderr','postflight.stderr'):assert (HERE/name).stat().st_size==0
assert (HERE/'state').read_text()=='STABLE041_RESTORED_COMPACT_Q4_TIMING_EXACT128\n'
assert (HERE/'parent.pid').read_text()=='37996\n' and (HERE/'direct-launcher.pid').read_text()=='37995\n'
child=(HERE/'compact-q4-stream.stdout').read_text();parent=(HERE/'direct-parent.stdout').read_text()
result=verify_parent(parent);assert result==record['observed']
assert verify(child)['timing']==result['timing'] and parent.count(child)==1
assert result['words']==128 and result['ends']==2 and result['restored_generation']==44
assert [value['cycles'] for value in result['timing']]==[690318,690195,2134279,2134325]
post=(HERE/'postflight.stdout').read_text()
assert post.startswith('ALL107_PIDS_ABSENT_NO_MATCHING_OWNER\n')
assert post.endswith('PASS_INDEPENDENT107_NORMAL041_GEN44_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED\n')
assert 'P100A_RELOAD driver55 tag2026102100010012 generation44 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n' in post
assert '64 (Unknown), revision 0000 running at 1000MHz\n' in post and ' / 137034936320)' in post
stage='/boot/home/pioneer-native-compact-q4-timing107-a/'
for name in ('direct-parent.stdout','direct-parent.stderr','direct-parent.exit','compact-q4-stream.stdout',
             'compact-q4-stream.stderr','compact-q4-stream.exit','state','parent.pid','direct-launcher.pid'):
    match=re.search(r'^([0-9a-f]{64})  '+re.escape(stage+name)+r'$',post,re.M)
    assert match and match[1]==record['files'][name]['sha256'],name
for digest in ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
               '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
               'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
               '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933',
               '0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af',
               'a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16',
               '9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389',
               '69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2'):
    assert re.search(r'^'+digest+r'  /',post,re.M)
print('PASS SAVED actual107/128exactWords/2cleanENDs/4physicalCycleRecords/wholeHashes+actualExits0/independentNormal041-gen44/64CPU-fullRAM-Assistant-runtime;NOT modelTPS_or20TPS')
