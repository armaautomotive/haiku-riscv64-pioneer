#!/usr/bin/env python3
"""Compile/inspect private held-hidden controller and unchanged controls ONLY."""
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
    sys.argv=[str(here/'run_pioneer_q38_token_pushed_native_context_compile.py'),str(qualified),str(out/'controls')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
# The new feature is absent in this control. DO NOT silently change its pin.
assert hashlib.sha256(control['obj'].read_bytes()).hexdigest()=='c78a84c7ec8f74c0f73f74e67579dfa94bcc71399145508fcb062619844c9b80'
args=control['args'][:]
obj=out/'hidden-read-context-NODRV.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_token_hidden_read_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
frames=control['control']['control']['base']['frames'](obj.with_suffix('.su'))
assert frames
for name,size in frames.items():
    assert size<=2048 or name in control['frames'] and size<=control['frames'][name],(name,size)
required=('pioneer_token_context_scoped_hidden_read','ReadQ38TokenNativeHiddenChunk',
          'ReadHiddenChunk','ReadHiddenPacket','HiddenReadOutput')
for name in required:
    assert any(name in actual for actual in frames),(name,'real body must be emitted')
tool=control['tool']
table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16),row[2])
         for row in map(str.split,table.splitlines()) if len(row)==4}
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
assert not normal & symbols.keys()
for name in required:
    assert any(name in actual and size>32 for actual,(_,size,_) in symbols.items()),name
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={row[-1]:row[-3] for row in map(str.split,sections.splitlines()) if len(row)>=6 and row[-1] in symbols}
header=(here.parent/'PioneerQ38TokenPushedPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(expected)==13
def data(name):
    start,size,_=symbols[name]
    raw=out/(name+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section[name],str(obj),str(raw)],check=True)
    contents=raw.read_bytes();assert start+size<=len(contents)
    return contents[start:start+size]
for digest,label in expected:
    matches=[name for name in symbols if 'PioneerQ38TokenPushedPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(matches)==1 and hashlib.sha256(data(matches[0])).hexdigest()==digest,label
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1
control_rope=[name for name,(_,size,_) in control['symbols'].items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert rope==control_rope
# Compare full RoPE bytes with the actual unchanged control section.
control_start,control_size,_=control['symbols'][rope[0]]
control_raw=out/'control-rope.raw'
subprocess.run([tool+'objcopy','-O','binary','-j',control['section'][rope[0]],str(control['obj']),str(control_raw)],check=True)
assert data(rope[0])==control_raw.read_bytes()[control_start:control_start+control_size]
for forbidden in ('OriginalSource','ReferenceProducts','ValidationSession','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','Q38WorkControl'):
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
    rel=subprocess.check_output([tool+'objdump','-r','-j',section[name],str(obj)],text=True)
    assert not any(start<=int(m[1],16)<start+size for line in rel.splitlines()
                   for m in [re.match(r'^([0-9a-f]+)\s+R_',line)] if m)
    return [value for _,value in selected]
for name,kind in (('init_hardware','refusal'),('init_driver','refusal'),('uninit_driver','inert'),
                  ('publish_devices','null'),('find_device','null')):
    assert words('pioneer_q38_token_context_body_'+name)==words('pioneer_token_context_expected_'+kind)
    assert words('pioneer_q38_token_context_only_'+name)==words('pioneer_token_context_expected_'+kind)
hidden_frames={name:size for name,size in frames.items() if any(part in name for part in required)}
print('PASS ACTUAL private QT3NODRV hidden controller/FD-team-thread scope/real native sources+owner/5 real emitted bodies/all13 firmware arrays+2MiBRoPE unchanged/renamed ABI inert/no CPU oracle+Work imports')
for name,size in sorted(hidden_frames.items()):print('HIDDEN_FRAME '+str(size)+' '+name)
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' hidden-read-context-NODRV.o')
print('COMPILE+INSPECT ONLY;NO link/install/public opcode/native runtime/physical/numerical/Work/TPS grant;combined host stack remains unqualified')
