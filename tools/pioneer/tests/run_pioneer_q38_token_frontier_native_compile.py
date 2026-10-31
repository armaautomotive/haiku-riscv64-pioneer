#!/usr/bin/env python3
"""Actual distinct private counter reader: SDK compile/inspect, NO device IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
OBJECT_SHA='05d149c9ccabe3e6b6b0d1d6c7ed66fd76bad35644fdc9085e410edcda53353f'

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
old=previous/'job-boot-fold-token-context-NODRV.o';argv=(previous/'compile-argv.txt').read_bytes()
assert hashlib.sha256(old.read_bytes()).hexdigest()=='f4f8e84b1b3253ff6fd17122d3705934dd01c75ffd44ce2504f84a3158f0fd0d'
assert hashlib.sha256(argv).hexdigest() in ('5a8974ffbcae97e844b7b42733e1e23daaf6e513ba13e82a3ffa3ba8e9bcafd9',
                                         'e0e48e8532930ce0ba948b47137d726b1af41bd4864151f9ae70d2ced9ad96ab')
args=argv.decode().splitlines()
assert args[0]=='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert args.count('-c')==args.count('-o')==1 and '-shared' not in args
assert pathlib.Path(args[args.index('-c')+1]).name=='pioneer_q38_token_job_boot_fold_native_context_compile.cpp'
assert pathlib.Path(args[args.index('-o')+1]).name==old.name
out.mkdir(mode=0o700);tool=args[0][:-3]
control=out/'unchanged-job-fold-token-context-NODRV.o';old_args=args[:]
old_args[old_args.index('-c')+1]=str(here/'pioneer_q38_token_job_boot_fold_native_context_compile.cpp')
old_args[old_args.index('-o')+1]=str(control);subprocess.run(old_args,check=True)
assert control.read_bytes()==old.read_bytes(),'old complete private object changed'
obj=out/'frontier-token-context-NODRV.o'
args[args.index('-c')+1]=str(here/'pioneer_q38_token_frontier_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
def records(path):
    raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True)
    values={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
    table=subprocess.check_output([tool+'objdump','-t',str(path)],text=True)
    section={row[-1]:row[-3] for row in map(str.split,table.splitlines()) if len(row)>=6 and row[-1] in values}
    return raw,values,section
table,symbols,sections=records(obj);_,old_symbols,old_sections=records(control)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
assert not {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}&symbols.keys()
def payload(path,name,values,section,label):
    start,size=values[name];raw=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',section[name],str(path),str(raw)],check=True)
    data=raw.read_bytes();assert start+size<=len(data);return data[start:start+size]
header=(here.parent/'PioneerQ38TokenFrontierPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='2bd70b273e8c6b2196c0d087ca03d87f34a5567cb1cb361b305483ce4b17c7e5'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(arrays)==13
for digest,label in arrays:
    names=[name for name in symbols if 'PioneerQ38TokenFrontierPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==1 and hashlib.sha256(payload(obj,names[0],symbols,sections,label)).hexdigest()==digest,label
for namespace in ('PioneerQ38TokenJobBootFoldPrograms','PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert not any(namespace in name for name in symbols),namespace
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert payload(obj,rope[0],symbols,sections,'new-rope')==payload(control,rope[0],old_symbols,old_sections,'old-rope')
demangled=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
for required in ('Q38TokenNativeControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
                 'Q38TokenFrontierControl(', 'pioneer_token_frontier_context_control',
                 'pioneer_token_frontier_context_scoped_observe','PioneerQ38TokenNativeOwner::InspectTerminal<'):
    assert required in demangled,required
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO'):
    assert forbidden not in table and forbidden not in undefined,forbidden
def frames(path):
    result={}
    for line in path.read_text().splitlines():
        parts=line.split('\t');name=re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)',parts[0])
        assert len(parts)==3 and name and parts[2]=='static',line
        result[name[1]]=int(parts[1])
    assert result;return result
before,after=frames(control.with_suffix('.su')),frames(obj.with_suffix('.su'))
for name,size in after.items():assert size<=2048 or name in before and size<=before[name],(name,size)
assert hashlib.sha256(obj.read_bytes()).hexdigest()==OBJECT_SHA,'unqualified private observer/adapter object'
print('PASS_ACTUAL_FRONTIER_PRIVATE_NATIVE_SDK exact13CompiledArrays1 oldFullPrivateObjectUnchanged1 rope2MiBUnchanged1 '
      'realSameScopeCounterReader1 conventionalDriverABIAbsent1 noCPUOracleWorkOrTestImports1 individualFramesBounded1')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' frontier-token-context-NODRV.o')
print('COMPILE_INSPECT_ONLY;private typed copy/lock adapter is NOT a routed physical endpoint/runtime/full kernel stack/new model/physical/timing/Work/TPS qualification;DO_NOT_INSTALL')
