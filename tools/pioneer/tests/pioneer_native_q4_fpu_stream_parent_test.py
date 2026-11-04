#!/usr/bin/env python3
"""Actual86 parent flow/output/exchange/idle/one-shot fixture tests;NO device."""
import hashlib
import pathlib
import re
import subprocess
import tempfile
assert __debug__
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_native_q4_fpu_stream_physical.sh').read_text()
raw = (here.parent / 'benchmarks/qwen38-native-q4-fpu-stream-reference-20261104.log').read_bytes()
assert hashlib.sha256(raw).hexdigest() == '56ec253884deb7ba3f8a72dc09202f0b2a687752ebb15274843cbf14483008e9'
expected = {}
for line in raw.decode().splitlines():
    match=re.fullmatch(r'Q4_STREAM_FRACTION_REFERENCE sample([0-8]) tensor(\d+) first(\d+) width(\d+) row(\d+) word([0-9a-f]{8})',line);assert match
    sample,tensor,first,width,row=map(int,match.group(1,2,3,4,5))
    assert tensor==(7 if sample<3 else 8 if sample<6 else 6) and width==(5120 if sample<6 else 17408)
    assert first==((0,8704,17376) if sample<6 else (0,2560,5088))[sample%3] and row<32 and (sample,row) not in expected
    expected[sample,row]=match[6]
assert len(expected)==288
normal=['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_Q4_STREAM_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
        'AUTHENTICATED_FIXED_Q4_STREAM_MODEL c600de03 samples9 widths5120_17408 rows32 full_k_payload_and_reference_exact;DEBUG_NOT_WORK']
positions=[]
for sample in range(9):
    tensor=7 if sample<3 else 8 if sample<6 else 6;width=5120 if sample<6 else 17408;blocks=width//256
    first=((0,8704,17376) if sample<6 else (0,2560,5088))[sample%3]
    for job in range(2):
        for row in range(32):
            positions.append(len(normal));word=expected[sample,row]
            normal.append(f'NATIVE_Q4_FPU_STREAM_RETURNED_WORD sample{sample} tensor{tensor} first{first} width{width} pass{job} row{row} got{word} expected{word}')
    normal.append(f'PASS_NATIVE_Q4_FPU_STREAM_SAMPLE sample{sample} tensor{tensor} first{first} width{width} phases4 dual_jobs2 exact_words64 polls1 generation9 saved_l126908 saved_gddr{blocks*1664} requests{2*blocks*56} flits{2*blocks*104};EXPLICIT_END_L1_GDDR_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
normal.append('PASS_NATIVE_Q4_FPU_STREAM samples9 exact_words576 clean_ends9;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
assert len(normal)==588
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-q4-parent-test.'))
flow=source.split('# NATIVE_Q4_STREAM_FLOW_BEGIN\n',1)[1].split('# NATIVE_Q4_STREAM_FLOW_END',1)[0]
client=folder/'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-q4-fpu-full-k-stream ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then echo UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
''');client.chmod(0o700)
program=folder/'flow.sh'
program.write_text('''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;reference=$stage/reference
export stage events mode
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
'''+flow)
cases=[('pass',normal),('runfail',[]),('dirty',normal),('false-pass',normal),('swapfail',normal),('busy',normal),
       ('missing-auth',normal[1:]),('missing-verified',normal[:1]+normal[2:]),('duplicate-auth',normal[:1]+normal),
       ('duplicate-verified',normal[:2]+normal[1:]),('early-final',[normal[-1]]+normal[:-1]),('extra-final',normal+[normal[-1]]),
       ('garbage',normal+['NOISE']),('short-reference',normal)]
for number,position in enumerate(positions):
    cases.append((f'missing-{number}',normal[:position]+normal[position+1:]))
    cases.append((f'duplicate-{number}',normal[:position]+[normal[position]]+normal[position:]))
    bad=list(normal);bad[position]=re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}','got00000000 expected00000000',bad[position]);cases.append((f'both-wrong-{number}',bad))
for sample in range(9):
    index=66+sample*65;width=5120 if sample<6 else 17408;blocks=width//256
    for name,old,new in [('generation','generation9','generation10'),('poll-zero','polls1','polls0'),('poll-large','polls1','polls20001'),
                         ('fidelity','phases4','phases1'),('geometry',f'width{width}','width256'),('journal','saved_l126908','saved_l112372'),
                         ('gddr',f'saved_gddr{blocks*1664}','saved_gddr0'),('requests',f'requests{2*blocks*56}','requests320'),
                         ('flits',f'flits{2*blocks*104}','flits2880'),('clock','CLOCK_PCI_RESTORED','PCI_RESTORED'),
                         ('fifo','FIFO_TLB_CLOCK_PCI_RESTORED','TLB_CLOCK_PCI_RESTORED')]:
        bad=list(normal);assert old in bad[index];bad[index]=bad[index].replace(old,new);cases.append((f'{name}-{sample}',bad))
for name,a,b in [('rows',2,3),('jobs',2,34),('samples',2,67)]:
    bad=list(normal);bad[a],bad[b]=bad[b],bad[a];cases.append((name,bad))
old=here.parent/'benchmarks/qwen38-native-dram-band64-observed-20261104/gddr64.stdout'
assert old.is_file()
cases.append(('old84',old.read_text().splitlines()))
cases.append(('old85',(here.parent/'benchmarks/qwen38-native-q4-fpu-physical-20261104/q4.stdout').read_text().splitlines()))
for mode,output in cases:
    stage=folder/mode;stage.mkdir();(stage/'reference').write_bytes(raw if mode!='short-reference' else b'');(stage/'output').write_text('\n'.join(output)+'\n')
    result=subprocess.run(['sh',str(program),str(stage),str(client),mode],capture_output=True)
    events=(stage/'events').read_text().splitlines()
    if mode=='pass':
        assert result.returncode==0 and events.count('run')==1 and events.count('exchange:2026110400010086:2026102100010012')==1,(result.stdout,result.stderr)
        assert (stage/'state').read_text().strip()=='STABLE041_RESTORED_Q4_STREAM_EXACT576_PASSED'
    else:
        assert result.returncode and 'exchange:2026110400010086:2026102100010012' not in events,(mode,events,result.stdout,result.stderr)
        if 'run' in events:assert events[-1]=='run',(mode,events)
idle=source.split('idle(){\n',1)[1].split('\n}\n',1)[0]
program=folder/'idle.sh';program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n'+idle+'\n}\nidle\n')
base='Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-q4-fpu-stream-20261104-a/parent\n'
inventories=[(base,True),(base+'99 /boot/system/Tracker\n',True),(base+'99 /bin/bash -c stage=/boot/home/pioneer-native-q4-fpu-stream-20261104-a\n',False),
             (base+'99 /boot/home/pioneer-native-q4-fpu-stream-diagnostic\n',False),(base+'99 /boot/home/HaikuAssistantService\n',False),('Id Team\n',False),(base+'malformed\n',False)]
for index,(text,accepted) in enumerate(inventories):
    path=folder/f'inventory-{index}';path.write_text(text);result=subprocess.run(['sh',str(program),str(path)],capture_output=True);assert (result.returncode==0)==accepted
gate=source.split('# NATIVE_Q4_STREAM_PRIOR_GATE_BEGIN\n',1)[1].split('# NATIVE_Q4_STREAM_PRIOR_GATE_END',1)[0]
gate=gate.replace('original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf','original=$stage/original')
program=folder/'prior.sh';program.write_text('''set -eu
stage=$1;mode=$2
stat(){
 if [ "$mode" = bad-owner ];then echo 1:1
 elif [ "$2" = '%u:%h:%a:%s' ];then
  if [ "$mode" = bad-model-permission ];then echo 0:1:644:18973870528;else echo 0:1:600:18973870528;fi
 else echo 0:1;fi
}
sha256sum(){ shasum -a 256 "$@"; }
'''+gate)
gate_cases=('pass','bad-owner','dirty-reference','missing-reference','linked-reference','linked-model','missing-model','bad-model-permission')
for mode in gate_cases:
    stage=folder/('prior-'+mode);stage.mkdir()
    if mode=='linked-reference':
        (stage/'target').write_bytes(raw);(stage/'q4-stream-reference.log').symlink_to(stage/'target')
    elif mode!='missing-reference':(stage/'q4-stream-reference.log').write_bytes(raw if mode!='dirty-reference' else raw+b'X')
    if mode=='linked-model':
        (stage/'target-model').write_bytes(b'NOT_A_MODEL');(stage/'original').symlink_to(stage/'target-model')
    elif mode!='missing-model':(stage/'original').write_bytes(b'NOT_A_MODEL')
    result=subprocess.run(['sh',str(program),str(stage),mode],capture_output=True);assert (result.returncode==0)==(mode=='pass'),(mode,result.stdout,result.stderr)
exchange=source.split('exchange(){\n',1)[1].split('\n}\n',1)[0]
program=folder/'exchange.sh';program.write_text('''set -eu
stage=$1;failure=$2;installed=$stage/installed;replacement=$stage/replacement;client=$stage/client;client_hash=CLIENT;admin=Admin
step(){ n=$(cat "$stage/count");n=$((n+1));echo "$n" > "$stage/count";echo "$1" >> "$stage/events";[ "$n" != "$failure" ]; }
exact(){ step exact; };idle(){ step idle; };metadata(){ step metadata; };Admin(){ step "admin:$1"; }
cp(){ step cp && command cp "$@"; };chmod(){ step chmod && command chmod "$@"; }
mv(){ step mv && command mv "$@"; };sync(){ step sync; }
printf(){ step printf && command printf "$@"; }
exchange(){
'''+exchange+'\n}\nexchange OLD NEW oldtag newtag 6 7 "$replacement" || exit 1\n')
exchange_count=None
for failure in range(100):
    stage=folder/f'exchange-{failure}';stage.mkdir();(stage/'count').write_text('0\n');(stage/'installed').write_text('OLD');(stage/'replacement').write_text('NEW')
    result=subprocess.run(['sh',str(program),str(stage),str(failure)],capture_output=True);events=(stage/'events').read_text().splitlines()
    if not failure:
        assert result.returncode==0 and (stage/'installed').read_text()=='NEW' and events.count('admin:--quiesce')==1 and events.count('admin:--wait')==1;exchange_count=len(events)
    else:assert result.returncode and len(events)==failure,(failure,events,result.stdout,result.stderr)
    if failure==exchange_count:break
assert exchange_count and exchange_count<100
once='for name in .trial-once'+source.split('for name in .trial-once',1)[1].split('# NATIVE_Q4_STREAM_FLOW_BEGIN',1)[0]
program=folder/'once.sh';program.write_text('set -eu\nstage=$1;stable=$stage/stable;stable_hash=STABLE\nexact(){ :; }\n'+once)
once_names=('.trial-once','stable041.backup','driver.incoming','q4-stream.stdout','q4-stream.stderr','q4-stream.exit','parent.pid','state')
for name in ('pass',*once_names):
    stage=folder/('once-'+name);stage.mkdir();(stage/'stable').write_text('STABLE')
    if name!='pass':(stage/name).write_text('EXISTING')
    result=subprocess.run(['sh',str(program),str(stage)],capture_output=True);assert (result.returncode==0)==(name=='pass')
    if name=='pass':
        before=(stage/'state').read_bytes(),(stage/'parent.pid').read_bytes();again=subprocess.run(['sh',str(program),str(stage)],capture_output=True)
        assert again.returncode and before==((stage/'state').read_bytes(),(stage/'parent.pid').read_bytes())
    else:assert (stage/name).read_text()=='EXISTING'
metadata=source.split('metadata(){\n',1)[1].split('\n}\n',1)[0]
program=folder/'metadata.sh';program.write_text('set -eu\ninventory=$1;failure=$2;admin=Admin\nAdmin(){ [ "$failure" = 0 ] && cat "$inventory"; }\nmetadata(){\n'+metadata+'\n}\nmetadata 2026102100010012 8\n')
good_metadata='P100A_RELOAD driver55 tag2026102100010012 generation8 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n'
metadata_cases=[(good_metadata,0,True),(good_metadata,1,False),('',0,False),(good_metadata+'EXTRA\n',0,False),(good_metadata.replace('\n','\r\n'),0,False)]
for old,new in [('driver55','driver54'),('2026102100010012','2026110400010086'),('generation8','generation9'),('quiesced0','quiesced1'),('cold0','cold1'),('active_owner0','active_owner1'),('open_count1','open_count0'),('open_count1','open_count2'),('guard1','guard0')]:
    metadata_cases.append((good_metadata.replace(old,new),0,False))
for i,(text,failure,accepted) in enumerate(metadata_cases):
    p=folder/f'metadata-{i}';p.write_text(text);r=subprocess.run(['sh',str(program),str(p),str(failure)],capture_output=True);assert (r.returncode==0)==accepted
health=source.split('health(){\n',1)[1].split('\n}\n',1)[0]
program=folder/'health.sh';program.write_text('set -eu\ninventory=$1;failure=$2\nsysinfo(){ [ "$failure" = 0 ] && cat "$inventory"; }\nhealth(){\n'+health+'\n}\nhealth\n')
good_health='64 CPUs running at 100 MHz\nMemory (used/max 10 / 137034936320)\n'
health_cases=[(good_health,0,True),(good_health,1,False),('',0,False),(good_health*2,0,False),(good_health.replace('64 CPUs','1 CPU'),0,False),
              (good_health.replace('137034936320','3185577984'),0,False),(good_health.splitlines()[0]+'\n',0,False),(good_health.splitlines()[1]+'\n',0,False)]
for i,(text,failure,accepted) in enumerate(health_cases):
    p=folder/f'health-{i}';p.write_text(text);r=subprocess.run(['sh',str(program),str(p),str(failure)],capture_output=True);assert (r.returncode==0)==accepted
print(f'PASS actual86 parent:{len(cases)} flow/output+{len(inventories)} idle+{len(gate_cases)} reference/source+{exchange_count+1} exchange-step+{len(once_names)+2} one-shot+{len(metadata_cases)} metadata+{len(health_cases)} CPU/RAM cases;all576 missing/duplicate/equal-but-wrong words refuse;NO next query/restore/retry after uncertainty;NOT hardware proof')
print('CPU-only actual86 parent evidence:',folder)
