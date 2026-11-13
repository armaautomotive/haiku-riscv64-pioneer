#!/usr/bin/env python3
"""Actual dense diagnostic parent flow/output gate; fake endpoints only."""
import pathlib
import re
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
source=(here.parent/'pioneer_recurrent_band_physical_20261112.sh').read_text()
flow=source.split('# RECURRENT_BAND_FLOW_BEGIN\n',1)[1].split('# RECURRENT_BAND_FLOW_END',1)[0]
rows=(0,3072,6112)
reference=[];normal=['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
 'AUTHENTICATED_FIXED_MODEL c600de03 tensor3 width5120 rows6144 samples3 payload_and_reference_exact;DEBUG_NOT_WORK']
positions=[]
for sample,first in enumerate(rows):
 for row in range(32):reference.append(f'RECURRENT_INTEGER_REFERENCE sample{sample} first{first} row{row} word{sample*32+row+1:08x}')
 for job in range(2):
  for row in range(32):
   positions.append(len(normal));word=f'{sample*32+row+1:08x}'
   normal.append(f'NATIVE_LEARNED_BAND_RETURNED_WORD sample{sample} first{first} pass{job} row{row} got{word} expected{word}')
 normal.append(f'PASS_NATIVE_LEARNED_BAND sample{sample} tensor3 first{first} width5120 phases4 dual_jobs2 exact_words64 polls1 generation37;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
normal.append('PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
with tempfile.TemporaryDirectory(prefix='recurrent-band-parent-',dir='/private/tmp') as tmp:
 folder=pathlib.Path(tmp);client=folder/'client'
 client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-learned-bands ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then echo UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED >&2;fi
if [ "$mode" = falsepass ];then exit 1;fi
exit 0
''');client.chmod(0o700)
 prefix='''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;reference=$stage/reference
export stage events mode
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
'''
 program=folder/'flow.sh';program.write_text(prefix+flow)
 cases=[('pass',normal),('runfail',[]),('dirty',normal),('falsepass',normal),('swapfail',normal),('busy',normal),
        ('missing-auth',normal[1:]),('early-final',[normal[-1]]+normal[:-1]),('extra-final',normal+[normal[-1]]),('bad-ref',normal)]
 for number,position in enumerate(positions):
  cases.append((f'missing-{number}',normal[:position]+normal[position+1:]))
  cases.append((f'duplicate-{number}',normal[:position]+[normal[position]]+normal[position:]))
  changed=list(normal);changed[position]=re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}','got00000000 expected00000000',changed[position]);cases.append((f'wrong-{number}',changed))
 for sample in range(3):
  marker=66+sample*65
  for label,old,new in (('generation','generation37','generation36'),('poll','polls1','polls0'),('geometry','width5120','width6144')):
   changed=list(normal);changed[marker]=changed[marker].replace(old,new);cases.append((f'{label}-{sample}',changed))
 for mode,output in cases:
  stage=folder/mode;stage.mkdir();(stage/'reference').write_text('' if mode=='bad-ref' else '\n'.join(reference)+'\n')
  (stage/'output').write_text('\n'.join(output)+'\n')
  result=subprocess.run(['sh',str(program),str(stage),str(client),mode],text=True,capture_output=True)
  events=(stage/'events').read_text().splitlines()
  restore='exchange:2026111200010147:2026102100010012'
  if mode=='pass':
   assert result.returncode==0 and events.count('run')==1 and events.count(restore)==1,(result.stdout,result.stderr)
   assert (stage/'recurrent.state').read_text().strip()=='STABLE_RESTORED_RECURRENT_BANDS_EXACT192_PASSED'
  else:
   assert result.returncode!=0 and restore not in events,(mode,events,result.stdout,result.stderr)
   if 'run' in events:assert events[-1]=='run',(mode,events)
 idle=source.split('idle(){\n',1)[1].split('\n}\n',1)[0]
 idle_program=folder/'idle.sh'
 idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n'+idle+'\n}\nidle\n')
 base='Id Team\n1 kernel_team\nSELF /boot/home/pioneer-recurrent-band-20261112-a/parent\n'
 inventories=((base,True),(base+'99 /boot/system/Tracker\n',True),
              (base+'99 /boot/home/HaikuAssistantService\n',False),
              (base+'99 /boot/home/pioneer-recurrent-band-diagnostic147\n',False),
              ('Id Team\n',False),(base+'malformed\n',False))
 for index,(text,accepted) in enumerate(inventories):
  inventory=folder/('inventory-'+str(index));inventory.write_text(text)
  result=subprocess.run(['sh',str(idle_program),str(inventory)],text=True,capture_output=True)
  assert (result.returncode==0)==accepted,(text,result.stderr)
 cache_body=source.split('cache_verified(){\n',1)[1].split('\n}\n',1)[0]
 cache_body=cache_body.replace('/boot/home/pioneer-recurrent-q8-cache-verify-20261112-a','$1')
 cache_program=folder/'cache-check.sh'
 cache_program.write_text('''set -eu
stat(){ case "$*" in *"%u:%a"*) printf '0:700\\n';; *) printf '0:1:644\\n';; esac; }
cache_verified(){
'''+cache_body+'''\n}
cache_verified "$1"
''')
 cache=folder/'cache';cache.mkdir();(cache/'stdout').write_text('RECURRENT_Q8_CACHE_MATCH fragments=1008 pages=1436160 raw=5882511360 padded=5882511360; NO DEVICE OR SPEED\n')
 (cache/'stderr').write_text('');(cache/'exit').write_text('0\n')
 def cache_ok():return subprocess.run(['sh',str(cache_program),str(cache)],capture_output=True).returncode==0
 assert cache_ok()
 (cache/'exit').write_text('1\n');assert not cache_ok();(cache/'exit').write_text('0\n')
 (cache/'stderr').write_text('error\n');assert not cache_ok();(cache/'stderr').write_text('')
 (cache/'stdout').write_text('incomplete\n');assert not cache_ok()
 (cache/'stdout').unlink();(cache/'stdout').symlink_to(cache/'stderr');assert not cache_ok()
print(f'PASS real recurrent-parent flow and {len(cases)} synthetic success/failure gate cases; no hardware')
