#!/usr/bin/env python3
"""Actual PRIVATE failure-detail object; preserve whole old subjects first."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

PRIVATE_SHA='22a862c26595460dcd2c5e720bbe24079d2cb3a2b371e25d11337602dbbff5c3'
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[1]
out=pathlib.Path(sys.argv[1]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
def command(relative):
    values=(here/'benchmarks'/relative).read_text().splitlines()
    assert values.count('-c')==values.count('-o')==1
    oldroot=pathlib.Path(values[values.index('-c')+1]).parents[3]
    return [value.replace(str(oldroot)+'/',str(root)+'/') for value in values]
def compile(values,target,source=None):
    values=values[:];values[values.index('-o')+1]=str(target)
    if source is not None:values[values.index('-c')+1]=str(source)
    subprocess.run(values,check=True)
    (out/(target.stem+'-argv.txt')).write_text('\n'.join(values)+'\n')
    return values
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
private=command('qwen38-twin-batch-private-native-20261101/build-main/compile-argv.txt')
assert pathlib.Path(private[private.index('-c')+1]).name=='pioneer_q38_token_twin_batch_prefetch_native_context_compile.cpp'
control=out/'unchanged-twin-private.o';compile(private,control)
assert sha(control)=='feaa9a9f7323ce1759177c62ae84b39c5993cffab4263b00806d4b9e7638a0ec','ENTIRE old private object must reproduce FIRST'
public=command('qwen38-twin-batch-public-build-20261101/main/compile-argv.txt')
public_control=out/'unchanged-twin-public.o';compile(public,public_control)
assert sha(public_control)=='a0f0b93bf04325fc39074884a56ae0720e4370fef2c05457d726e1ba3504fe26','ENTIRE retired public object must remain exact,NOT installed'
obj=out/'frontier-failure-detail-NODRV.o'
compile(private,obj,here/'tests/pioneer_q38_frontier_failure_detail_native_context_compile.cpp')
tool=private[0].removesuffix('g++') if hasattr(str,'removesuffix') else private[0][:-3]
def records(path):
    raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True)
    symbols={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
    raw=subprocess.check_output([tool+'objdump','-t',str(path)],text=True)
    sections={row[-1]:row[-3] for row in map(str.split,raw.splitlines()) if len(row)>=6 and row[-1] in symbols}
    return symbols,sections
def payload(path,name,symbols,sections,label):
    first,size=symbols[name];target=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',sections[name],str(path),str(target)],check=True)
    data=target.read_bytes();assert first+size<=len(data)
    return data[first:first+size]
before,before_sections=records(control);after,after_sections=records(obj)
arrays=[name for name in before if 'PioneerQ38TokenTwinBatchPrefetchPrograms' in name and re.search(r'(?:Root|Peer)[A-Za-z]*E$',name)]
assert len(arrays)==14,len(arrays)
rope=[name for name,(_,size) in before.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1
for index,name in enumerate(arrays+rope):
    assert name in after and before[name][1]==after[name][1]
    assert payload(control,name,before,before_sections,'old-data-'+str(index))==payload(obj,name,after,after_sections,'new-data-'+str(index)),name
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
assert not set(abi)&after.keys()
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        symbol=prefix+name
        assert payload(control,symbol,before,before_sections,'old-'+symbol)==payload(obj,symbol,after,after_sections,'new-'+symbol)
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
assert 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<' in table
for word in ('Q38WorkControl','llama_','ggml_','ReferenceProducts','OriginalSource','ValidationSession',
             'NativeQ38PeerInstructionModel','Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO'):
    assert word not in table and word not in undefined,word
def frames(path):
    rows=[line.split('\t') for line in path.with_suffix('.su').read_text().splitlines()]
    assert rows and all(len(row)==3 and row[2]=='static' for row in rows)
    return {re.sub(r'^.*?:[0-9]+:[0-9]+:','',row[0]):int(row[1]) for row in rows}
old_frames,new_frames=frames(control),frames(obj)
for name,size in new_frames.items():assert size<=2048 or name in old_frames and size<=old_frames[name],(name,size)
observed=[(name,size) for name,size in new_frames.items() if 'ObserveCommittedFrontier' in name]
assert observed
assert sha(obj)==PRIVATE_SHA,'new PRIVATE subject must match its independent main/clean frozen bytes'
print('PASS whole old feaa PRIVATE+a0f0 RETIRED PUBLIC objects byte-exact FIRST;all14 firmware arrays+2MiB RoPE unchanged/'
      'all10 renamed ABI bodies/wrappers exact and conventional ABI absent/no Work,CPU reference or HOST engines;'
      'static frames<=2048 or unchanged larger predecessor;NOT whole kernel/IRQ/indirect stack proof',flush=True)
print('observer_frames '+repr(observed),flush=True)
print(sha(obj)+' '+obj.name,flush=True)
print('PRIVATE_BUILD_ONLY opt-in cached first-failure DATA;NO installable driver,retired010c retry,model,lifecycle,'
      'physical speed or Assistant Work qualification',flush=True)
