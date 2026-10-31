#!/usr/bin/env python3
"""Inspect actual private fault-ROM native body; NEVER link/install/invoke."""
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
    sys.argv=[str(here/'run_pioneer_q38_token_hidden_read_native_compile.py'),str(qualified),str(out/'unchanged-controls')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
args=control['args'][:]
obj=out/'fault-token-context-NODRV.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_token_fault_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
frames=control['control']['control']['control']['base']['frames'](obj.with_suffix('.su'))
assert frames
for name,size in frames.items():
    assert size<=2048 or name in control['frames'] and size<=control['frames'][name],(name,size)
tool=control['tool']
table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16),row[2])
         for row in map(str.split,table.splitlines()) if len(row)==4}
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
assert not normal & symbols.keys()
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={row[-1]:row[-3] for row in map(str.split,sections.splitlines()) if len(row)>=6 and row[-1] in symbols}
def data(name):
    start,size,_=symbols[name]
    raw=out/(name+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section[name],str(obj),str(raw)],check=True)
    value=raw.read_bytes();assert start+size<=len(value)
    return value[start:start+size]
header=(here.parent/'PioneerQ38TokenFaultPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='4e7a5fa71d546fb859c26376db50159bf6126c9e1185f48a36f7bad51eb11cdd'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(expected)==13
for digest,label in expected:
    matches=[name for name in symbols if 'PioneerQ38TokenFaultPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(matches)==1 and hashlib.sha256(data(matches[0])).hexdigest()==digest,label
assert not any('PioneerQ38TokenPushedPrograms' in name or 'PioneerQ38TokenProgramsL' in name for name in symbols)
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1
start,size,_=control['symbols'][rope[0]]
raw=out/'unchanged-rope.raw'
subprocess.run([tool+'objcopy','-O','binary','-j',control['section'][rope[0]],str(control['obj']),str(raw)],check=True)
assert data(rope[0])==raw.read_bytes()[start:start+size]
for required in ('PioneerQ38TokenNativeOwner::InspectTerminal(', 'ObserveQ38TokenNativeTerminal(',
                 'Q38TokenNativeControl(', 'pioneer_token_context_scoped_terminal_observe(',
                 'pioneer_token_context_scoped_hidden_read('):
    assert any(required in name and size>0 for name,size in frames.items()),required
for forbidden in ('ReferenceProducts','CompareFinal','OriginalSource','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','ValidationSession','Q38WorkControl'):
    assert forbidden not in table and forbidden not in undefined,forbidden
def words(name):
    start,size,kind=symbols[name];assert size and kind in ('t','T')
    dis=subprocess.check_output([tool+'objdump','-d','-j',section[name],str(obj)],text=True)
    selected=[(int(m[1],16),m[2]) for line in dis.splitlines()
              for m in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+',line)] if m and start<=int(m[1],16)<start+size]
    address=start
    for at,value in selected:
        assert at==address;address+=len(value)//2
    assert address==start+size
    reloc=subprocess.check_output([tool+'objdump','-r','-j',section[name],str(obj)],text=True)
    assert not any(start<=int(m[1],16)<start+size for line in reloc.splitlines()
                   for m in [re.match(r'^([0-9a-f]+)\s+R_',line)] if m)
    return [value for _,value in selected]
for name,kind in (('init_hardware','refusal'),('init_driver','refusal'),('uninit_driver','inert'),
                  ('publish_devices','null'),('find_device','null')):
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        assert words(prefix+name)==words('pioneer_token_context_expected_'+kind)
print('PASS ACTUAL private fault-ROM native integration/all13 compiled arrays SHA-exact/old program arrays absent/2MiB RoPE unchanged/real observer+controller+FD scope emitted/64B result+last2 offsets checked/all10 conventional entries inert/no CPU-reference+Work+TEST imports')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' fault-token-context-NODRV.o')
print('COMPILE+INSPECT ONLY;NO native runtime,physical module,user-detail logging,model correctness,clean END,Work or TPS grant')
