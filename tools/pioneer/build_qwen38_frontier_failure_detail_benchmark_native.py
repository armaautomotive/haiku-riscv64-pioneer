#!/usr/bin/env python3
"""Separate public diagnostic BUILD only. Never stage,install or execute."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

OBJECT_SHA='0e984211811f01a792542fdd1bf5eeb06c99a669ef9f5277a9ee3f23d568d008'
MODULE_SHA='592084df7cc967b688f0f8ef84421165aaeb4e5bb2b7d2e9ca739bf0188c6716'
args=sys.argv[1:];inspect='--inspect-candidate' in args
if inspect:assert args.count('--inspect-candidate')==1;args.remove('--inspect-candidate')
assert len(args)==1 and (inspect or OBJECT_SHA and MODULE_SHA)
assert not any(key.startswith(('PIONEER_','TT_PIONEER_')) for key in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_frontier_failure_detail_native.py'),str(out/'unchanged-private')]
    parent=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
sha=parent['sha'];tool=parent['tool'];control=parent['public_control']
assert sha(parent['obj'])=='22a862c26595460dcd2c5e720bbe24079d2cb3a2b371e25d11337602dbbff5c3'
assert sha(control)=='a0f0b93bf04325fc39074884a56ae0720e4370fef2c05457d726e1ba3504fe26'
# Reproduce the ENTIRE retired module too. It is a control,NEVER installed.
link=(here/'benchmarks/qwen38-twin-batch-public-build-20261101/main/link-argv.txt').read_text().splitlines()
assert pathlib.Path(link[6]).name=='token-twin-batch-prefetch-benchmark-native.o' and link[-2]=='-o'
link[6]=str(control);old_module=out/'RETIRED010c-control-NOT-FOR-INSTALL'
link[-1]=str(old_module);subprocess.run(link,check=True)
assert sha(old_module)=='010c184ef474f962205918fd9a9a34b23b2ab47c3cf6a62d285fcb36d2e9e899'
obj=out/'frontier-failure-detail-benchmark-native.o'
command=parent['public'][:];command[command.index('-o')+1]=str(obj)
command[command.index('-c')+1]=str(here/'tests/pioneer_q38_frontier_failure_detail_benchmark_native_compile.cpp')
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
symbols,sections=parent['records'](obj);old_symbols,old_sections=parent['records'](control)
header=(here/'PioneerQ38TokenTwinBatchPrefetchPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='049613d63e3524be9a5aa1d3ef5f234adf4c61e4142a4d79eaf7e3bd50ae9db1'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==14
for digest,label in arrays:
    found=[name for name in symbols if 'PioneerQ38TokenTwinBatchPrefetchPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(found)==1
    data=parent['payload'](obj,found[0],symbols,sections,'public-detail-'+label)
    assert hashlib.sha256(data).hexdigest()==digest
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert parent['payload'](obj,rope[0],symbols,sections,'public-detail-rope')==parent['payload'](control,rope[0],old_symbols,old_sections,'public-control-rope')
before,after=parent['frames'](control),parent['frames'](obj)
for name,size in after.items():assert size<=2048 or name in before and size<=before[name],(name,size)
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
assert all(name in symbols for name in abi)
module=out/'tenstorrent_pci_probe-FRONTIER-EARLY-FAILURE-DIAGNOSTIC-ONLY'
link[6]=str(obj);link[-1]=str(module);subprocess.run(link,check=True)
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
for name in ('PioneerQ38TokenTwinBatchPrefetchPrograms','Q38TokenNativeControl(',
             'ObserveQ38TokenNativeFrontier(','PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
             'Q38TokenFrontierControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):
    assert name in table,name
for name in ('PioneerQ38TokenBatchedPrefetchPrograms','PioneerQ38TokenPrefetchPrograms','PioneerQ38TokenFlatMatrixPrograms',
             'ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
             'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert name not in table and name not in undefined,name
graph=out/'frontier-failure-detail-callgraph.o';graph_args=command[:]
graph_args[graph_args.index('-o')+1]=str(graph)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True)
assert graph.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
nodes,edges=parse_graph(graph.with_suffix('.ci').read_text())
roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m'];assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report={'kind':'PARTIAL_ACTUAL_FRONTIER_FAILURE_DETAIL_PUBLIC_STACK_AUDIT','roots':results,
        'module_sha256':sha(module),'object_sha256':sha(obj),'whole_kernel_stack_qualified':False,
        'limits':'Known module ONLY;kernel/IRQ/entry/indirect costs excluded. NOT native lifecycle,Work or deployment qualification.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
print('PARTIAL knownModuleChain'+str(results[0]['known_module_chain_bytes'])+'/unresolved'+str(len(results[0]['unresolved']))+
      ';wholeKernelStackQualified=FALSE',flush=True)
for path,expected in ((obj,OBJECT_SHA),(module,MODULE_SHA)):
    digest=sha(path)
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name,flush=True)
print('PUBLIC_BUILD_ONLY distinct cached first-failure diagnostic/ENTIRE old privatefeaa+publica0f0+retired010c+'
      'qualified private22a FIRST/all14 unchanged arrays+2MiB RoPE/no runtime CPU reference or Work;'
      'NO native lifecycle,install,retired010c retry,hardware safety,model,TPS or speedup grant',flush=True)
