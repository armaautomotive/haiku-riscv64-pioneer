#!/usr/bin/env python3
"""Build isolated prefetch native subject; NEVER install or execute it."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

PRIVATE_SHA='e4e8cc3f616d8cc5c34789326ed3fdd8e3e920cf96c50ce19658d1d9b934f845'
PUBLIC_OBJECT_SHA='30691de52d1f34dcc01725b8d068c54fa283419ec97da7b15c58ac02729ba9a0'
PUBLIC_MODULE_SHA='919d4a631ff664549a67bb2372ce2db344ba29473c452c0b273c4b27a2bdf978'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==3 and args[0] in ('--private','--physical')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
physical=args[0]=='--physical'
assert inspect or (PUBLIC_OBJECT_SHA and PUBLIC_MODULE_SHA if physical else PRIVATE_SHA)
previous,out=map(pathlib.Path,args[1:]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
# Reproduce the entire qualified old subject FIRST from authenticated argv.
saved=sys.argv[:]
try:
    sys.argv=[str(here/'tests/run_pioneer_q38_token_prefetch_old_native_controls.py'),
              args[0],str(previous),str(out/'unchanged-flat')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
compile_args=old['args'][:];tool=compile_args[0][:-3];control=old['target']
source=('pioneer_q38_token_prefetch_benchmark_native_compile.cpp' if physical else
        'pioneer_q38_token_prefetch_native_context_compile.cpp')
obj=out/('token-prefetch-benchmark-native.o' if physical else 'token-prefetch-context-NODRV.o')
compile_args[compile_args.index('-c')+1]=str(here/'tests'/source)
compile_args[compile_args.index('-o')+1]=str(obj)
subprocess.run(compile_args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(compile_args)+'\n')

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
header=(here/'PioneerQ38TokenPrefetchPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='021fdff4100dd6f8a6ff8548f9fa961473503cf6c2cd2e0b6adc44053b007567'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==14 and sum(label=='PeerFetch' for _,label in arrays)==1
for digest,label in arrays:
    found=[name for name in symbols if 'PioneerQ38TokenPrefetchPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(found)==1 and hashlib.sha256(payload(obj,found[0],symbols,sections,label)).hexdigest()==digest,label
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert payload(obj,rope[0],symbols,sections,'new-rope')==payload(control,rope[0],old_symbols,old_sections,'old-rope')

def frames(path):
    values={}
    for line in path.read_text().splitlines():
        row=line.split('\t');found=re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)',row[0])
        assert len(row)==3 and found and row[2]=='static',line
        values[found[1]]=int(row[1])
    return values

before,after=frames(control.with_suffix('.su')),frames(obj.with_suffix('.su'))
for name,size in after.items():assert size<=2048 or name in before and size<=before[name],(name,size)
module=None
if physical:
    module=out/'tenstorrent_pci_probe-token-PREFETCH34-BENCHMARK';link=old['link'][:]
    link[6],link[-1]=str(obj),str(module);subprocess.run(link,check=True)
    (out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module or obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module or obj)],text=True)
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
if physical:assert all(name in symbols for name in abi)
else:
    assert not set(abi)&symbols.keys()
    for name,kind in (('init_hardware','refusal'),('init_driver','refusal'),('uninit_driver','inert'),('publish_devices','null'),('find_device','null')):
        expected=payload(obj,'pioneer_token_context_expected_'+kind,symbols,sections,'inert-'+kind)
        for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
            assert payload(obj,prefix+name,symbols,sections,prefix+name)==expected,(prefix,name)
    print('ACTUAL_PRIVATE5_BODIES_AND5_WRAPPERS_BYTEEXACT_INDEPENDENT_INERT_CONTROLS',flush=True)
for required in ('Q38TokenNativeControl(','ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<','Q38TokenFrontierControl(',
                 'PioneerQ38TokenPrefetchPrograms','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):
    assert required in table,required
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_',
                  'PioneerQ38TokenFlatMatrixPrograms','PioneerQ38TokenEmptyReceiptPollPrograms','PioneerQ38TokenCounterGatherPrograms',
                  'PioneerQ38TokenTidBoundaryPrograms','PioneerQ38TokenPacketIdentityBoundaryPrograms','PioneerQ38TokenCohortLocalSourcesPrograms',
                  'PioneerQ38TokenCommitBoundaryPrograms','PioneerQ38TokenRetainedCohortPrograms','PioneerQ38TokenFrontierPrograms',
                  'PioneerQ38TokenJobBootFoldPrograms','PioneerQ38TokenPreciseBetaPrograms','PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined,forbidden
if physical:
    graph=out/'token-prefetch-callgraph.o';graph_args=compile_args[:];graph_args[graph_args.index('-o')+1]=str(graph)
    subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True);assert graph.read_bytes()==obj.read_bytes()
    sys.path.insert(0,str(here/'tests'))
    from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
    nodes,edges=parse_graph(graph.with_suffix('.ci').read_text());roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m']
    assert len(roots)==1
    functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph)],text=True))
    results=audit(nodes,edges,functions,roots)
    assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
    assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
    report={'kind':'PARTIAL_ACTUAL_PREFETCH_PUBLIC_STACK_AUDIT','roots':results,
            'module_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),
            'whole_kernel_stack_qualified':False,'limits':'Known module only;kernel/IRQ/entry/indirect costs excluded. NOT Work qualification.'}
    (out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PARTIAL knownModuleChain'+str(results[0]['known_module_chain_bytes'])+'/unresolved'+str(len(results[0]['unresolved']))+';wholeKernelStackQualified=FALSE',flush=True)
for path,expected in ((obj,PUBLIC_OBJECT_SHA if physical else PRIVATE_SHA),*([(module,PUBLIC_MODULE_SHA)] if physical else [])):
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name,flush=True)
print('BUILD_ONLY prefetch14-array binding and preserved old controls;NO native execution,lifecycle,installation,model quality,Work or TPS grant',flush=True)
