#!/usr/bin/env python3
"""Build/inspect the explicit fault-ROM diagnostic revision;NEVER install."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA='98555df8f038598c79bd69cca8e911ceb111a6ee6787faa041299cec10fffac0'
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
qualified,out=map(pathlib.Path,sys.argv[1:])
assert qualified.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
saved=sys.argv[:]
try:
    sys.argv=[str(here/'tests/run_pioneer_q38_token_fault_validation_native_compile.py'),str(qualified),str(out/'sdk')]
    compiled=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
obj,args,tool=compiled['obj'],compiled['args'][:],compiled['tool']
assert hashlib.sha256(obj.read_bytes()).hexdigest()==OBJECT_SHA,'separate actual fault-ROM object pin'
legacy=compiled['control']['control']['control']['control']['control']['base']
old=legacy['original'][legacy['original'].index('-o')+1]
links=[row for row in legacy['commands'] if '-shared' in row and old in row]
assert len(links)==1
link=links[0][:]
assert link[:4]==[args[0],'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[-2]=='-o' and [pathlib.Path(p).name for p in link[4:-2]]==[
    'crtbeginS.o','haiku_version_glue.o','tenstorrent_pci_probe.o','PioneerPciProfile.o','kernel.so','libgcc.a','crtendS.o']
module=out/'tenstorrent_pci_probe-token-DEBUG75-FAULT'
link=[str(obj) if value==old else value for value in link];link[-1]=str(module)
assert all(pathlib.Path(p).is_file() for p in link[4:-2])
subprocess.run(link,check=True,env=legacy['clean'])
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
names={r[-1] for r in map(str.split,table.splitlines()) if r}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for required in ('Q38TokenNativeControl','Q38TokenHiddenValidationControl','InspectTerminal','BeginQ38TokenNativeBackendScoped','FinishQ38TokenNativeBackendScoped'):
    assert required in table,required
for forbidden in ('OriginalSource','ReferenceProducts','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','SyntheticIO','pioneer_token_context_'):
    assert forbidden not in table and forbidden not in undefined,forbidden
graph_obj=out/'token-fault-validation-callgraph.o';graph_args=args[:]
graph_args[graph_args.index('-o')+1]=str(graph_obj)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True,env=legacy['clean'])
assert graph_obj.read_bytes()==obj.read_bytes()
graph=graph_obj.with_suffix('.ci').read_bytes();nodes,edges=parse_graph(graph.decode())
roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m'];assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph_obj)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report={'kind':'PARTIAL_ACTUAL_DEBUG75_FAULT_PUBLIC_ENTRY_STACK_AUDIT','roots':results,
        'module_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),'object_sha256':OBJECT_SHA,
        'graph_sha256':hashlib.sha256(graph).hexdigest(),'whole_kernel_stack_qualified':False,
        'limits':'Known module chains only;unknown indirect/kernel/entry/IRQ costs are excluded,NOT zero. '
                 'Separate SHA-qualified tag75 fault-ROM revision,not old75 or Work. No deployment or TPS grant.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
print(report['module_sha256']+' '+module.name)
print('PASS linked actual separate FAULT-ROM tag75 revision/conventional ABI/all13 pinned arrays+2MiB RoPE/old tag75 controls unchanged/no CPU oracle+Work+TEST imports')
print('OBSERVED actual ProbeControl knownModuleChain'+str(results[0]['known_module_chain_bytes'])+
      '/unresolved'+str(len(results[0]['unresolved']))+'/reached'+str(results[0]['reached_nodes'])+
      ';PARTIAL ONLY,wholeKernelStackQualified=FALSE;graph object BYTE-IDENTICAL')
print('BUILD+INSPECT ONLY;NO installation,device IO,full-model correctness,lifecycle,physical,Work or TPS grant')
