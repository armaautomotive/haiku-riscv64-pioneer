#!/usr/bin/env python3
"""Exact new ROM DATA/native perimeter tests;no model or physical execution."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
inputs=[(root,helpers['pins'](here.parent/'build_qwen38_token_fault_marker_image.py','JOB_FOLD_PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_precise_beta_worker_image.py','PINS'))]
for directory,pins in inputs:helpers['authenticated'](directory,pins)
header=here.parent/'PioneerQ38TokenJobBootFoldPrograms.h'
assert hashlib.sha256(header.read_bytes()).hexdigest()=='1b8e610966e7a8b69d545db7dd9f58f326b4d3937a354960930c858e8e92472b'
data_roles=('PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS',
            'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS')
selected=dict.fromkeys(data_roles,'1')
include='#include "PioneerQ38TokenSelectedPrograms.h"\n'
abi=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
native_role=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h'
parents=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',native_role.read_text()))
assert len(parents)==21
native={**dict.fromkeys(parents,'1'),**selected,'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT':'1',
        'PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT':'1','PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC':'1',
        'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL',**{name:'test_fold_'+name for name in abi}}
compiler=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
refused=0
def source(values,tail=''):
    return ''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+include+tail
def check(text,okay):
    result=subprocess.run([*compiler,'-fsyntax-only','-x','c++','-'],input=text,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(text,result.stderr)
    if not okay:assert 'error:' in result.stderr
with tempfile.TemporaryDirectory(prefix='pioneer-q38-fold-program-data-',dir='/private/tmp') as name:
    out=pathlib.Path(name);generated=out/'programs.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(generated),'--job-boot-fold'],check=True)
    assert generated.read_bytes()==header.read_bytes()
    binary=out/'data-test'
    subprocess.run([*compiler,'-O1','-g','-fsanitize=address,undefined',
                    *['-D'+k+'='+v for k,v in selected.items()],'-DPIONEER_Q38_PROGRAM_DATA_TEST_JOB_BOOT_FOLD=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(generated)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    # Prove all nine worker arrays stay IDENTICAL to precise-beta DATA.
    def arrays(path):
        return dict((label,digest) for digest,label in re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',path.read_text()))
    new,old=arrays(header),arrays(here.parent/'PioneerQ38TokenPreciseBetaPrograms.h')
    assert len(new)==len(old)==13
    assert all(new[key]==old[key] for key in new if key.startswith('Peer'))
    assert new['RootText']!=old['RootText'] and new['RootAux']!=old['RootAux']
    corrupt=0
    for directory,pins in inputs:
        for artifact,_,_ in pins:
            case=out/f'bad-{corrupt}';case.mkdir()
            for path in directory.iterdir():
                if path.is_file():(case/path.name).symlink_to(path)
            target=case/artifact;target.unlink() # owned private symlink,not its source
            original=(directory/artifact).read_bytes();changed=bytearray(original);changed[len(changed)//2]^=1
            for payload in (changed,original[:-1]):
                target.write_bytes(payload);destination=out/f'NO_OUTPUT-{corrupt}'
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(destination),'--job-boot-fold'],capture_output=True)
                assert result.returncode==2 and artifact.encode() in result.stderr and not destination.exists();corrupt+=1
    assert corrupt==30
    check(source(selected,include+'static_assert(PioneerQ38TokenPrograms::JobBootFold==1);\n'),True)
    check(source(native,'static_assert(PioneerQ38TokenJobBootFoldSelection==2);\n'),True)
    for values,required in ((selected,set(data_roles)),(native,parents|set(data_roles)|{
            'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC'})):
        for key in required:
            for value in (None,'0','2'):
                bad=values.copy()
                if value is None:del bad[key]
                else:bad[key]=value
                # Omitting the new selector may legitimately select the OLD
                # beta package. That default must remain legal,but MUST NOT
                # satisfy this explicit new-code requirement.
                check(source(bad,'static_assert(PioneerQ38TokenPrograms::JobBootFold==1,"new code required");\n'),False);refused+=1
            check(source(values,'#undef '+key+'\n#define '+key+' 0\n'+include),False);refused+=1
    for key in abi:
        bad=native.copy();del bad[key];check(source(bad),False);refused+=1
    for tag in ('0','0x2026103100010075ULL','0x5154324e4f445256ULL'):
        check(source({**native,'PIONEER_DRIVER_BUILD_TAG':tag}),False);refused+=1
    for key in ('PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
                'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_VALIDATION_NATIVE',
                'PIONEER_Q38_TOKEN_VALIDATION_CLIENT','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT',
                'PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT','PIONEER_Q38_CPU_VALIDATION',
                'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                'PIONEER_Q38_JOB_BOOT_FOLD_IMAGE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
                'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION',
                'PIONEER_Q38_GLOBAL_WORKER_IMAGE','__FAST_MATH__'):
        for value in ('0','1'):
            check(source({**selected,key:value}),False);check(source({**native,key:value}),False);refused+=2
    for key in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT'):
        for value in ('0','1'):check(source({**selected,key:value}),False);refused+=1
    for old_header in ('PioneerQ38TokenPreciseBetaPrograms.h','PioneerQ38TokenFaultPrograms.h','PioneerQ38TokenPushedPrograms.h'):
        old_source=''.join('#define '+k+' 1\n' for k in data_roles if k!='PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS')
        old_source+='#include "'+old_header+'"\n#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS 1\n'+include
        check(old_source,False);refused+=1
    check(include+source(selected),False);refused+=1
    check(source(selected,'#undef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS\n'+include),False);refused+=1
    check(source(selected)+source(native),False);refused+=1
    print(f'PASS_JOB_BOOT_FOLD_PROGRAM_DATA all13Arrays3044253Words1 peersAllNineUnchanged1 {corrupt} corruptOrTruncatedBeforeOutputRefusals {refused} missing+zero+two+late+foreign+oldOwnerAndABIRenameRefusals;HOST DATA/private syntax ONLY,NO native/card/model/Work/TPS grant')
