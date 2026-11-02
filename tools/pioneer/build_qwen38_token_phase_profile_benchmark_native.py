#!/usr/bin/env python3
"""Separate full-token phase-timing bounded diagnostic BUILD; never stage/install/run."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA = '96be424ab13c3f05a853d4824d0684b5e657272135d886c5fd7c9010b743fe67'
MODULE_SHA = '0f2066ca48b24ae36550e57dd5334aa29645deb7f65dc8fd7bbbb9629f674e86'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or OBJECT_SHA and MODULE_SHA)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
def parent(script,directory):
    saved=sys.argv[:]
    try:
        sys.argv=[str(here/script),str(directory)]
        return runpy.run_path(sys.argv[0],run_name='__main__')
    finally:sys.argv=saved
old=parent('build_qwen38_wide_gdn_benchmark_native.py',out/'unchanged-wide-gdn')
private_saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_phase_profile_native.py'),str(out/'qualified-private-phase-timing'),*(['--inspect-candidate'] if inspect else [])]
    private=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=private_saved
hooks,tool=old['hooks'],old['tool'];sha=hooks['sha']
assert sha(old['module'])=='b8c0d49855490e756dfe1c49bed7933a20f2a3afea19436b37e3b4717417b8ed'
assert inspect or sha(private['obj'])==private['PRIVATE_SHA']
obj=out/'token-phase-profile-benchmark-native.o';command=old['command'][:]
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_token_phase_profile_benchmark_native_compile.cpp')
command[command.index('-o')+1]=str(obj)
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
symbols,sections=hooks['records'](obj)
def payload(path,name,values,mapping,label):
    first,size=values[name];target=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',mapping[name],str(path),str(target)],check=True)
    data=target.read_bytes();assert first+size<=len(data)
    return data[first:first+size]
for digest,label in private['arrays']:
    names=[n for n in symbols if 'PioneerQ38TokenPhaseProfilePrograms' in n and n.endswith(str(len(label))+label+'E')]
    assert len(names)==1
    assert hashlib.sha256(payload(obj,names[0],symbols,sections,'public-'+label)).hexdigest()==digest
rope=private['rope'];assert len(rope)==1 and rope[0] in symbols and rope[0] in old['symbols']
assert payload(obj,rope[0],symbols,sections,'public-rope')==payload(old['obj'],rope[0],old['symbols'],old['sections'],'old-rope')
previous,current=hooks['frames'](old['obj']),hooks['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
assert all(name in symbols for name in abi)
module=out/'tenstorrent_pci_probe-TOKEN-PHASE34-DIAGNOSTIC-ONLY';link=old['link'][:]
assert link[6]==str(old['obj']) and link[-1]==str(old['module'])
link[6],link[-1]=str(obj),str(module)
subprocess.run(link,check=True);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
for name in ('PioneerQ38TokenPhaseProfilePrograms','ReadQ38TokenNativePhaseProfile(',
             'PioneerQ38TokenNativeOwner::ReadPhaseProfile<','Q38TokenPhaseProfileControl(','Q38TokenNativeControl(','ObserveQ38TokenNativeFrontier(',
             'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<','Q38TokenFrontierControl(',
             'BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):assert name in table,name
for name in ('PioneerQ38TokenWideGdnPrograms','PioneerQ38TokenTwinBatchPrefetchPrograms',
             'PioneerQ38TokenBatchedPrefetchPrograms','PioneerQ38TokenPrefetchPrograms','PioneerQ38TokenFlatMatrixPrograms',
             'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
graph=out/'token-phase-callgraph.o';graph_args=command[:];graph_args[graph_args.index('-o')+1]=str(graph)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True);assert graph.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
nodes,edges=parse_graph(graph.with_suffix('.ci').read_text());roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m']
assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
(out/'partial-public-entry-stack.json').write_text(json.dumps({
    'kind':'PARTIAL_ACTUAL_TOKEN_PHASE_PUBLIC_STACK_AUDIT','roots':results,
    'module_sha256':sha(module),'object_sha256':sha(obj),'whole_kernel_stack_qualified':False,
    'limits':'Known module ONLY;kernel/IRQ/entry/indirect costs excluded. NOT lifecycle,Work or deployment qualification.'},indent=2)+'\n')
print('PARTIAL token-phase knownModuleChain'+str(results[0]['known_module_chain_bytes'])+
      '/unresolved'+str(len(results[0]['unresolved']))+';wholeKernelStackQualified=FALSE',flush=True)
for path,expected in ((obj,OBJECT_SHA),(module,MODULE_SHA)):
    digest=sha(path)
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name,flush=True)
print('PUBLIC_BUILD_ONLY full phase-timing ROOT+unchanged returning-GDN/all14 new arrays+2MiB RoPE authenticated/'
      'whole old PRIVATE+PUBLIC and frozen new phase PRIVATE controls FIRST/unchanged journal/no CPU reference or Work;'
      'NO native lifecycle,installation,model,physical,TPS or speed grant',flush=True)
