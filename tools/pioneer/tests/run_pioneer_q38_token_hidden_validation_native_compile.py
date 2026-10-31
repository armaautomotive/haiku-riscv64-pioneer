#!/usr/bin/env python3
"""Actual separate tag75 driver object,compile/inspect ONLY. Never install."""
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
project=here.parents[2]
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_token_validation_native_compile.py'),str(qualified),str(out/'tag74-controls')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
# Never silently update the independent old public or default-private pins.
assert hashlib.sha256(control['obj'].read_bytes()).hexdigest()=='772aafe8c1b47ddfd85f5608a96fa329562967359e23723d5277a30842f9432a'
args=[a for a in control['args'] if not a.startswith('-DPIONEER_Q38_TOKEN_VALIDATION_NATIVE=')]
assert '-DPIONEER_Q38_TOKEN_VALIDATION_NATIVE' not in args
args.append('-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE=1')
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags)==1 and args[tags[0]]=='-DPIONEER_DRIVER_BUILD_TAG=0x2026103000010074ULL'
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026103100010075ULL'
obj=out/'token-hidden-validation-tag75-INSPECT-ONLY.o'
args[args.index('-o')+1]=str(obj)
assert '-c' in args and '-shared' not in args
subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
tool=control['tool']
table=subprocess.check_output([tool+'nm','-S','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16),row[2])
         for row in map(str.split,table.splitlines()) if len(row)==4}
sections=subprocess.check_output([tool+'objdump','-t',str(obj)],text=True)
section={row[-1]:row[-3] for row in map(str.split,sections.splitlines()) if len(row)>=6 and row[-1] in symbols}
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
assert normal<=symbols.keys() and not any('pioneer_token_context_' in name for name in symbols)
for name in normal-{'api_version'}:assert symbols[name][1]>8 and symbols[name][2]=='T',name
required=('Q38TokenNativeControl','Q38TokenHiddenValidationControl','ReadQ38TokenNativeHiddenChunk',
          'ReadHiddenChunk','ReadHiddenPacket','HiddenReadOutput','BeginQ38TokenNativeBackendScoped','FinishQ38TokenNativeBackendScoped')
for name in required:assert any(name in key and value[1]>32 for key,value in symbols.items()),name
route='_ZL12ProbeControlPvjS_m';assert route in symbols
start,size,_=symbols[route]
relocations=subprocess.check_output([tool+'objdump','-r','-j',section[route],str(obj)],text=True)
targets={m[2] for line in relocations.splitlines() for m in [re.match(r'^([0-9a-f]+)\s+R_RISCV_CALL_PLT\s+(\S+)',line)]
         if m and start<=int(m[1],16)<start+size}
for name in ('Q38TokenNativeControl','Q38TokenHiddenValidationControl'):
    assert any(name in target for target in targets),(name,'actual reachable route required')
def data(name,object_path,infos,section_map,label):
    begin,bytes_,_=infos[name];raw=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section_map[name],str(object_path),str(raw)],check=True)
    contents=raw.read_bytes();assert begin+bytes_<=len(contents)
    return contents[begin:begin+bytes_]
header=(here.parent/'PioneerQ38TokenPushedPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(expected)==13
for digest,label in expected:
    matches=[name for name in symbols if 'PioneerQ38TokenPushedPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(matches)==1 and hashlib.sha256(data(matches[0],obj,symbols,section,label)).hexdigest()==digest,label
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1
assert data(rope[0],obj,symbols,section,'new-rope')==data(rope[0],control['obj'],control['symbols'],control['section'],'old-rope')
for forbidden in ('Q38WorkControl','ReferenceProducts','CompareFinal','OriginalSource','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','Q38NativeStateTest','Q38InstructionStackObserver','TokenControlTestCopy'):
    assert forbidden not in table and forbidden not in undefined,forbidden
assert not re.search(r'\b(?:cosf?|sinf?|powf?)\b',undefined)
frames=control['control']['control']['control']['base']['frames'](obj.with_suffix('.su'))
assert frames
for name,size in frames.items():
    assert size<=2048 or name in control['frames'] and size<=control['frames'][name],(name,size)
for name in required:
    if name!='ReadQ38TokenNativeHiddenChunk':assert any(name in key for key in frames),name
# GCC emits the helper's actual symbol as a constprop clone and writes only
# "constprop(...)" in .su. Bind that frame to its EXACT source definition,
# AND require the actual handler->helper relocation,not a missing/zero frame.
helper_source=project/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeExecution.inc'
helper_lines=[i for i,line in enumerate(helper_source.read_text().splitlines(),1)
              if 'bool ReadQ38TokenNativeHiddenChunk(' in line]
assert len(helper_lines)==1
helper_rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()
             if line.startswith(str(helper_source)+':'+str(helper_lines[0])+':')]
assert len(helper_rows)==1 and len(helper_rows[0])==3 and helper_rows[0][2]=='static'
assert 0<int(helper_rows[0][1])<=2048
handler=[key for key in symbols if 'Q38TokenHiddenValidationControl' in key]
helper=[key for key in symbols if 'ReadQ38TokenNativeHiddenChunk' in key]
assert len(handler)==len(helper)==1
begin,bytes_,_=symbols[handler[0]]
helper_relocations=subprocess.check_output([tool+'objdump','-r','-j',section[handler[0]],str(obj)],text=True)
assert any(begin<=int(m[1],16)<begin+bytes_ and m[2]==helper[0]
           for line in helper_relocations.splitlines() for m in [re.match(r'^([0-9a-f]+)\s+R_RISCV_CALL_PLT\s+(\S+)',line)] if m)
print('HIDDEN_TAG75_FRAME '+helper_rows[0][1]+' ReadQ38TokenNativeHiddenChunk actual source-bound constprop clone')
for name,size in sorted(frames.items()):
    if any(part in name for part in ('HiddenValidationControl','ReadQ38TokenNativeHiddenChunk','ReadHiddenChunk','ReadHiddenPacket','HiddenReadOutput')):
        print('HIDDEN_TAG75_FRAME '+str(size)+' '+name)
print('PASS actual separate tag75 conventional driver object/2 bounded diagnostic selectors reachable/real held-hidden reader emitted/all13 G3 arrays+2MiB RoPE exact/no CPU oracle+Work+TEST imports/default G3 and tag74 byte-exact/individual frames bounded;NO combined host-stack/runtime/physical/numerical/Work/TPS grant')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' token-hidden-validation-tag75-INSPECT-ONLY.o;COMPILE+INSPECT ONLY,NEVER link/install/invoke')
