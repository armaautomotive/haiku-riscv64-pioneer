#!/usr/bin/env python3
"""Compile/inspect actual QPX1 native held scope,never link/install/invoke it."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
if len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3]!='--final-release') or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
final=len(sys.argv)==4
qualified,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
 sys.argv=[str(here/'run_pioneer_q38_resident_layer_native_context_compile.py'),str(qualified),str(out/'old-controls')]
 base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
role=(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(parents)==21,parents
# The authenticated old trace intentionally keeps its OLD Work profile for
# unchanged controls. This new perimeter gets ONLY its explicit parents,
# never a retagged old Work image/dispatcher or CPU reference selection.
args=[a for a in base['args'] if a!='-DTT_PIONEER_Q38_WORK_MODE'
 and not a.startswith(('-DTT_PIONEER_Q38_WORK_MODE=','-DPIONEER_Q38_'))]
for name in sorted(parents):
 if not any(a=='-D'+name or a.startswith('-D'+name+'=') for a in args):args+=['-D'+name+'=1']
obj=out/'recurrent-prefix-context-NODRV.o'
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x5152504e4f445256ULL'
args[args.index('-c')+1]=str(here/'pioneer_q38_recurrent_prefix_native_context_compile.cpp');args[args.index('-o')+1]=str(obj)
if final:args+=['-DPIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE=1']
subprocess.run(args,check=True,env=base['clean'])
frames=base['frames'](obj.with_suffix('.su'));assert frames
for name,size in frames.items():assert size<=2048 or name in base['previous'] and size<=base['previous'][name],(name,size)
required=('PioneerQ38RecurrentPrefixMemory::PrepareHeld(', 'PioneerQ38RecurrentPrefixMemory::CloseHeldNoLaunch(',
 'RecurrentPrefixBaseScope(', 'RecurrentPrefixMemoryAccessScope(', 'RecurrentPrefixMemoryScope(', 'MapRecurrentPrefixMemory(',
 'RecurrentPrefixMemoryReadChecked(', 'RecurrentPrefixMemoryWrite(', 'pioneer_recurrent_prefix_context_scoped_held(',
 'pioneer_recurrent_prefix_context_scoped_dispatch_end(', 'pioneer_recurrent_prefix_context_destroy_clean(',
 'RecurrentPrefixInitializationScope(', 'RecurrentPrefixInitializationOtherCoresHeld(',
 'InitializeQ38RecurrentPrefixState(', 'InitializeQ38RecurrentPrefixHistory(', 'pioneer_recurrent_prefix_context_scoped_initialized_held(',
 'PioneerQ38RecurrentPrefixOwner::Prepare(', 'PioneerQ38RecurrentPrefixOwner::Run(', 'PioneerQ38RecurrentPrefixOwner::Close(',
 'RecurrentPrefixExecutionScope(', 'pioneer_recurrent_prefix_context_scoped_execution(')
if final:required+=('RecurrentPrefixFinalReleaseScope(', 'RecurrentPrefixFinalUnmap(', 'pioneer_recurrent_prefix_context_locked_end(')
for name in required:assert any(name in key for key in frames),(name,'actual complete implementation must be instantiated')
sizes=[v for k,v in frames.items() if 'RecurrentPrefix' in k or 'pioneer_recurrent_prefix_context_' in k];assert sizes and max(sizes)<=2048
tool=str(pathlib.Path(args[0]).parent/'riscv64-unknown-haiku-')
table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
symbols={r[-1]:(int(r[0],16),int(r[1],16),r[2]) for r in map(str.split,table.splitlines()) if len(r)==4}
assert not normal&symbols.keys() and all('pioneer_q38_recurrent_prefix_context_body_'+n in symbols for n in normal)
for forbidden in ('Q38WorkControl','OriginalSource','CompareFinal','ReferenceProducts','Q38MixedLiveValidationControl','PioneerQ38MixedLiveNativeState'):
 assert forbidden not in table,forbidden
# Compare exact code bytes in each symbol's OWN section,including COMDAT.
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={r[-1]:r[-3] for r in map(str.split,sections.splitlines()) if len(r)>=6 and r[-1] in symbols}
def code(name):
 start,size,_=symbols[name];assert size
 dis=subprocess.check_output([tool+'objdump','-d','-j',section[name],str(obj)],text=True)
 rows=[(int(m[1],16),m[2]) for line in dis.splitlines() for m in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+',line)] if m]
 selected=sorted((a,w) for a,w in rows if start<=a<start+size);assert selected
 at=start;words=[]
 for a,w in selected:assert a==at;at+=len(w)//2;words.append(w)
 assert at==start+size;return words
for name,control in (('init_hardware','refusal'),('init_driver','refusal'),('uninit_driver','inert'),('publish_devices','null'),('find_device','null')):
 assert code('pioneer_q38_recurrent_prefix_context_only_'+name)==code('pioneer_recurrent_prefix_context_expected_'+control)
# Not a witness whose only private constructors/scopes fold to false.
assert symbols['pioneer_recurrent_prefix_context_scoped_held'][1]>32
print('PASS ACTUAL QPX1 native FD/team/thread/source/model/clock/hosts/ALL120 held journal/4new L1 aliases/retention+dispatch-END SDK compile/maxNewFrame'+str(max(sizes)))
print('PASS NO conventional driver ABI/ALL5 independent inert controls BYTE-IDENTICAL/genuine private setup bodies retained;NO public compute/CPU reference/Work grant')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' recurrent-prefix-context-NODRV.o')
print('PASS SAME actual native completed-embedding context/private recurrent+history zero leases/real controller init+CLOSE SDK bodies;NO old native-role aliases or caller-ready setters')
print('PASS DISTINCT actual native QPX1 execution owner/metadata stage/private RUN+counter proof/STOP+ALL120holds/full journal restore body compile;NO CPU products or token/state commit')
if final:print('PASS DISTINCT prefix final-release/native closed+clock+FD/team/thread scope/30VM-only aliases/actual locked END call seal;COMPILE ONLY,NOT runtime or physical qualification')
print('COMPILE/INSPECT ONLY;private bodies WOULD do IO if invoked. Never linked/installed/invoked;runtime RUN/STOP/HOLD/restoration/full final disposal/physical and model-speed qualification STILL OWED')
