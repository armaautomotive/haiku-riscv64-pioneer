#!/usr/bin/env python3
"""PRIVATE fresh-slot+held reader BUILD;never install or run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA = '1a04292b5246421f94f398c7d956459eed732c2aad20cf3f850ea8816235141c'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or PRIVATE_SHA)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_matrix_phase_native.py'),str(out/'unchanged-sampler')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
hooks,tool=parent['hooks'],parent['tool']
assert hooks['sha'](parent['obj'])=='be835676df7838a238ef11cdd1941ca6489b6c54b26117ae462307ebdbe9f36f'
obj=out/'token-matrix-phase-read-NODRV.o';command=parent['command'][:]
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_token_matrix_phase_read_native_context_compile.cpp')
command[command.index('-o')+1]=str(obj)
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
before,before_sections=hooks['records'](parent['obj']);symbols,sections=hooks['records'](obj)
payload=parent['payload']
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',
                  (here/'PioneerQ38TokenMatrixPhasePrograms.h').read_text())
assert len(arrays)==14
for digest,label in arrays:
    names=[n for n in symbols if 'PioneerQ38TokenMatrixPhasePrograms' in n and n.endswith(str(len(label))+label+'E')]
    assert len(names)==1 and names[0] in before;name=names[0]
    current=payload(obj,name,symbols,sections,'read-new-'+label)
    assert hashlib.sha256(current).hexdigest()==digest
    assert current==payload(parent['obj'],name,before,before_sections,'read-old-'+label)
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in before
assert payload(obj,rope[0],symbols,sections,'read-new-rope')==payload(parent['obj'],rope[0],before,before_sections,'read-old-rope')
assert not set(hooks['abi'])&symbols.keys()
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        symbol=prefix+name
        assert payload(obj,symbol,symbols,sections,'read-new-'+symbol)==payload(parent['obj'],symbol,before,before_sections,'read-old-'+symbol)
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
for name in ('ReadQ38TokenNativeMatrixPhase(','PioneerQ38TokenNativeOwner::ReadMatrixPhase<',
             'pioneer_token_context_scoped_matrix_phase'):
    assert name not in parent['table'],name
for name in ('ReadQ38TokenNativeMatrixPhase(','PioneerQ38TokenNativeOwner::ReadMatrixPhase<',
             'pioneer_token_context_scoped_matrix_phase','PioneerQ38TokenMatrixPhasePrograms'):
    assert name in table,name
for name in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
previous,current=hooks['frames'](parent['obj']),hooks['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:assert digest==PRIVATE_SHA
print('PRIVATE_BUILD_ONLY entire unchanged be835 sampler+ancestors FIRST/all code arrays+RoPE+10 inert ABI bodies exact/'
      'fresh16word peer-slot zero+readback/held-drained-doublechecked reader/static frames<=2048 or unchanged predecessors;'
      'NOT lifecycle,whole-stack,model,physical,Work,TPS or deployment qualification',flush=True)
print(digest+' '+obj.name,flush=True)
