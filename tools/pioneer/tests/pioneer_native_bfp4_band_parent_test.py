#!/usr/bin/env python3
"""Run the exact BFP4 parent flow with fake endpoints; NEVER access a device."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile

assert __debug__
here=Path(__file__).resolve().parent
source=(here.parent/'pioneer_native_bfp4_band_physical_118.sh').read_text()
flow=source.split('# BFP4_FLOW_BEGIN\n',1)[1].split('# BFP4_FLOW_END',1)[0]
ref=(here.parent/'pioneer_native_bfp4_band_reference_5120.txt').read_bytes()
assert hashlib.sha256(ref).hexdigest()=='3448d29dc93451b95262466950541b33f013dd41f30db9275a09dbffcdd296c9'
golden=(here.parent/'PioneerNativeBfp4BandGolden.h').read_text()
match=re.search(r'static const uint32_t k5120\[32\]=\{([^}]+)\}',golden,re.S)
assert match
values=re.findall(r'0x([0-9a-f]{8})',match.group(1))
assert len(values)==32 and ref.decode().splitlines()==[f'row{i} {value}' for i,value in enumerate(values)]
words=[f'NATIVE_BFP_BAND_RETURNED_WORD k5120 pass{p} row{r} got{value} expected{value}' for p in range(2) for r,value in enumerate(values)]
summary='PASS_NATIVE_BFP_FULL_K bits4 width5120 rows32 dual_jobs2 phases2 exact_words64 polls1 generation13;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM'
normal=words+[summary]
prefix='''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;reference=$stage/reference.txt
export stage events mode
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
'''
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-parent-',dir='/private/tmp') as temp:
    root=Path(temp)
    client=root/'client'
    client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-k5120 ] || exit 2
echo run4 >> "$events"
if [ "$mode" = runfail ];then echo RUN_UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED_STDERR >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
''')
    client.chmod(0o700)
    program=root/'flow.sh'
    program.write_text(prefix+flow)
    cases=[('pass',normal,ref),('runfail',[],ref),('dirty',normal,ref),('false-pass',normal,ref),
           ('swapfail',normal,ref),('busy',normal,ref),('bad-generation',words+[summary.replace('generation13','generation14')],ref),
           ('zero-polls',words+[summary.replace('polls1','polls0')],ref),('large-polls',words+[summary.replace('polls1','polls20001')],ref),
           ('old-format',words+[summary.replace('bits4','bits8')],ref),('early-summary',[summary]+words,ref),
           ('extra-summary',normal+[summary],ref),('garbage',normal+['UNRECOGNIZED'],ref),('missing-reference',normal,b''),
           ('duplicate-reference',normal,ref+ref.splitlines(keepends=True)[0])]
    for index in range(64):
        cases.append((f'missing-{index}',normal[:index]+normal[index+1:],ref))
        cases.append((f'duplicate-{index}',words+[words[index],summary],ref))
        wrong=list(normal)
        wrong[index]=re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}','got00000000 expected00000000',wrong[index])
        cases.append((f'both-wrong-{index}',wrong,ref))
    for mode,output,reference in cases:
        stage=root/mode
        stage.mkdir()
        (stage/'reference.txt').write_bytes(reference)
        (stage/'output').write_text('\n'.join(output)+'\n')
        result=subprocess.run(['sh',str(program),str(stage),str(client),mode],text=True,capture_output=True)
        events=(stage/'events').read_text().splitlines()
        if mode=='pass':
            assert result.returncode==0,(mode,result.stdout,result.stderr)
            assert events.count('run4')==1 and 'exchange:2026110600010118:2026102100010012' in events
            assert (stage/'state').read_text().strip()=='STABLE041_RESTORED_FULL_K5120_BFP4_EXACT64_PASSED'
        else:
            assert result.returncode!=0,(mode,result.stdout)
            assert events.count('run4')==int(mode not in ('swapfail','busy'))
            assert 'exchange:2026110600010118:2026102100010012' not in events
            if 'run4' in events:
                assert events[-1]=='run4',(mode,events)
    print(f'PASS BFP4 actual parent flow: {len(cases)} fake cases; incorrect or unproven data never triggers restore or another device query; NO hardware access')
