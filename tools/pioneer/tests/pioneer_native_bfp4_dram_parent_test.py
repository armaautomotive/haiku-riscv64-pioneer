#!/usr/bin/env python3
"""Exercise actual diagnostic120 parent flow/exchange using fake endpoints only."""
import pathlib
import runpy
import subprocess
import tempfile
import sys

assert __debug__
here=pathlib.Path(__file__).resolve().parent
if len(sys.argv) not in (1, 2):
 raise SystemExit('use [GENERATED_MATCHED_PARENT]')
matched=len(sys.argv)==2
source=(pathlib.Path(sys.argv[1]) if matched else here.parent/'pioneer_native_bfp4_dram_physical_120.sh').read_text()
if matched:
 assert 'stage=/boot/home/pioneer-native-bfp4-matched121-20261107-a' in source
 assert '2026110700010121' in source and 'awk -v generation=19' in source
 assert '2026110600010120' not in source
newtag='2026110700010121' if matched else '2026110600010120'
finalstate=('STABLE041_RESTORED_MATCHED_BFP4_GDDR_EXACT192_PASSED' if matched else
            'STABLE041_RESTORED_BFP4_GDDR_BANDS_EXACT192_PASSED')
flow=source.split('# BFP4_FLOW_BEGIN\n',1)[1].split('# BFP4_FLOW_END',1)[0]
normal=runpy.run_path(str(here/'pioneer_native_bfp4_dram_output_test.py'))['lines']
if matched:normal=[line.replace('generation17','generation19') for line in normal]
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-bfp4-dram-parent.'))
client=folder/'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-bfp4-gddr-learned-bands ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then echo UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED >&2;fi
if [ "$mode" = falsepass ];then exit 1;fi
''')
client.chmod(0o700)
prefix='''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;gate=$4
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
       ('missing',normal[:-2]+normal[-1:]),('extra',normal+['unexpected']),
       ('bad-word',[line.replace('got00000000','got00000001',1) if i==2 else line for i,line in enumerate(normal)]),
       ('bad-timing',[line.replace('cycles12345;','cycles0;',1) if i==66 else line for i,line in enumerate(normal)])]
for mode,output in cases:
 stage=folder/mode;stage.mkdir();(stage/'output').write_text('\n'.join(output)+'\n')
 result=subprocess.run(['sh',str(program),str(stage),str(client),mode,str(here.parent/'pioneer_native_bfp4_dram_output.awk')],capture_output=True,text=True)
 events=(stage/'events').read_text().splitlines()
 if mode=='pass':
  assert result.returncode==0 and events.count('run')==1 and events.count('exchange:'+newtag+':2026102100010012')==1,(result.stdout,result.stderr,events)
  assert (stage/'state').read_text().strip()==finalstate
 else:
  assert result.returncode!=0 and 'exchange:'+newtag+':2026102100010012' not in events,(mode,events)
  if 'run' in events:assert events[-1]=='run',(mode,events)
exchange=source.split('exchange(){\n',1)[1].split('\n}\n',1)[0]
exchange_program=folder/'exchange.sh'
exchange_program.write_text('''set -eu
stage=$1;failure=$2;installed=$stage/installed;replacement=$stage/replacement;client=$stage/client;client_hash=CLIENT;admin=Admin
step(){ n=$(cat "$stage/count");n=$((n+1));echo "$n" > "$stage/count";echo "$1" >> "$stage/events";[ "$n" != "$failure" ]; }
exact(){ step exact; };idle(){ step idle; };metadata(){ step metadata; };Admin(){ step "admin:$1"; }
cp(){ step cp && command cp "$@"; };chmod(){ step chmod && command chmod "$@"; }
mv(){ step mv && command mv "$@"; };sync(){ step sync; }
printf(){ step printf && command printf "$@"; }
exchange(){
'''+exchange+'\n}\nexchange OLD NEW oldtag newtag 16 17 "$replacement" || exit 1\n')
count=None
for failure in range(100):
 stage=folder/('exchange-'+str(failure));stage.mkdir();(stage/'count').write_text('0\n')
 (stage/'installed').write_text('OLD');(stage/'replacement').write_text('NEW')
 result=subprocess.run(['sh',str(exchange_program),str(stage),str(failure)],capture_output=True,text=True)
 events=(stage/'events').read_text().splitlines()
 if not failure:
  assert result.returncode==0 and (stage/'installed').read_text()=='NEW'
  assert events.count('admin:--quiesce')==1 and events.count('admin:--wait')==1
  count=len(events)
 else:assert result.returncode!=0 and len(events)==failure,(failure,events,result.stderr)
 if failure==count:break
assert count and count<100
prior_gate=source.split('# BFP4_TIMED_PRIOR_GATE_BEGIN\n',1)[1].split('# BFP4_TIMED_PRIOR_GATE_END',1)[0]
prior_gate=prior_gate.replace('prior=/boot/home/pioneer-native-bfp4-timed-119-20261106-a','prior=$stage/prior')
prior_program=folder/'prior.sh'
prior_program.write_text('''set -eu
stage=$1;mode=$2
stat(){
 if [ "$mode" = badowner ];then echo 1:1
 elif [ "$2" = '%u:%a' ];then echo 0:700
 else echo 0:1;fi
}
sha256sum(){ shasum -a 256 "$@"; }
'''+prior_gate)
record=here.parent/'benchmarks/qwen38-bfp4-band-timing-physical-20261106'
prior_modes=('pass','state','exit','stderr','stdout','missing','linked','badowner')
for mode in prior_modes:
 stage=folder/('prior-'+mode);(stage/'prior').mkdir(parents=True)
 for name in ('state','band5120.stdout','band5120.stderr','band5120.exit'):
  if mode=='missing' and name=='state':continue
  dest=stage/'prior'/name
  if mode=='linked' and name=='band5120.stdout':dest.symlink_to(record/name)
  else:dest.write_bytes((record/name).read_bytes()+(b'X' if mode in ('state','exit','stderr','stdout') and name=={'state':'state','exit':'band5120.exit','stderr':'band5120.stderr','stdout':'band5120.stdout'}.get(mode) else b''))
 result=subprocess.run(['sh',str(prior_program),str(stage),mode],capture_output=True,text=True)
 assert (result.returncode==0)==(mode=='pass'),(mode,result.stderr)
once='for name in .trial-once'+source.split('for name in .trial-once',1)[1].split('# BFP4_FLOW_BEGIN',1)[0]
once_program=folder/'once.sh'
once_program.write_text('set -eu\nstage=$1;stable=$stage/stable;stable_hash=STABLE\nexact(){ :; }\n'+once)
once_names=('.trial-once','stable041.backup','driver.incoming','band5120.stdout','band5120.stderr','band5120.exit','parent.pid','state')
for name in ('pass',*once_names):
 stage=folder/('once-'+name);stage.mkdir();(stage/'stable').write_text('STABLE')
 if name!='pass':(stage/name).write_text('EXISTING')
 result=subprocess.run(['sh',str(once_program),str(stage)],capture_output=True,text=True)
 assert (result.returncode==0)==(name=='pass'),(name,result.stderr)
 if name=='pass':assert (stage/'.trial-once').is_dir() and (stage/'state').read_text().strip()=='ADMITTED_BEFORE_SWAP'
idle=source.split('idle(){\n',1)[1].split('\n}\n',1)[0]
idle_program=folder/'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n'+idle+'\n}\nidle\n')
base='Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-bfp4-dram120-20261106-a/parent\n'
inventories=((base,True),(base+'99 /boot/system/Tracker\n',True),
             (base+'99 /boot/home/pioneer-native-bfp4-dram-model-diagnostic\n',False),
             (base+'99 /boot/home/HaikuAssistantService\n',False),('Id Team\n',False),(base+'malformed\n',False))
for index,(inventory,accepted) in enumerate(inventories):
 path=folder/('inventory-'+str(index));path.write_text(inventory)
 result=subprocess.run(['sh',str(idle_program),str(path)],capture_output=True,text=True)
 assert (result.returncode==0)==accepted,(index,result.stderr)
print(f'PASS BFP4 parent: {len(cases)} flow cases, {count} exchange-failure boundaries, {len(prior_modes)} prior, {len(once_names)+1} one-shot and {len(inventories)} idle gates; no action after uncertainty; NO DEVICE')
