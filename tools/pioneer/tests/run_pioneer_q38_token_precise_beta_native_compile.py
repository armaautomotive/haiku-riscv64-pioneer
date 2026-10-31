#!/usr/bin/env python3
"""Actual private SDK owner with new code DATA. COMPILE/INSPECT ONLY."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent;repo=here.parents[2]
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
old_object=previous/'fault-token-context-NODRV.o';argv_data=(previous/'compile-argv.txt').read_bytes()
assert hashlib.sha256(old_object.read_bytes()).hexdigest()=='0983c720820cc6ad8d54c522f16d6bc5c6bac2248981ead5b79a84afcb2ecd4c'
assert hashlib.sha256(argv_data).hexdigest()=='56c34862126e3f9139fe4d49ce89b75b8bf7dab70bb8845304beffce5440d158'
args=argv_data.decode().splitlines()
assert args[0]=='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert '-c' in args and '-shared' not in args and args.count('-o')==args.count('-c')==1
assert pathlib.Path(args[args.index('-c')+1]).name=='pioneer_q38_token_fault_native_context_compile.cpp'
assert pathlib.Path(args[args.index('-o')+1]).name==old_object.name
# The old command is SHA-bound before replacing only source/output.
# No shell expansion, flag overrides or unchecked compiler command is accepted.
# Standard/generated Haiku SDK headers remain the SAME read-only SDK inputs
# in both builds. Only relative project implementation includes come from
# this checkout; do not map shared SDK headers onto an incomplete archive.
out.mkdir(mode=0o700);obj=out/'precise-beta-token-context-NODRV.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_token_precise_beta_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
tool=args[0][:-3] # g++ -> binutils prefix
assert tool.endswith('-')
def records(path):
    table=subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True)
    values={r[-1]:(int(r[0],16),int(r[1],16),r[2]) for r in map(str.split,table.splitlines()) if len(r)==4}
    sections=subprocess.check_output([tool+'objdump','-t',str(path)],text=True)
    section={r[-1]:r[-3] for r in map(str.split,sections.splitlines()) if len(r)>=6 and r[-1] in values}
    return table,values,section
table,symbols,section=records(obj);old_table,old_symbols,old_section=records(old_object)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
assert not normal&symbols.keys()
def data(path,name,values,sections,label):
    start,size,_=values[name];raw=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',sections[name],str(path),str(raw)],check=True)
    payload=raw.read_bytes();assert start+size<=len(payload);return payload[start:start+size]
header=(here.parent/'PioneerQ38TokenPreciseBetaPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='0e7e21ee62832062fd809ac7af8b9d5da00bb479778f559bc808e4dcede156bd'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(expected)==13
for digest,label in expected:
    found=[name for name in symbols if 'PioneerQ38TokenPreciseBetaPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(found)==1 and hashlib.sha256(data(obj,found[0],symbols,section,label)).hexdigest()==digest,label
assert not any('PioneerQ38TokenFaultPrograms' in name or 'PioneerQ38TokenPushedPrograms' in name for name in symbols)
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert data(obj,rope[0],symbols,section,'new-rope')==data(old_object,rope[0],old_symbols,old_section,'old-rope')
demangled=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
for required in ('Q38TokenNativeControl(', 'PioneerQ38TokenNativeOwner::InspectTerminal<', 'ObserveQ38TokenNativeTerminal(',
                 'pioneer_token_context_scoped_hidden_read', 'pioneer_token_context_scoped_terminal_observe'):
    assert required in demangled,required
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO'):
    assert forbidden not in table and forbidden not in undefined,forbidden
def frames(path):
    result={}
    for line in path.read_text().splitlines():
        parts=line.split('\t');match=re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)',parts[0])
        assert len(parts)==3 and match and parts[2]=='static',line
        result[match[1]]=int(parts[1])
    assert result;return result
old_frames=frames(old_object.with_suffix('.su'));new_frames=frames(obj.with_suffix('.su'))
for name,size in new_frames.items():assert size<=2048 or name in old_frames and size<=old_frames[name],(name,size)
print('PASS_ACTUAL_PRECISE_BETA_PRIVATE_NATIVE_SDK all13CompiledArraysSHAExact1 oldCodeNamespacesAbsent1 rope2MiBUnchanged1 realOwnerAndController1 individualFramesBounded1 conventionalDriverABIAbsent1 noCPUOracleWorkOrTestImports1')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' precise-beta-token-context-NODRV.o')
print('COMPILE_INSPECT_ONLY_NOT_NATIVE_RUNTIME_WHOLE_KERNEL_STACK_PHYSICAL_MODEL_WORK_OR_TPS;DO_NOT_INSTALL')
