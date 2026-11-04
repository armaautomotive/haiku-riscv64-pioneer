#!/usr/bin/env python3
"""Read-only frozen ACTUAL85 records;NEVER hardware admission/retry authority."""
import hashlib
import pathlib
from pioneer_native_q4_fpu_output import verify
assert __debug__
folder=pathlib.Path(__file__).resolve().parents[1]/'benchmarks/qwen38-native-q4-fpu-physical-20261104'
pins={'parent.stdout':'3144be41ed6e610b654e65c96515f5491904aadee34afea8a9ceeeaffb28f072',
      'q4.stdout':'c2a3f6e22adbf9260bfae6c64761329b2daeb136ffbed7357afc5a95251584eb',
      'postflight.stdout':'fcfb32835d8b2c841fe591d2b97be398d48fc19246ea2b98c0f0dc5f01056b23',
      'parent.source.sh':'713fb9d61e6d41b9106ae41f09d92a6248ca73056835d442ba4c9572b199377d'}
for name,digest in pins.items():assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,name
for name in ('parent.stderr','q4.stderr','postflight.stderr'):assert (folder/name).read_bytes()==b''
for name in ('parent.exit','q4.exit','postflight.exit'):assert (folder/name).read_bytes()==b'0\n'
assert (folder/'state').read_text()=='STABLE041_RESTORED_Q4_BLOCKS_EXACT192_PASSED\n'
assert (folder/'parent.pid').read_text()=='1172\n'
parent=(folder/'parent.stdout').read_text();child=(folder/'q4.stdout').read_text();post=(folder/'postflight.stdout').read_text()
assert len(parent.splitlines())==234 and len(child.splitlines())==198 and parent.count(child)==1
assert verify(child)=={'samples':3,'words':192,'ends':3,'generation':7,'polls':[1,1,1]}
assert parent.endswith('NATIVE_Q4_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM\n')
meta='P100A_RELOAD driver55 tag2026102100010012 generation8 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
assert meta in parent and meta in post
for text in (parent,post):
    assert '64 (Unknown), revision 0000 running at 1000MHz' in text
    assert '/ 137034936320)' in text
assert '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403  /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe' in post
for digest,path in [('861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74','servers/HaikuAssistantQwen27'),
                    ('c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18','servers/HaikuAssistantService'),
                    ('0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933','apps/HaikuAssistant')]:
    assert digest+'  /boot/home/config/non-packaged/'+path in post
print('PASS frozen ACTUAL85 records:original parent98591 CLOSED0,192 exact real words/three ENDs,normal041/gen8 restored,64 CPUs/full RAM;NOT new hardware admission/retry/model-quality/full-native-inference/speed proof')
