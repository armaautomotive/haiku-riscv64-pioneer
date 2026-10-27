#!/usr/bin/env python3
"""Native QBL1 source/perimeter/L1/owner compile;NO runtime/launch grant."""
import hashlib
import pathlib
import re
import runpy
import subprocess
import sys
assert len(sys.argv)==3 or len(sys.argv)==4 and sys.argv[3] in ('--sigmoid-candidate','--validation-readback')
sigmoid=len(sys.argv)==4
readback=sigmoid and sys.argv[3]=='--validation-readback'
qualified,out=map(pathlib.Path,sys.argv[1:3])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
 sys.argv=[str(here/'run_pioneer_q38_resident_layer_native_context_compile.py'),str(qualified),str(out/'old-controls')]
 base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
 sys.argv=saved
args=base['args'][:]
if sigmoid:args+=['-DPIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS=1']
if readback:args+=['-DPIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK=1']
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x51424c4e4f445256ULL'
obj=out/'mixed-live-context-NODRV.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_mixed_live_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True,env=base['clean'])
frames=base['frames'](obj.with_suffix('.su'));assert frames
for name,size in frames.items():
 assert size<=2048 or name in base['previous'] and size<=base['previous'][name],(name,size)
for required in ('PrepareHeld(', 'CloseHeldNoLaunch(', 'pioneer_live_context_prepare_held(', 'MixedLiveBaseScope(',
 'MixedLiveMemoryReadChecked(', 'MixedLiveMemoryWrite(', 'pioneer_live_context_scoped_held(', 'pioneer_live_context_scoped_execute(',
 'PioneerQ38MixedLiveOwner::Prepare(', 'PioneerQ38MixedLiveOwner::Run(', 'PioneerQ38MixedLiveOwner::Close(',
 'pioneer_live_context_end(', 'pioneer_live_context_destroy_clean('):
 assert any(required in name for name in frames),(required,'genuine native implementation must be instantiated')
if readback:
 for required in ('pioneer_live_context_scoped_validation(', 'PioneerQ38MixedLiveOwner::CloseForValidation(',
  'PioneerQ38MixedLiveOwner::CloseImpl(', 'PioneerQ38MixedLiveOwner::ValidationReadback('):
  assert any(required in name for name in frames),(required,'real guarded readback must be instantiated')
nm=str(pathlib.Path(args[0]).parent/'riscv64-unknown-haiku-nm')
table=subprocess.check_output([nm,'--defined-only',str(obj)],text=True)
names={r[-1] for r in map(str.split,table.splitlines()) if r}
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
assert not names&normal
assert all('pioneer_q38_live_context_body_'+n in names for n in normal)
assert all('pioneer_q38_live_context_only_'+n in names for n in normal-{'api_version'})
for forbidden in ('Q38ResidentLayerValidationControl','RunQ38ResidentLayerNative','RunQ38ProjectionExecution','RunQ38LearnedRmsModel',
 'DecoderOriginalSource','CompareResidentLayer','PioneerQ38ResidentLayerOwner'):
 assert forbidden not in table,forbidden
tool=str(pathlib.Path(args[0]).parent/'riscv64-unknown-haiku-objdump')
assembly=subprocess.check_output([tool,'-d','-j','.text',str(obj)],text=True)
symbol_table=subprocess.check_output([nm,'-S','--defined-only',str(obj)],text=True)
symbols={r[3]:(int(r[0],16),int(r[1],16)) for r in map(str.split,symbol_table.splitlines()) if len(r)==4 and r[2] in ('T','t')}
instructions={int(m[1],16):m[2] for line in assembly.splitlines() for m in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+',line)] if m}
def code(name):
 start,size=symbols[name];rows=sorted((a,w) for a,w in instructions.items() if start<=a<start+size);at=start
 assert rows
 for a,w in rows:
  assert a==at and len(w) in (4,8);at+=len(w)//2
 assert at==start+size
 return [w for a,w in rows]
for n in ('init_hardware','init_driver'):
 assert code('pioneer_q38_live_context_only_'+n)==code('pioneer_live_context_expected_refusal')
for n in ('publish_devices','find_device'):
 assert code('pioneer_q38_live_context_only_'+n)==code('pioneer_live_context_expected_null')
assert code('pioneer_q38_live_context_only_uninit_driver')==code('pioneer_live_context_expected_inert')
for scope in ('_ZN16PioneerSessionIO25MixedLiveCallScopeMatchesEv','_ZN16PioneerSessionIO18MixedLiveBaseScopeEv'):
 assert len(code(scope))>16 and code(scope)!=code('pioneer_live_context_expected_null'),(scope,'scope must contain genuine runtime checks,not optimized constant false')
sizes=[v for k,v in frames.items() if any(n in k for n in ('MixedLive','mixed_live','pioneer_live_context_'))]
print('PASS ACTUAL native context/FD+team+callthread/source+embedding/retained model/clock+hosts/new L1 aliases+held journal/private RUN+STOP+ALL120holds+restore/retention+END compile;maxNewFrame'+str(max(sizes)))
print('PASS NO conventional ABI exports/ALL5 independent inert controls byte-identical/private ACTUAL setup+closed-storage bodies retained/scopes NOT constant false;NO public mixed execution entry')
if readback:print('PASS separate guarded validation fixed5page readback retained;HEAP staging/atomic capture+restore;NO loadable driver/client/physical qualification')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('COMPILE ONLY;private actual setup bodies WOULD perform IO if invoked;tool only compiles+inspects,never links/installs/invokes. Genuine mixed execution/STOP/restore/native numerical qualification and full-model integration STILL OWED;NO card/speed claim')
