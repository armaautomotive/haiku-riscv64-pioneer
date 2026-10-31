#!/usr/bin/env python3
"""Compile/inspect new exact physical diagnostic; NEVER install or run it."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys
assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==4
assert not inspect or sys.argv[3]=='--inspect-candidate'
OBJECT_SHA='ed855c3fc7eedf64c8ef806f1386f50982f604fd56bb8abdf0afe1c4e09bca8c'
MODULE_SHA='9ad9e86bd5e45c5cb11dfe286f3528452dfe7b0e4630f67546acbb43a03064e8'
assert inspect or OBJECT_SHA and MODULE_SHA,'candidate not independently frozen'
previous,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_decode_benchmark_native.py'),str(previous),str(out/'unchanged-decode-driver')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
# First reproduce ENTIRE old4ebc9463,44209,18362 controls,not just flags.
args=old['args'][:];tool=old['tool'];obj=out/'frontier-benchmark-native.o'
args[args.index('-c')+1]=str(here/'tests/pioneer_q38_token_frontier_benchmark_native_compile.cpp')
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
frames=old['control']['compiled']['frames'](obj.with_suffix('.su'))
for name,size in frames.items():assert size<=2048 or name in old['frames'] and size<=old['frames'][name],(name,size)
module=out/'tenstorrent_pci_probe-token-FRONTIER34-BENCHMARK';link=old['link'][:]
link[6]=str(obj);link[-1]=str(module);subprocess.run(link,check=True)
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
for required in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
                 'Q38TokenNativeControl(', 'Q38TokenFrontierControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'PioneerQ38TokenFrontierPrograms',
                 'BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped(','PostedCancelSourceGuard'):
    assert required in table,required
for forbidden in ('Q38TokenHiddenValidationControl(','OriginalSource','ReferenceProducts','ValidationSession',
                  'Q38WorkControl','NativeQ38PeerInstructionModel','SyntheticIO','PostedCancelEndpoint',
                  'PioneerQ38TokenJobBootFoldPrograms','PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined,forbidden
raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={row[-1]:row[-3] for row in map(str.split,sections.splitlines()) if len(row)>=6 and row[-1] in symbols}
def payload(name,label):
    start,size=symbols[name];path=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section[name],str(obj),str(path)],check=True)
    data=path.read_bytes();assert start+size<=len(data);return data[start:start+size]
header=(here/'PioneerQ38TokenFrontierPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='2bd70b273e8c6b2196c0d087ca03d87f34a5567cb1cb361b305483ce4b17c7e5'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(arrays)==13
for digest,label in arrays:
    names=[name for name in symbols if 'PioneerQ38TokenFrontierPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==1 and hashlib.sha256(payload(names[0],label)).hexdigest()==digest,label
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and payload(rope[0],'new-rope')==old['extract'](rope[0],'expected-rope')
graph_obj=out/'frontier-callgraph.o';graph_args=args[:];graph_args[graph_args.index('-o')+1]=str(graph_obj)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True);assert graph_obj.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
nodes,edges=parse_graph(graph_obj.with_suffix('.ci').read_text())
roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m'];assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph_obj)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report={'kind':'PARTIAL_ACTUAL_FRONTIER_PUBLIC_STACK_AUDIT','roots':results,
        'module_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),
        'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),'whole_kernel_stack_qualified':False,
        'limits':'Known module only;unknown kernel/IRQ/entry/indirect costs excluded,not zero. NO install or Work grant.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
for path,expected in ((obj,OBJECT_SHA),(module,MODULE_SHA)):
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name)
print('PASS_BUILD_ONLY new conventional6607+6608 diagnostic/old4eb+44209+18362 controls unchanged/'
      'all13 actual arrays+RoPE exact/new source+sameowner counter/lifecycle/noCPU oracle+Work+TEST imports')
print('PARTIAL knownModuleChain'+str(results[0]['known_module_chain_bytes'])+'/unresolved'+str(len(results[0]['unresolved']))+
      ';wholeKernelStackQualified=FALSE;NO physical,numerical,steadyTPS,residentWork or deployment grant')
