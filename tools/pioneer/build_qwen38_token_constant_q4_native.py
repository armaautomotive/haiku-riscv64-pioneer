#!/usr/bin/env python3
"""Complete constant-Q4 worker PRIVATE native subject; never install or run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA = '5e2c95a3887920990a3ef0b80cfd0057a68bc95b645f03476c7869d9dd622e63'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or PRIVATE_SHA)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_phase_profile_native.py'),str(out/'unchanged-phase-timing')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
hooks,tool=parent['hooks'],parent['tool']
assert hooks['sha'](parent['obj'])=='2076cee83e0fc5e06914d55c25528752488166399e36a4eedef08fd406278d41'
obj=out/'token-constant-q4-NODRV.o';command=parent['command'][:]
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_token_constant_q4_native_context_compile.cpp')
command[command.index('-o')+1]=str(obj)
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')

def payload(path,name,values,mapping,label):
    first,size=values[name];target=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',mapping[name],str(path),str(target)],check=True)
    data=target.read_bytes();assert first+size<=len(data)
    return data[first:first+size]

before,before_sections=hooks['records'](parent['obj']);symbols,sections=hooks['records'](obj)
header=(here/'PioneerQ38TokenConstantQ4Programs.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='79c3056dfbe9b7dd564dc93c3577d72c0bcb004d5018dd0d6b7da212dfad8061'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==14
changed=[]
for digest,label in arrays:
    names=[name for name in symbols if 'PioneerQ38TokenConstantQ4Programs' in name and name.endswith(str(len(label))+label+'E')]
    old_names=[name for name in before if 'PioneerQ38TokenPhaseProfilePrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==len(old_names)==1
    current=payload(obj,names[0],symbols,sections,'new-'+label)
    previous=payload(parent['obj'],old_names[0],before,before_sections,'old-'+label)
    assert hashlib.sha256(current).hexdigest()==digest
    if current!=previous:changed.append(label)
assert changed==['PeerFront','PeerFetch','PeerAffine','PeerText'],changed
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in before
assert payload(obj,rope[0],symbols,sections,'new-rope')==payload(parent['obj'],rope[0],before,before_sections,'old-rope')
assert not set(hooks['abi'])&symbols.keys()
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        symbol=prefix+name
        assert payload(obj,symbol,symbols,sections,'new-'+symbol)==payload(parent['obj'],symbol,before,before_sections,'old-'+symbol)
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
for name in ('PioneerQ38TokenConstantQ4Programs','ReadQ38TokenNativePhaseProfile(',
             'PioneerQ38TokenNativeOwner::ReadPhaseProfile<','Q38TokenPhaseProfileControl(','Q38TokenNativeControl(',
             'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
             'BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):assert name in table,name
for name in ('PioneerQ38TokenPhaseProfilePrograms','PioneerQ38TokenWideGdnPrograms','PioneerQ38TokenTwinBatchPrefetchPrograms',
             'PioneerQ38TokenBatchedPrefetchPrograms','PioneerQ38TokenPrefetchPrograms','PioneerQ38TokenFlatMatrixPrograms',
             'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
previous,current=hooks['frames'](parent['obj']),hooks['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:assert digest==PRIVATE_SHA
print('PASS ENTIRE old2076 phase-timing+ancestors FIRST/all14 new arrays authenticated/'
      'ONLY complete PeerFront+PeerFetch+PeerAffine+PeerText change/10 other arrays+2MiB RoPE+10 inert ABI bodies exact/'
      'unchanged53913832B journal geometry/no conventional ABI,old namespace,CPU model reference or Work/'
      'static frames<=2048 or unchanged larger predecessor;NOT whole-kernel/combined-stack proof',flush=True)
print(digest+' '+obj.name,flush=True)
print('PRIVATE_BUILD_ONLY full-worker constant Q4 issue;NO native lifecycle,installation,original-model,physical,Work,TPS or speed grant',flush=True)
