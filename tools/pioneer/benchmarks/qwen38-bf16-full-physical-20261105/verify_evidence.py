#!/usr/bin/env python3
"""Audit saved full-model closure,not a fresh admission or warm-rate proof."""
import hashlib
import json
from pathlib import Path
import re
import sys

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3];files=here/'remote'
assert json.loads((here/'actual-exits.json').read_text())=={
 'primary_trial_waiter':0,'independent_postflight':0,'whole_archive_copy':0,
 'unchanged_reader_main':0,'unchanged_reader_clean':0}
post=(here/'postflight.stdout').read_text()
assert not (here/'postflight.stderr').read_bytes()
assert 'ALL_TRIAL_PIDS_ABSENT_NO_MATCHING_OWNER' in post
assert post.endswith('PASS_INDEPENDENT_BF16_105_CLOSED_NORMAL041_GEN40_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED\n')
expected={}
for digest,name in re.findall(r'^([0-9a-f]{64})  /boot/home/pioneer-q38-bf16-full-105-a/([^/\n]+)$',post,re.M):
 assert name not in expected;expected[name]=digest
assert len(expected)==15 and {p.name for p in files.iterdir()}==expected.keys()
for name,digest in expected.items():assert hashlib.sha256((files/name).read_bytes()).hexdigest()==digest,name
for name in ('direct-parent.exit','full-offload.exit'):assert (files/name).read_text()=='0\n'
assert (files/'parent.exit').read_text()=='1\n' # preserved PRE-TRIAL launcher refusal,not GPU exit
assert (files/'state').read_text()=='FULL_CARD_HELLO_FOUR_CLOSED_STABLE_RESTORED\n'
for name in ('direct-parent.stderr','full-offload.stderr','launcher.stderr','parent.stdout','parent.stderr'):
 assert not (files/name).read_bytes()
script='pioneer_q38_bf16_full_offload_physical.sh'
assert expected[script]=='aa6baf6d22cd4bdca6521ffce66d46451b49abf9e748a5c7b978c527aaf007b8'
assert (files/script).read_bytes()==(root/'tools/pioneer'/script).read_bytes()
parent=(files/'direct-parent.stdout').read_text()
assert parent.endswith('PASS_FULL_CARD_HELLO_FOUR_STABLE_RESTORED_NO_ASSISTANT_INSTALL_OR_SPEED_CLAIM\n')
assert parent.count('HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION\n')==2
assert parent.count('QUIESCED_CLOSED;NEW_HARDWARE_WORK_BLOCKED_UNTIL_RELOAD\n')==2
statuses=re.findall(r'^P100A_RELOAD driver55 tag([0-9]{16}) generation([0-9]+) quiesced([01]) cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY$',parent,re.M)
assert len(statuses)==12 and {int(g) for _,g,_ in statuses}=={38,39,40}
for tag,generation,_ in statuses:assert tag==('2026110500010105' if generation=='39' else '2026102100010012')
assert 'generation40 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' in post
assert len(re.findall(r'^64 .*running at',post,re.M))==1 and '/ 137034936320)' in post
assert '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403  /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe' in post
for digest in ('b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f',
 'd1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229',
 '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
 'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
 '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933',
 '0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af',
 'a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16',
 '9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389',
 '69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2'):
 assert re.search(r'^'+digest+r'  /boot/home/[^\n]+$',post,re.M),digest
for name in ('reader-main','reader-clean'):assert not (here/(name+'.stderr')).read_bytes()
assert (here/'reader-main.stdout').read_bytes()==(here/'reader-clean.stdout').read_bytes()
sys.path.insert(0,str(root/'tools/pioneer/tests'))
from parse_pioneer_q38_full_offload_log import parse_complete,stable_text
parsed=parse_complete(stable_text(files/'full-offload.stdout'),stable_text(files/'full-offload.stderr'))
assert parsed==json.loads((here/'reader-main.stdout').read_text().splitlines()[0])
assert parsed['upload_events']==2257 and parsed['uploaded_bytes']==19399024640 and parsed['output_tokens']==4
assert parsed['reported_post_start_ack_seconds']=='216.622' and parsed['reported_generate_total_seconds']=='2007.300'
assert parsed['warm_token_rate'] is None and not parsed['broad_model_quality']
print('PASS SAVED real27B BF16/expected4tokens/actual exits0/15complete remote hashes/independent normal041-gen40/'
 '64CPU-fullRAM-Assistant-runtime preserved/unchanged main+clean readers;216.622s PROMPT+GENERATION,NOT warm TPS or speedup')
