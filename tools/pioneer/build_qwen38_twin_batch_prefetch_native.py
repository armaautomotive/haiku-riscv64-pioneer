#!/usr/bin/env python3
"""Copy-free twin-batch PRIVATE native BUILD;NEVER install or execute."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA='feaa9a9f7323ce1759177c62ae84b39c5993cffab4263b00806d4b9e7638a0ec'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==2 and (inspect or PRIVATE_SHA)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,args);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_batched_prefetch_native.py'),str(previous),str(out/'unchanged-batched')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
control=parent['obj']
assert hashlib.sha256(control.read_bytes()).hexdigest()=='3a24eb79013746eff0b7aa4be2cdeafbac8169a605c606327a2427c249a00126'
command=parent['command'][:];tool=parent['tool']
obj=out/'token-twin-batch-prefetch-context-NODRV.o'
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_token_twin_batch_prefetch_native_context_compile.cpp')
command[command.index('-o')+1]=str(obj)
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')

def records(path):
    raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True)
    values={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
    raw=subprocess.check_output([tool+'objdump','-t',str(path)],text=True)
    sections={row[-1]:row[-3] for row in map(str.split,raw.splitlines()) if len(row)>=6 and row[-1] in values}
    return values,sections

def payload(path,name,values,sections,label):
    start,size=values[name];target=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',sections[name],str(path),str(target)],check=True)
    data=target.read_bytes();assert start+size<=len(data)
    return data[start:start+size]

symbols,sections=records(obj);old_symbols,old_sections=records(control)
header=(here/'PioneerQ38TokenTwinBatchPrefetchPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='049613d63e3524be9a5aa1d3ef5f234adf4c61e4142a4d79eaf7e3bd50ae9db1'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==14 and sum(label=='PeerFetch' for _,label in arrays)==1
for digest,label in arrays:
    found=[name for name in symbols if 'PioneerQ38TokenTwinBatchPrefetchPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(found)==1 and hashlib.sha256(payload(obj,found[0],symbols,sections,label)).hexdigest()==digest
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert payload(obj,rope[0],symbols,sections,'new-rope')==payload(control,rope[0],old_symbols,old_sections,'old-rope')
frames=parent['parent']['frames']
before,after=frames(control.with_suffix('.su')),frames(obj.with_suffix('.su'))
for name,size in after.items():assert size<=2048 or name in before and size<=before[name],(name,size)
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
assert not set(abi)&symbols.keys()
for name,kind in (('init_hardware','refusal'),('init_driver','refusal'),('uninit_driver','inert'),('publish_devices','null'),('find_device','null')):
    expected=payload(obj,'pioneer_token_context_expected_'+kind,symbols,sections,'inert-'+kind)
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        assert payload(obj,prefix+name,symbols,sections,prefix+name)==expected
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
for required in ('PioneerQ38TokenTwinBatchPrefetchPrograms','Q38TokenNativeControl(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<','BeginQ38TokenNativeBackendScoped(',
                 'FinishQ38TokenNativeBackendScoped('):assert required in table,required
for forbidden in ('PioneerQ38TokenBatchedPrefetchPrograms','PioneerQ38TokenPrefetchPrograms',
                  'PioneerQ38TokenFlatMatrixPrograms','PioneerQ38TokenPreciseBetaPrograms',
                  'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert forbidden not in table and forbidden not in undefined,forbidden
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:assert digest==PRIVATE_SHA
print('ACTUAL_PRIVATE5_BODIES_AND5_WRAPPERS_BYTEEXACT_INERT_CONTROLS/all14 NEW twin arrays+unchanged rope/'
      'ENTIRE old batched+prefetch+flat+ancestors FIRST/staticFrames<=2048 or unchanged older bound',flush=True)
print(digest+' '+obj.name,flush=True)
print('BUILD_ONLY distinct copy-free twin-batch PRIVATE context;NO conventional ABI,native lifecycle,install,model,'
      'physical,Work or TPS grant',flush=True)
