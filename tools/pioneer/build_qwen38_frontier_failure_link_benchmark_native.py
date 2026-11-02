#!/usr/bin/env python3
"""New public QFL1 diagnostic BUILD only. Never stage/install/execute."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA='370722a7e54c9b76a2bff335d9ecce97528e40316aef75145d3da5770ed98400'
MODULE_SHA='96a030daf8136143aafa1f518b2ff3323178b5aa7b530c7d9395db5bf447e3e2'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or OBJECT_SHA and MODULE_SHA)
assert not any(key.startswith(('PIONEER_','TT_PIONEER_')) for key in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
def parent(script,directory):
    saved=sys.argv[:]
    try:
        sys.argv=[str(here/script),str(directory)]
        return runpy.run_path(sys.argv[0],run_name='__main__')
    finally:sys.argv=saved
old=parent('build_qwen38_frontier_failure_detail_benchmark_native.py',out/'RETIRED-QFD1-control-NOT-FOR-INSTALL')
private=parent('build_qwen38_frontier_failure_link_native.py',out/'qualified-private-QFL1')
hooks=old['parent'];sha=hooks['sha'];tool=hooks['tool']
assert sha(old['module'])=='592084df7cc967b688f0f8ef84421165aaeb4e5bb2b7d2e9ca739bf0188c6716'
assert sha(private['obj'])=='3593260a72978b739120eb26a9ba866a622f7a6369b0ffbda4e4bce6c8b518e3'
sys.stdout.flush()
obj=out/'frontier-failure-link-benchmark-native.o'
command=old['command'][:];command[command.index('-o')+1]=str(obj)
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_frontier_failure_link_benchmark_native_compile.cpp')
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
symbols,sections=hooks['records'](obj)
for digest,label in old['arrays']:
    found=[name for name in symbols if 'PioneerQ38TokenTwinBatchPrefetchPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(found)==1
    data=hooks['payload'](obj,found[0],symbols,sections,'public-link-'+label)
    assert hashlib.sha256(data).hexdigest()==digest
rope=old['rope'];assert len(rope)==1 and rope[0] in symbols
assert hooks['payload'](obj,rope[0],symbols,sections,'public-link-rope')==hooks['payload'](old['obj'],rope[0],old['symbols'],old['sections'],'old-public-detail-rope')
previous,current=hooks['frames'](old['obj']),hooks['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
assert all(name in symbols for name in old['abi'])
module=out/'tenstorrent_pci_probe-FRONTIER-PREERROR-LINK-DIAGNOSTIC-ONLY'
link=old['link'][:];link[6]=str(obj);link[-1]=str(module)
subprocess.run(link,check=True);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
for name in ('PioneerQ38TokenTwinBatchPrefetchPrograms','Q38TokenNativeControl(',
             'ObserveQ38TokenNativeFrontier(','PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
             'Q38TokenFrontierControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):assert name in table,name
for name in ('PioneerQ38TokenBatchedPrefetchPrograms','PioneerQ38TokenPrefetchPrograms','PioneerQ38TokenFlatMatrixPrograms',
             'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
graph=out/'frontier-failure-link-callgraph.o';graph_args=command[:];graph_args[graph_args.index('-o')+1]=str(graph)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True);assert graph.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
nodes,edges=parse_graph(graph.with_suffix('.ci').read_text())
roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m'];assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report={'kind':'PARTIAL_ACTUAL_QFL1_PUBLIC_STACK_AUDIT','roots':results,
        'module_sha256':sha(module),'object_sha256':sha(obj),'whole_kernel_stack_qualified':False,
        'limits':'Known module ONLY;kernel/IRQ/entry/indirect costs excluded. NOT native lifecycle,Work or deployment qualification.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
print('PARTIAL_QFL1 knownModuleChain'+str(results[0]['known_module_chain_bytes'])+'/unresolved'+str(len(results[0]['unresolved']))+';wholeKernelStackQualified=FALSE',flush=True)
for path,expected in ((obj,OBJECT_SHA),(module,MODULE_SHA)):
    digest=sha(path)
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name,flush=True)
print('PUBLIC_BUILD_ONLY QFL1 ONE pre-error local savedLINK read;whole old PRIVATE/PUBLIC/retired010c+5920 and frozen3593 private FIRST;'
      'all14 firmware arrays+2MiB RoPE exact/no CPU model reference or Work;NO install,native lifetime,physical fault cause or TPS grant',flush=True)
