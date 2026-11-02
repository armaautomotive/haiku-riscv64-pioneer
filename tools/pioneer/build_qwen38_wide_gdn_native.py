#!/usr/bin/env python3
"""Complete new returning-GDN PRIVATE native subject; never install or run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA = 'de2f80f0419a7cbbaf4082db763de7737959e13aab6dee58f75b63a90bcce3ae'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or PRIVATE_SHA)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_batched_output_closure_native.py'),str(out/'unchanged-corrected-output')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
hooks,tool=parent['hooks'],parent['tool']
assert hooks['sha'](parent['obj'])=='b688d89b569495d4d027b3880f7e28f2e1ece15974c3d10823f0cfa6d7c36a30'
obj=out/'wide-gdn-NODRV.o';command=parent['command'][:]
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_wide_gdn_native_context_compile.cpp')
command[command.index('-o')+1]=str(obj)
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')

def payload(path,name,values,mapping,label):
    first,size=values[name];target=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',mapping[name],str(path),str(target)],check=True)
    data=target.read_bytes();assert first+size<=len(data)
    return data[first:first+size]

before,before_sections=hooks['records'](parent['obj']);symbols,sections=hooks['records'](obj)
header=(here/'PioneerQ38TokenWideGdnPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='d285c9adb421fc0010d8ad48371eb58ca1480fc0d787f7c147afd4a93457a787'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==14
changed=[]
for digest,label in arrays:
    names=[name for name in symbols if 'PioneerQ38TokenWideGdnPrograms' in name and name.endswith(str(len(label))+label+'E')]
    old_names=[name for name in before if 'PioneerQ38TokenBatchedOutputClosurePrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==len(old_names)==1
    current=payload(obj,names[0],symbols,sections,'new-'+label)
    previous=payload(parent['obj'],old_names[0],before,before_sections,'old-'+label)
    assert hashlib.sha256(current).hexdigest()==digest
    if current!=previous:changed.append(label)
assert changed==['PeerFront','PeerGdn'],changed
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
for name in ('PioneerQ38TokenWideGdnPrograms','Q38TokenNativeControl(',
             'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
             'BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):assert name in table,name
for name in ('PioneerQ38TokenBatchedOutputClosurePrograms','PioneerQ38TokenTwinBatchPrefetchPrograms',
             'PioneerQ38TokenBatchedPrefetchPrograms','PioneerQ38TokenPrefetchPrograms','PioneerQ38TokenFlatMatrixPrograms',
             'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
previous,current=hooks['frames'](parent['obj']),hooks['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:assert digest==PRIVATE_SHA
print('PASS ENTIRE oldb688 corrected-output+QFL1 controls FIRST/all14 new arrays authenticated/'
      'ONLY complete PeerFront+PeerGdn change/ROOT+12 other arrays+2MiB RoPE+10 inert ABI bodies exact/'
      'unchanged53913832B journal geometry/no conventional ABI,old namespace,CPU model reference or Work/'
      'static frames<=2048 or unchanged larger predecessor;NOT whole-kernel/combined-stack proof',flush=True)
print(digest+' '+obj.name,flush=True)
print('PRIVATE_BUILD_ONLY returning wide GDN;NO native lifecycle,installation,original-model,physical,Work,TPS or speed grant',flush=True)
