#!/usr/bin/env python3
"""Actual optimized private SDK owner;compile/inspect ONLY,never install."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
old=previous/'precise-beta-token-context-NODRV.o';argv_bytes=(previous/'compile-argv.txt').read_bytes()
assert hashlib.sha256(old.read_bytes()).hexdigest()=='4b2098cf7fe33edf0450d8e50cec4b8b82efb992c74b38175d773b4cdf2a2ae8'
assert hashlib.sha256(argv_bytes).hexdigest() in (
    'b7cdff3d8053f98e9d0b0de939dfe15e46a495ef485e78526cc44268e0be6bc3',
    'c42b527fc6f9f68be3eb8ed4fa5578391f5167a3725c1c0199ba352dfc5dee40')
args=argv_bytes.decode().splitlines()
assert args[0]=='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert args.count('-c')==args.count('-o')==1 and '-shared' not in args
assert pathlib.Path(args[args.index('-c')+1]).name=='pioneer_q38_token_precise_beta_native_context_compile.cpp'
assert pathlib.Path(args[args.index('-o')+1]).name==old.name
# Preserve the authenticated full SDK argv;replace ONLY source and output.
# Standard/generated Haiku headers stay shared read-only SDK inputs. All
# actual implementation-relative includes come from this separate checkout.
out.mkdir(mode=0o700);obj=out/'job-boot-fold-token-context-NODRV.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_token_job_boot_fold_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n');tool=args[0][:-3]
def records(path):
    table=subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True)
    values={p[-1]:(int(p[0],16),int(p[1],16),p[2]) for p in map(str.split,table.splitlines()) if len(p)==4}
    sections=subprocess.check_output([tool+'objdump','-t',str(path)],text=True)
    section={p[-1]:p[-3] for p in map(str.split,sections.splitlines()) if len(p)>=6 and p[-1] in values}
    return table,values,section
table,symbols,sections=records(obj);_,old_symbols,old_sections=records(old)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
assert not {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}&symbols.keys()
def payload(path,name,values,section,label):
    start,size,_=values[name];raw=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section[name],str(path),str(raw)],check=True)
    data=raw.read_bytes();assert start+size<=len(data);return data[start:start+size]
header=(here.parent/'PioneerQ38TokenJobBootFoldPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='1b8e610966e7a8b69d545db7dd9f58f326b4d3937a354960930c858e8e92472b'
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(expected)==13
for digest,label in expected:
    names=[name for name in symbols if 'PioneerQ38TokenJobBootFoldPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==1 and hashlib.sha256(payload(obj,names[0],symbols,sections,label)).hexdigest()==digest,label
for namespace in ('PioneerQ38TokenPreciseBetaPrograms','PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert not any(namespace in name for name in symbols),namespace
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert payload(obj,rope[0],symbols,sections,'new-rope')==payload(old,rope[0],old_symbols,old_sections,'old-rope')
demangled=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
for required in ('Q38TokenNativeControl(', 'PioneerQ38TokenNativeOwner::InspectTerminal<', 'ObserveQ38TokenNativeTerminal(',
                 'pioneer_token_context_scoped_hidden_read','pioneer_token_context_scoped_terminal_observe'):
    assert required in demangled,required
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO'):
    assert forbidden not in table and forbidden not in undefined,forbidden
def frames(path):
    result={}
    for line in path.read_text().splitlines():
        p=line.split('\t');match=re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)',p[0])
        assert len(p)==3 and match and p[2]=='static',line
        result[match[1]]=int(p[1])
    assert result;return result
old_frames,new_frames=frames(old.with_suffix('.su')),frames(obj.with_suffix('.su'))
for name,size in new_frames.items():assert size<=2048 or name in old_frames and size<=old_frames[name],(name,size)
print('PASS_ACTUAL_JOB_BOOT_FOLD_PRIVATE_NATIVE_SDK all13CompiledArraysSHAExact1 oldCodeNamespacesAbsent1 rope2MiBUnchanged1 realOwnerAndController1 individualFramesBounded1 conventionalDriverABIAbsent1 noCPUOracleWorkOrTestImports1')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' job-boot-fold-token-context-NODRV.o')
print('COMPILE_INSPECT_ONLY_NOT_RUNTIME_WHOLE_KERNEL_STACK_PHYSICAL_MODEL_WORK_OR_TPS;DO_NOT_INSTALL')
