#!/usr/bin/env python3
"""Complete returning sampler PRIVATE native BUILD; never install or run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA = 'be835676df7838a238ef11cdd1941ca6489b6c54b26117ae462307ebdbe9f36f'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or PRIVATE_SHA)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_constant_q4_native.py'),str(out/'unchanged-constant-q4')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
hooks,tool=parent['hooks'],parent['tool']
assert hooks['sha'](parent['obj'])=='5e2c95a3887920990a3ef0b80cfd0057a68bc95b645f03476c7869d9dd622e63'
obj=out/'token-matrix-phase-NODRV.o';command=parent['command'][:]
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_token_matrix_phase_native_context_compile.cpp')
command[command.index('-o')+1]=str(obj)
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
before,before_sections=hooks['records'](parent['obj']);symbols,sections=hooks['records'](obj)
header=(here/'PioneerQ38TokenMatrixPhasePrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='e42b21bd0a5c0f061f3bbbd2cb620e988d03f561b62fdf444624959ddb03ca09'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==14
payload=parent['payload'];changed=[]
for digest,label in arrays:
    names=[name for name in symbols if 'PioneerQ38TokenMatrixPhasePrograms' in name and name.endswith(str(len(label))+label+'E')]
    old_names=[name for name in before if 'PioneerQ38TokenConstantQ4Programs' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==len(old_names)==1
    current=payload(obj,names[0],symbols,sections,'matrix-new-'+label)
    previous=payload(parent['obj'],old_names[0],before,before_sections,'matrix-old-'+label)
    assert hashlib.sha256(current).hexdigest()==digest
    if current!=previous:changed.append(label)
assert changed==['PeerFront','PeerService','PeerFetch','PeerAffine','PeerText'],changed
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in before
assert payload(obj,rope[0],symbols,sections,'matrix-new-rope')==payload(parent['obj'],rope[0],before,before_sections,'matrix-old-rope')
assert not set(hooks['abi'])&symbols.keys()
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        symbol=prefix+name
        assert payload(obj,symbol,symbols,sections,'matrix-new-'+symbol)==payload(parent['obj'],symbol,before,before_sections,'matrix-old-'+symbol)
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
for name in ('PioneerQ38TokenMatrixPhasePrograms','ReadQ38TokenNativePhaseProfile(',
             'PioneerQ38TokenNativeOwner::ReadPhaseProfile<','Q38TokenNativeControl(',
             'BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):assert name in table,name
for name in ('PioneerQ38TokenConstantQ4Programs','PioneerQ38TokenPhaseProfilePrograms','PioneerQ38TokenWideGdnPrograms',
             'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
previous,current=hooks['frames'](parent['obj']),hooks['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:assert digest==PRIVATE_SHA
print('PASS entire old5e2 constant-Q4 PRIVATE+ancestors FIRST/all14 new arrays authenticated/'
      'ONLY five complete peer sections change/9 others+2MiB RoPE+10 inert ABI bodies exact/'
      'unchanged53913832B journal/no conventional ABI,old namespace,CPU oracle or Work/'
      'static frames<=2048 or unchanged predecessors;NOT whole-kernel stack qualification',flush=True)
print(digest+' '+obj.name,flush=True)
print('PRIVATE_BUILD_ONLY complete returning sampler;NO held sampler readout,lifecycle,model,physical,Work,TPS or deployment grant',flush=True)
