#!/usr/bin/env python3
"""Exercise the real one-shot physical flow with fake endpoints only."""
from pathlib import Path
import re
import subprocess
import tempfile

here=Path(__file__).resolve().parent
source=(here.parent/'pioneer_small_recurrent_band_physical_20261113.sh').read_text()
assert source.count('# SMALL_RECURRENT_FLOW_BEGIN\n')==source.count('# SMALL_RECURRENT_FLOW_END')==1
flow=source.split('# SMALL_RECURRENT_FLOW_BEGIN\n',1)[1].split('# SMALL_RECURRENT_FLOW_END',1)[0]
with tempfile.TemporaryDirectory(prefix='small-band-parent-',dir='/private/tmp') as tmp:
 folder=Path(tmp);preflight=folder/'preflight';client=folder/'client'
 preflight.write_text('''#!/bin/sh
echo preflight >> "$events"
if [ "$mode" = sourcefail ];then exit 1;fi
if [ "$mode" = sourcedirty ];then echo bad >&2;fi
[ "$mode" = sourcemissing ] || echo "$source_pass"
''');preflight.chmod(0o700)
 client.write_text('''#!/bin/sh
echo run >> "$events"
if [ "$mode" = runfail ];then exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo bad >&2;fi
[ "$mode" != falsepass ]
''');client.chmod(0o700)
 prefix='''set -eu
stage=$1;preflight=$2;client=$3;mode=$4;tensor=$5;events=$stage/events;reference=$stage/reference
export stage events mode source_pass
source_pass=SOURCE_PASS;generation=42;candidate_generation=43;restored_generation=44
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable;candidate_tag=2026111300010149
cp(){ :; }
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
'''
 program=folder/'flow.sh';program.write_text(prefix+flow)
 for tensor in (11,12):
  first=(0,32,0);reference=[];normal=['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
   f'AUTHENTICATED_FIXED_MODEL c600de03 tensor{tensor} width5120 rows48 samples3 payload_and_reference_exact;DEBUG_NOT_WORK']
  positions=[]
  for s in range(3):
   for r in range(32):reference.append(f'SMALL_INTEGER_REFERENCE sample{s} first{first[s]} row{r} word00000000')
   for p in range(2):
    for r in range(32):
     positions.append(len(normal));normal.append(f'NATIVE_LEARNED_BAND_RETURNED_WORD sample{s} first{first[s]} pass{p} row{r} got00000000 expected00000000')
   normal.append(f'PASS_NATIVE_LEARNED_BAND sample{s} tensor{tensor} first{first[s]} width5120 phases4 dual_jobs2 exact_words64 polls1 generation43;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
  normal.append('PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
  cases=[('pass',normal),('sourcefail',normal),('sourcedirty',normal),('sourcemissing',normal),('swapfail',normal),('busy',normal),
         ('runfail',normal),('dirty',normal),('falsepass',normal),('missing-auth',normal[1:]),('bad-ref',normal)]
  for n,pos in enumerate(positions):
   cases.append((f'missing-{n}',normal[:pos]+normal[pos+1:]))
   cases.append((f'duplicate-{n}',normal[:pos]+[normal[pos]]+normal[pos:]))
   altered=list(normal);altered[pos]=re.sub(r'got[0-9a-f]{8}', 'got00000001', altered[pos]);cases.append((f'wrong-{n}',altered))
  for mode,output in cases:
   stage=folder/(str(tensor)+'-'+mode);stage.mkdir();(stage/'reference').write_text('' if mode=='bad-ref' else '\n'.join(reference)+'\n')
   (stage/'output').write_text('\n'.join(output)+'\n')
   result=subprocess.run(['sh',str(program),str(stage),str(preflight),str(client),mode,str(tensor)],text=True,capture_output=True)
   events=(stage/'events').read_text().splitlines();restore='exchange:2026111300010149:2026102100010012'
   if mode=='pass':
    assert result.returncode==0 and events.count('preflight')==events.count('run')==events.count(restore)==1,(result.stderr,events)
    assert (stage/'trial.state').read_text().strip()=='STABLE_RESTORED_EXACT192'
   else:
    assert result.returncode!=0 and restore not in events,(mode,events,result.stderr)
    if mode in ('sourcefail','sourcedirty','sourcemissing'):assert not any(e.startswith('exchange:') for e in events)
    if 'run' in events:assert events[-1]=='run',(mode,events)
print('PASS alpha/beta actual one-shot flow and exhaustive word loss/duplication/mismatch, source and owner failures; NO DEVICE')
