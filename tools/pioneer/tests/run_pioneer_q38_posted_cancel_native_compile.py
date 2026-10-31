#!/usr/bin/env python3
"""Rebuild old literal controls then inspect actual posted-cancel adapter; NO install."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
qualified,out=map(pathlib.Path,sys.argv[1:])
assert qualified.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_token_fault_validation_native_compile.py'),str(qualified),str(out/'unchanged-fault-controls')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
assert hashlib.sha256(control['obj'].read_bytes()).hexdigest()=='98555df8f038598c79bd69cca8e911ceb111a6ee6787faa041299cec10fffac0'
args=control['args'][:]+['-DPIONEER_Q38_TOKEN_POSTED_CANCEL=1']
obj=out/'token-posted-cancel-INSPECT-ONLY.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_posted_cancel_native_compile.cpp')
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
tool=control['tool']
table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
symbols={r[-1]:(int(r[0],16),int(r[1],16),r[2]) for r in map(str.split,table.splitlines()) if len(r)==4}
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={r[-1]:r[-3] for r in map(str.split,sections.splitlines()) if len(r)>=6 and r[-1] in symbols}
for name in ('PostedCancelScope','PostedCancelSourceGuard','PostedCancelControlWrite','PostedCancelWordWrite',
             'Q38TokenNativeControl','CancelQ38TokenNativeRoot','BeginQ38TokenNativeBackendScoped','FinishQ38TokenNativeBackendScoped'):
    assert any(name in k and value[1]>16 for k,value in symbols.items()),name
assert any('PioneerQ38PostedCancel' in name and 'Request' in name for name in symbols)
header=(here.parent/'PioneerQ38TokenFaultPrograms.h').read_text()
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header)
assert len(expected)==13
for digest,label in expected:
    matches=[k for k in symbols if 'PioneerQ38TokenFaultPrograms' in k and k.endswith(str(len(label))+label+'E')]
    assert len(matches)==1
    raw=control['control']['data'](matches[0],obj,symbols,section,'posted-'+label)
    assert hashlib.sha256(raw).hexdigest()==digest,label
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1
assert control['control']['data'](rope[0],obj,symbols,section,'posted-rope') == \
    control['control']['data'](rope[0],control['obj'],control['symbols'],control['section'],'control-rope')
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','PostedCancelEndpoint','pioneer_token_context_'):
    assert forbidden not in table and forbidden not in undefined,forbidden
frames=control['control']['control']['control']['control']['control']['base']['frames'](obj.with_suffix('.su'))
assert frames
for name,bytes_ in frames.items():
    assert bytes_<=2048 or name in control['frames'] and bytes_<=control['frames'][name],(name,bytes_)
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' '+obj.name)
print('PASS actual conventional G3 fault-ROM posted-cancel adapter/controller compiled/all old literal controls unchanged/all13 ROM arrays+2MiB RoPE exact/no CPU oracle+Work+TEST imports/individual frames bounded')
print('COMPILE+INSPECT ONLY;no installation,physical delivery,full native lifecycle,whole-stack,model numerics,Work or TPS grant')
