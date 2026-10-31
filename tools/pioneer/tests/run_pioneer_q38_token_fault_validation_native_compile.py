#!/usr/bin/env python3
"""Actual fault-ROM revision of bounded tag75. Compile/inspect,NEVER install."""
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
    sys.argv=[str(here/'run_pioneer_q38_token_hidden_validation_native_compile.py'),str(qualified),str(out/'unchanged-tag75-controls')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
assert hashlib.sha256(control['obj'].read_bytes()).hexdigest()=='b3a6d7863ee7473e42cec4dcf13edb4faf60a6b326bf9a82747f39e26486bf05'
args=control['args'][:]
args+=['-DPIONEER_Q38_TOKEN_FAULT_PROGRAMS=1','-DPIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE=1']
obj=out/'token-fault-validation-tag75-INSPECT-ONLY.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_token_fault_validation_native_compile.cpp')
args[args.index('-o')+1]=str(obj)
assert '-c' in args and '-shared' not in args
subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
tool=control['tool']
table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
symbols={r[-1]:(int(r[0],16),int(r[1],16),r[2]) for r in map(str.split,table.splitlines()) if len(r)==4}
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={r[-1]:r[-3] for r in map(str.split,sections.splitlines()) if len(r)>=6 and r[-1] in symbols}
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
assert normal<=symbols.keys() and not any('pioneer_token_context_' in k for k in symbols)
for name in normal-{'api_version'}:assert symbols[name][1]>8 and symbols[name][2]=='T',name
required=('Q38TokenNativeControl','Q38TokenHiddenValidationControl','ReadQ38TokenNativeHiddenChunk',
          'InspectTerminal','ObserveQ38TokenNativeTerminal','BeginQ38TokenNativeBackendScoped','FinishQ38TokenNativeBackendScoped')
for name in required:assert any(name in k and v[1]>32 for k,v in symbols.items()),name
route='_ZL12ProbeControlPvjS_m';start,size,_=symbols[route]
relocs=subprocess.check_output([tool+'objdump','-r','-j',section[route],str(obj)],text=True)
targets={m[2] for line in relocs.splitlines() for m in [re.match(r'^([0-9a-f]+)\s+R_RISCV_CALL_PLT\s+(\S+)',line)]
         if m and start<=int(m[1],16)<start+size}
for name in ('Q38TokenNativeControl','Q38TokenHiddenValidationControl'):
    assert any(name in target for target in targets),(name,'real public route required')
header=(here.parent/'PioneerQ38TokenFaultPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='4e7a5fa71d546fb859c26376db50159bf6126c9e1185f48a36f7bad51eb11cdd'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(expected)==13
for digest,label in expected:
    matches=[k for k in symbols if 'PioneerQ38TokenFaultPrograms' in k and k.endswith(str(len(label))+label+'E')]
    assert len(matches)==1 and hashlib.sha256(control['data'](matches[0],obj,symbols,section,'fault-'+label)).hexdigest()==digest,label
assert not any('PioneerQ38TokenPushedPrograms' in k or 'PioneerQ38TokenProgramsL' in k for k in symbols)
rope=[k for k,(_,n,_) in symbols.items() if 'PioneerQ38TokenRope' in k and k.endswith('DataE') and n==2097152]
assert len(rope)==1
assert control['data'](rope[0],obj,symbols,section,'fault-rope')==control['data'](rope[0],control['obj'],control['symbols'],control['section'],'control-rope')
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','TokenControlTestCopy','pioneer_token_context_'):
    assert forbidden not in table and forbidden not in undefined,forbidden
frames=control['control']['control']['control']['control']['base']['frames'](obj.with_suffix('.su'))
assert frames
for name,n in frames.items():assert n<=2048 or name in control['frames'] and n<=control['frames'][name],(name,n)
for name in ('Q38TokenNativeControl','InspectTerminal'):
    assert any(name in k for k in frames),name
# GCC names the real observer's .su row only constprop(...). Bind it to
# the exact source definition AND the actual control->observer relocation.
source=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeExecution.inc'
lines=[i for i,line in enumerate(source.read_text().splitlines(),1) if 'bool ObserveQ38TokenNativeTerminal(' in line]
assert len(lines)==1
rows=[]
for line in obj.with_suffix('.su').read_text().splitlines():
    parts=line.split('\t');m=re.fullmatch(r'(.*?):([0-9]+):[0-9]+:(.*)',parts[0])
    if m and pathlib.Path(m[1]).resolve()==source and int(m[2])==lines[0]:rows.append(parts)
assert len(rows)==1 and rows[0][2]=='static' and 0<int(rows[0][1])<=2048,rows
handler=[k for k in symbols if 'Q38TokenNativeControl' in k]
observer=[k for k in symbols if 'ObserveQ38TokenNativeTerminal' in k]
assert len(handler)==len(observer)==1
begin,length,_=symbols[handler[0]]
calls=subprocess.check_output([tool+'objdump','-r','-j',section[handler[0]],str(obj)],text=True)
assert any(begin<=int(m[1],16)<begin+length and m[2]==observer[0]
           for line in calls.splitlines() for m in [re.match(r'^([0-9a-f]+)\s+R_RISCV_CALL_PLT\s+(\S+)',line)] if m)
print('FAULT_OBSERVER_FRAME '+rows[0][1]+' actual source-bound observer clone+control relocation')
print('PASS actual separate FAULT-ROM tag75 revision/conventional ABI/both bounded public routes/actual observer+failure-detail copy/ALL13 fault+unchanged-worker arrays SHA-exact/2MiB RoPE unchanged/old tag75+74+G3 byte-exact/no CPU oracle+Work+TEST imports/individual frames bounded')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' token-fault-validation-tag75-INSPECT-ONLY.o')
print('COMPILE+INSPECT ONLY;numeric tag75 is NOT byte identity;NO install/native runtime/whole-stack/model correctness/Work/TPS grant')
