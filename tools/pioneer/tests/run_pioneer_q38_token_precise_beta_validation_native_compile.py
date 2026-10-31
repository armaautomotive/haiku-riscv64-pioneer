#!/usr/bin/env python3
"""Compile/inspect actual separate precise-beta diagnostic; NEVER install."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

job_boot_fold='--job-boot-fold' in sys.argv
if job_boot_fold:
    assert sys.argv.count('--job-boot-fold')==1
    sys.argv.remove('--job-boot-fold')
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
old_object=previous/'token-posted-cancel-INSPECT-ONLY.o';argv_data=(previous/'compile-argv.txt').read_bytes()
assert hashlib.sha256(old_object.read_bytes()).hexdigest()=='26d54996ebfde7fa9c7a97ee936fb8535affb409e689b008dba0a6dfbfa191f5'
assert hashlib.sha256(argv_data).hexdigest()=='a04e60b4c1bffe083cc504c6269dc85540409913e9d0632b2cc62aba2b69a2e0'
args=argv_data.decode().splitlines()
assert args[0]=='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert '-c' in args and '-shared' not in args and args.count('-o')==args.count('-c')==1
assert pathlib.Path(args[args.index('-c')+1]).name=='pioneer_q38_posted_cancel_native_compile.cpp'
assert pathlib.Path(args[args.index('-o')+1]).name==old_object.name
# The complete old command is SHA-bound. Replace source/output ONLY; use
# the SAME standard/generated read-only SDK headers in independent builds.
# Actual project implementation comes from the wrapper's relative includes.
out.mkdir(mode=0o700);obj=out/('token-job-boot-fold-INSPECT-ONLY.o' if job_boot_fold else 'token-precise-beta-INSPECT-ONLY.o')
args[args.index('-c')+1]=str(here/('pioneer_q38_token_job_boot_fold_validation_native_compile.cpp' if job_boot_fold else 'pioneer_q38_token_precise_beta_validation_native_compile.cpp'))
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
# Independently identical main/clean objects;compile identity only,not
# deployment or native lifetime authority.
assert hashlib.sha256(obj.read_bytes()).hexdigest()==('ee3e9ad9cef6e6e5b449071ad953ce7b18e73df7fd69c935a9e759625288d115' if job_boot_fold else 'dca90a0f1e83597a350621ae76c9d02339234861e52ed48a4b29c2e7565d5edf')
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
tool=args[0][:-3];assert tool.endswith('-')
def records(path):
    table=subprocess.check_output([tool+'nm','-S','--defined-only',str(path)],text=True)
    values={r[-1]:(int(r[0],16),int(r[1],16),r[2]) for r in map(str.split,table.splitlines()) if len(r)==4}
    sections=subprocess.check_output([tool+'objdump','-t',str(path)],text=True)
    section={r[-1]:r[-3] for r in map(str.split,sections.splitlines()) if len(r)>=6 and r[-1] in values}
    return table,values,section
table,symbols,section=records(obj);old_table,old_symbols,old_section=records(old_object)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=symbols.keys()
def data(path,name,values,sections,label):
    start,size,_=values[name];raw=out/(label+'.raw')
    subprocess.run([tool+'objcopy','-O','binary','-j',sections[name],str(path),str(raw)],check=True)
    payload=raw.read_bytes();assert start+size<=len(payload);return payload[start:start+size]
namespace='PioneerQ38TokenJobBootFoldPrograms' if job_boot_fold else 'PioneerQ38TokenPreciseBetaPrograms'
header=(here.parent/(namespace+'.h')).read_bytes()
assert hashlib.sha256(header).hexdigest()==('1b8e610966e7a8b69d545db7dd9f58f326b4d3937a354960930c858e8e92472b' if job_boot_fold else '0e7e21ee62832062fd809ac7af8b9d5da00bb479778f559bc808e4dcede156bd')
expected=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(expected)==13
for digest,label in expected:
    found=[name for name in symbols if namespace in name and name.endswith(str(len(label))+label+'E')]
    assert len(found)==1 and hashlib.sha256(data(obj,found[0],symbols,section,label)).hexdigest()==digest,label
assert not any('PioneerQ38TokenFaultPrograms' in name or 'PioneerQ38TokenPushedPrograms' in name for name in symbols)
if job_boot_fold:
    assert not any('PioneerQ38TokenPreciseBetaPrograms' in name for name in symbols)
rope=[name for name,(_,size,_) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old_symbols
assert data(obj,rope[0],symbols,section,'new-rope')==data(old_object,rope[0],old_symbols,old_section,'old-rope')
demangled=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
for required in ('Q38TokenNativeControl(', 'Q38TokenHiddenValidationControl(', 'PostedCancelSourceGuard(',
                 'PioneerQ38PostedCancel::Request<', 'BeginQ38TokenNativeBackendScoped(', 'FinishQ38TokenNativeBackendScoped('):
    assert required in demangled,required
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','PostedCancelEndpoint','pioneer_token_context_'):
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
print(('PASS_ACTUAL_JOB_BOOT_FOLD_PHYSICAL_SDK' if job_boot_fold else 'PASS_ACTUAL_PRECISE_BETA_PHYSICAL_SDK')+' all13CompiledArraysSHAExact1 oldCodeNamespacesAbsent1 rope2MiBUnchanged1 realOwnerAndControllerAndPostedCancel1 individualFramesBounded1 conventionalDriverABI1 noCPUOracleWorkOrTestImports1')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' '+obj.name)
print('COMPILE_INSPECT_ONLY_NOT_INSTALL_NATIVE_RUNTIME_WHOLE_KERNEL_STACK_PHYSICAL_MODEL_WORK_OR_TPS')
