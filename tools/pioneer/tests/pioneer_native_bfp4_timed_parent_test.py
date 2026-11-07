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
source=(here.parent/'pioneer_native_bfp4_timed_physical_119.sh').read_text()
flow=source.split('# BFP4_FLOW_BEGIN\n',1)[1].split('# BFP4_FLOW_END',1)[0]
ref=(here.parent/'pioneer_native_bfp4_band_reference_5120.txt').read_bytes()
assert hashlib.sha256(ref).hexdigest()=='3448d29dc93451b95262466950541b33f013dd41f30db9275a09dbffcdd296c9'
golden=(here.parent/'PioneerNativeBfp4BandGolden.h').read_text()
match=re.search(r'static const uint32_t k5120\[32\]=\{([^}]+)\}',golden,re.S)
assert match
values=re.findall(r'0x([0-9a-f]{8})',match.group(1))
assert len(values)==32 and ref.decode().splitlines()==[f'row{i} {value}' for i,value in enumerate(values)]
words=[f'NATIVE_BFP_BAND_RETURNED_WORD k5120 pass{p} row{r} got{value} expected{value}' for p in range(2) for r,value in enumerate(values)]
summary='PASS_NATIVE_BFP_FULL_K bits4 width5120 rows32 dual_jobs2 phases2 exact_words64 polls1 generation15;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM'
timings=['NATIVE_BFP4_BAND_TIMING width5120 pass0 start1000 end2000 cycles1000;CARD_CYCLES_ONLY_L1_INPUTS',
         'NATIVE_BFP4_BAND_TIMING width5120 pass1 start3000 end4000 cycles1000;CARD_CYCLES_ONLY_L1_INPUTS']
normal=words+timings+[summary]
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
           ('swapfail',normal,ref),('busy',normal,ref),('bad-generation',words+timings+[summary.replace('generation15','generation16')],ref),
           ('zero-polls',words+timings+[summary.replace('polls1','polls0')],ref),('large-polls',words+timings+[summary.replace('polls1','polls20001')],ref),
           ('old-format',words+timings+[summary.replace('bits4','bits8')],ref),('early-summary',[summary]+words+timings,ref),
           ('extra-summary',normal+[summary],ref),('garbage',normal+['UNRECOGNIZED'],ref),('missing-reference',normal,b''),
           ('duplicate-reference',normal,ref+ref.splitlines(keepends=True)[0])]
    for index in range(2):
        cases.append((f'missing-timing-{index}',words+[timings[1-index],summary],ref))
        cases.append((f'duplicate-timing-{index}',words+timings+[timings[index],summary],ref))
    cases += [
        ('reversed-timing',words+timings[::-1]+[summary],ref),
        ('bad-duration',words+[timings[0].replace('cycles1000','cycles999'),timings[1],summary],ref),
        ('zero-time',words+[timings[0].replace('start1000','start0'),timings[1],summary],ref),
        ('backward-time',words+[timings[0].replace('end2000','end900'),timings[1],summary],ref),
        ('overlap',words+[timings[0],timings[1].replace('start3000','start1500').replace('cycles1000','cycles2500'),summary],ref),
        ('unknown-scope',words+[timings[0].replace('L1_INPUTS','GDDR'),timings[1],summary],ref),
        ('unsafe-integer',words+[timings[0].replace('start1000','start1234567890123456'),timings[1],summary],ref),
    ]
    for index in range(64):
        cases.append((f'missing-{index}',normal[:index]+normal[index+1:],ref))
        cases.append((f'duplicate-{index}',words+[words[index]]+timings+[summary],ref))
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
            assert events.count('run4')==1 and 'exchange:2026110600010119:2026102100010012' in events
            assert (stage/'state').read_text().strip()=='STABLE041_RESTORED_TIMED_BFP4_EXACT64_PASSED'
        else:
            assert result.returncode!=0,(mode,result.stdout)
            assert events.count('run4')==int(mode not in ('swapfail','busy'))
            assert 'exchange:2026110600010119:2026102100010012' not in events
            if 'run4' in events:
                assert events[-1]=='run4',(mode,events)
    prior_block=source.split('# BFP4_TIMED_PRIOR_GATE_BEGIN\n',1)[1].split('# BFP4_TIMED_PRIOR_GATE_END',1)[0]
    prior_block=prior_block.replace('prior=/boot/home/pioneer-native-bfp4-band-118-20261106-a','prior=$1')
    prior_script=root/'prior.sh'
    prior_script.write_text('''set -eu
stat(){ if [ "$2" = '%u:%a' ];then echo 0:700;else echo 0:1;fi; }
sha256sum(){ shasum -a 256 "$@"; }
'''+prior_block)
    saved=here.parent/'benchmarks/qwen38-bfp4-band-physical-20261106'
    for mode in ('pass','bad-state','bad-exit','dirty-stderr','dirty-output','missing-output','link-output','linked-directory'):
        location=root/('prior-'+mode)
        if mode=='linked-directory':
            actual=root/'prior-actual';actual.mkdir();location.symlink_to(actual,target_is_directory=True);directory=actual
        else:
            location.mkdir();directory=location
        for name in ('state','band5120.stdout','band5120.stderr','band5120.exit'):
            if mode=='missing-output' and name=='band5120.stdout':
                continue
            data=(saved/name).read_bytes()
            if mode=='bad-state' and name=='state':data=b'BAD\n'
            if mode=='bad-exit' and name=='band5120.exit':data=b'1\n'
            if mode=='dirty-stderr' and name=='band5120.stderr':data=b'error\n'
            if mode=='dirty-output' and name=='band5120.stdout':data+=b'x'
            if mode=='link-output' and name=='band5120.stdout':
                target=root/'linked-output';target.write_bytes(data);(directory/name).symlink_to(target)
            else:
                (directory/name).write_bytes(data)
        check=subprocess.run(['sh',str(prior_script),str(location)],capture_output=True,text=True)
        assert (check.returncode==0)==(mode=='pass'),(mode,check.stdout,check.stderr)
    print(f'PASS timed BFP4 actual parent flow: {len(cases)} fake cases plus 8 prior-result checks; incorrect or unproven data never triggers restore or another device query; NO hardware access')
