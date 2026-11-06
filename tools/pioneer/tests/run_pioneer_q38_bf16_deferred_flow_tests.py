#!/usr/bin/env python3
"""Actual Run/Prepare source routing over explicit MOCK callback outcomes."""
import hashlib,json,os,pathlib,subprocess,sys
here=pathlib.Path(__file__).resolve().parent
assert __debug__ and len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out=pathlib.Path(sys.argv[1]);assert out.is_absolute() and not out.exists();out.mkdir(mode=0o700)
root=here.parents[2]
paths=[pathlib.Path(__file__).resolve(),here/'pioneer_q38_bf16_deferred_flow_test.cpp',
 here.parent/'PioneerQ38Bf16DeferredAudit.h',here.parent/'blackhole_q38_bf16_projection_root.inc',
 here.parent/'blackhole_qwen38_mixed_projection_root.cpp']
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
def extract(path,signature):
 text=path.read_text();assert text.count(signature)==1
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
prepare=extract(here.parent/'blackhole_q38_bf16_projection_root.inc','static BF_ROOT_FN void BfPrepare(')
run=extract(here.parent/'blackhole_qwen38_mixed_projection_root.cpp','static unsigned Run(')
# Only ABI adaptation: hosted pointers are64bit,firmware pointers32bit.
for name in ('desc','job'):
 old='reinterpret_cast<unsigned>('+name+')';assert prepare.count(old)==1
 prepare=prepare.replace(old,'reinterpret_cast<uintptr_t>('+name+')')
(out/'actual_bf_prepare.inc').write_text(prepare+'\n')
(out/'actual_projection_run.inc').write_text(run+'\n')
exits={};commands=[]
def call(name,args):
 args=list(map(str,args));commands.append([name,*args])
 with (out/(name+'.stdout')).open('xb') as a,(out/(name+'.stderr')).open('xb') as b:
  result=subprocess.run(args,stdout=a,stderr=b,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_DEFERRED_FLOW',name,'actual0_stderr0',flush=True)
flags=['-DPIONEER_Q38_GLOBAL_CHAIN_BF16_FPU=1','-DPIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT=1','-DPIONEER_Q38_BF16_PROFILE_IMAGE=1']
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined','-I'+str(out)]
source=here/'pioneer_q38_bf16_deferred_flow_test.cpp'
for name,extra in (('old',[]),('deferred',['-DPIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE=1'])):
 binary=out/name;call(name+'-compile',[*common,*flags,*extra,source,'-o',binary]);call(name,[binary])
for name,line in (
 ('missing-matrix','  if(BfQ4(d) && !audit.MatrixClosed(w))scalar_fault_park();'),
 ('missing-idle','  if(BfQ4(d) && !audit.IdleClosed(w))scalar_fault_park();')):
 assert run.count(line)==1
 mutant=run.replace(line,'  (void)w; // TEST MUTANT:omit required closure mark')
 (out/'actual_projection_run.inc').write_text(mutant+'\n')
 binary=out/name
 # Missing idle coverage cannot affect the119-consumer case. Limit that
 # mutation's cases explicitly instead of pretending it creates a gap there.
 if name=='missing-idle':
  text=source.read_text().replace('118U,119U','118U')
  fixture=out/'idle-mutant.cpp';fixture.write_text(text.replace('#include "../PioneerQ38Bf16DeferredAudit.h"','#include "'+str(here.parent/'PioneerQ38Bf16DeferredAudit.h')+'"'))
 else:fixture=source
 call(name+'-compile',[*common,*flags,'-DPIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE=1','-DPIONEER_TEST_MISSING_MARK=1',fixture,'-o',binary])
 call(name,[binary])
(out/'actual_projection_run.inc').write_text(run+'\n')
(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
(out/'extracted-sha256.json').write_text(json.dumps({'prepare_host_pointer_adapted':hashlib.sha256(prepare.encode()).hexdigest(),'run_actual':hashlib.sha256(run.encode()).hexdigest()},indent=2)+'\n')
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==h for p,h in frozen.items())
(out/'source-sha256.txt').write_text(''.join(h+'  '+str(p.relative_to(root))+'\n' for p,h in sorted(frozen.items())))
print('PASS_DEFERRED_CONTROLLER_FLOW_ACTUAL_SOURCE_MOCK_CALLBACKS_NOT_NATIVE_HARDWARE_OR_TPS',flush=True)
