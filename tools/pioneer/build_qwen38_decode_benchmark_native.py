#!/usr/bin/env python3
"""Compile/inspect separate longer diagnostic. NEVER install or run hardware."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys
OBJECT_SHA='66b01f59eb4d88a1360c3d8263507924bad0c2c7997fc78c86e0176acb84c088'
MODULE_SHA='4ebc9463b80cfa73fe957800cadac819e7ff05e17ff7a2db7fd6362da2c43b14'
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_precise_beta_native.py'),str(previous),str(out/'unchanged-short-driver'),'--job-boot-fold']
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
# The complete old44209 and posted18362 controls have just reproduced.
args=control['args'][:];tool=control['tool'];cxx=args[0]
obj=out/'decode-benchmark-native.o'
args[args.index('-c')+1]=str(here/'tests/pioneer_q38_token_decode_benchmark_native_compile.cpp')
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
frames=control['compiled']['frames'](obj.with_suffix('.su'))
old_frames=control['compiled']['new_frames']
for name,size in frames.items():assert size<=2048 or name in old_frames and size<=old_frames[name],(name,size)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
link=control['link'][:];module=out/'tenstorrent_pci_probe-token-DECODE34-BENCHMARK'
link[6]=str(obj);link[-1]=str(module);subprocess.run(link,check=True)
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
for required in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
                 'Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped(',
                 'PioneerQ38TokenJobBootFoldPrograms','PostedCancelSourceGuard'):
    assert required in table,required
for forbidden in ('Q38TokenHiddenValidationControl(','OriginalSource','ReferenceProducts','ValidationSession',
                  'Q38WorkControl','NativeQ38PeerInstructionModel','SyntheticIO','PostedCancelEndpoint',
                  'PioneerQ38TokenPreciseBetaPrograms','PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined,forbidden
# Authenticate all actual firmware arrays, not just namespace strings.
raw_table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw_table.splitlines()) if len(row)==4}
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={row[-1]:row[-3] for row in map(str.split,sections.splitlines()) if len(row)>=6 and row[-1] in symbols}
header=(here/'PioneerQ38TokenJobBootFoldPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='1b8e610966e7a8b69d545db7dd9f58f326b4d3937a354960930c858e8e92472b'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(expected)==13
def extract(name,label):
    start,size=symbols[name];target=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section[name],str(obj),str(target)],check=True)
    data=target.read_bytes();assert start+size<=len(data);return data[start:start+size]
for digest,label in expected:
    names=[name for name in symbols if 'PioneerQ38TokenJobBootFoldPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==1 and hashlib.sha256(extract(names[0],label)).hexdigest()==digest,label
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1
# The control SDK saved its ENTIRE section. Compare the actual symbol's
# bounded slice, not that larger section (which also contains firmware).
old_start,old_size,_=control['compiled']['symbols'][rope[0]]
old_section=(out/'unchanged-short-driver/sdk/new-rope.raw').read_bytes()
assert old_size==2097152 and old_start+old_size<=len(old_section)
assert extract(rope[0],'rope')==old_section[old_start:old_start+old_size]
graph_obj=out/'decode-benchmark-callgraph.o';graph_args=args[:];graph_args[graph_args.index('-o')+1]=str(graph_obj)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True);assert graph_obj.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
graph=graph_obj.with_suffix('.ci').read_bytes();nodes,edges=parse_graph(graph.decode())
roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m'];assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph_obj)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report={'kind':'PARTIAL_ACTUAL_DECODE34_PUBLIC_STACK_AUDIT','roots':results,
        'module_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),
        'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),'whole_kernel_stack_qualified':False,
        'limits':'Known module chains only;unknown kernel/entry/IRQ/indirect costs excluded,NOT zero;NO deployment or Work authority.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
for path,expected_sha in ((obj,OBJECT_SHA),(module,MODULE_SHA)):
    actual=hashlib.sha256(path.read_bytes()).hexdigest()
    assert actual==expected_sha,(path,actual)
    print(actual+' '+path.name)
print('PASS_FROZEN separate34-position physical diagnostic/old44209+18362 literal controls unchanged/'
      'all13 firmware arrays+2MiB RoPE exact/conventionalABI/no CPU oracle+Work+MOCK imports/'
      'no old short control or hidden-read routing')
print('PARTIAL knownModuleChain'+str(results[0]['known_module_chain_bytes'])+'/unresolved'+str(len(results[0]['unresolved']))+
      ';wholeKernelStackQualified=FALSE;graph object BYTE-IDENTICAL;NO native/physical/model numerics/resident Work/steady-TPS grant')
