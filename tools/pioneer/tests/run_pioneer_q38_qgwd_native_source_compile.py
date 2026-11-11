#!/usr/bin/env python3
"""Compile real QGWD native driver body with renamed ABI; never link/run."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

qgwf=len(sys.argv)==4 and sys.argv[1]=='--qgwf-native-inspection'
qgwe=qgwf or len(sys.argv)==4 and sys.argv[1]=='--qgwe-native-inspection'
two=len(sys.argv)==4 and sys.argv[1]=='--two-candidate'
candidate=two or len(sys.argv)==4 and sys.argv[1]=='--candidate'
if len(sys.argv)!=(4 if candidate or qgwe else 3) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: run_pioneer_q38_qgwd_native_source_compile.py [--candidate|--two-candidate] ABS_SDK ABS_PROGRAM_HEADER')
sdk,header=map(Path,sys.argv[2:] if candidate or qgwe else sys.argv[1:])
assert sdk.is_absolute() and sdk.is_dir() and header.is_absolute() and header.is_file()
assert hashlib.sha256(header.read_bytes()).hexdigest()==('b0c3d213676ded8ec484cee54908fc6443d4f2f50e199084a782230111fc1ac6' if qgwf else '4377811ebdc782bb67251beb120e32fdef18aab29ece5e8d77ba07bbcb0846b2' if qgwe else 'f4bc8d499b2c12d381fb24ba3423af1fed6792fa94e52882eb627697d721a687' if two else '84b7bda4b16924be3da350499a0376e2acb0ce1b47a73ee49fcc914829041c72' if candidate else '90dd22aada31d665a42fda9fa158afd888d13e04c45365203ae5f42c0fb6daff')
here=Path(__file__).resolve().parent;root=here.parents[2];pioneer=here.parent
role=(root/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(parents)==21
names=parents|set(('PIONEER_Q38_HYBRID_TRACKED_UPLOAD','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
 'PIONEER_Q38_TOKEN_BF16_PROGRAMS','PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC','PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
 'PIONEER_Q38_QGWD_NATIVE_INSPECTION','PIONEER_Q38_QGWD_PROGRAMS_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
 'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION',
 'PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE','PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION',
 'PIONEER_Q38_QGWD_CONTROL_INSPECTION','PIONEER_Q38_QGWD_SPAN_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION',
 'PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION','PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION','PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION'))
names.update('TT_PIONEER_EXPERIMENTAL_Q38_'+n for n in ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS',
 'COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR'))
if qgwe:names.update(('PIONEER_Q38_QGWE_NATIVE_INSPECTION','PIONEER_Q38_QGWE_PROGRAMS_INSPECTION','PIONEER_Q38_QGWE_BATCH_DENSE_PLAN'))
if qgwf:names.update(('PIONEER_Q38_QGWF_NATIVE_INSPECTION','PIONEER_Q38_QGWF_PROGRAMS_INSPECTION','PIONEER_Q38_QGWF_DUAL_BATCH_PLAN'))
if candidate:
    names-={'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'}
    names.update(('PIONEER_Q38_QGWD_NATIVE_141','PIONEER_Q38_HYBRID_FULL_NATIVE',
                  'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128'))
    if two:names.add('PIONEER_Q38_QGWD_TWO_NATIVE_142')
headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
release=sdk/'objects/haiku/riscv64/release'
includes=[]
for part in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/part)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(headers),
           '-idirafter',str(headers/'posix'),'-I'+str(pioneer)]
for path in [headers/'os',*sorted((headers/'os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
cxx=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
flags=['-D'+name+'=1' for name in sorted(names)]
flags+=['-DPIONEER_DRIVER_BUILD_TAG='+('0x2026111100010142ULL' if two else '0x2026111000010141ULL' if candidate else '0x51544b4e4f445256ULL'),'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-fno-exceptions','-fno-rtti','-fstack-usage','-ffreestanding','-finline','-fno-semantic-interposition',
 '-fno-tree-vectorize','-fno-use-cxa-atexit','-fno-strict-aliasing','-fno-delete-null-pointer-checks',
 '-mcmodel=medany','-fpic',*includes,*flags]
if candidate:
    common+=['-fno-inline'] # inspection object only; force actual static paths to be emitted
with tempfile.TemporaryDirectory(prefix='qgwd-native-source-',dir='/private/tmp') as directory:
    obj=Path(directory)/'native.o'
    if qgwe:
        compiler=os.environ.get('CXX','c++');role=pioneer/('PioneerQ38QgwfNativeInspectionRole.h' if qgwf else 'PioneerQ38QgweNativeInspectionRole.h')
        entry=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
        rename=['-D'+name+'=qgwe_private_'+name for name in entry]
        inspection='PIONEER_Q38_QGWF_NATIVE_INSPECTION' if qgwf else 'PIONEER_Q38_QGWE_NATIVE_INSPECTION'
        probe='#ifndef '+inspection+'\n#error missing inspection\n#endif\n#include "'+str(role)+'"\n'
        def qgwe_check(arguments,success,body=probe):
            result=subprocess.run([compiler,'-std=c++17',*arguments,'-x','c++','-fsyntax-only','-'],input=body,capture_output=True,text=True)
            assert (result.returncode==0)==success,result.stderr[:1000]
        selected=flags+rename;qgwe_check(selected,True)
        for name in ('PIONEER_Q38_QGWE_NATIVE_INSPECTION','PIONEER_Q38_QGWD_NATIVE_INSPECTION',
                     'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
                     'PIONEER_Q38_QGWE_PROGRAMS_INSPECTION','PIONEER_Q38_QGWE_BATCH_DENSE_PLAN','_KERNEL_MODE') + (('PIONEER_Q38_QGWF_NATIVE_INSPECTION','PIONEER_Q38_QGWF_PROGRAMS_INSPECTION','PIONEER_Q38_QGWF_DUAL_BATCH_PLAN') if qgwf else ()):
            flag='-D'+name+'=1';assert flag in selected
            qgwe_check([f for f in selected if f!=flag],False)
            qgwe_check([f for f in selected if f!=flag]+['-D'+name+'=0'],False)
        for name in ('PIONEER_Q38_QGWD_NATIVE_141','PIONEER_Q38_HYBRID_FULL_NATIVE',
                     'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                     'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
            qgwe_check(selected+['-D'+name+'=1'],False)
        tag='-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL'
        qgwe_check([f for f in selected if f!=tag]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026111100010142ULL'],False)
        qgwe_check([f for f in selected if f!=rename[0]],False)
        qgwe_check(selected,False,probe+'#undef '+inspection+'\n#include "'+str(role)+'"\n')
    if candidate:
        compiler=os.environ.get('CXX','c++');role=pioneer/'PioneerQ38QgwdNativeCandidateRole.h'
        def check_role(arguments,success):
            probe='#ifndef PIONEER_Q38_QGWD_NATIVE_141\n#error candidate selector required\n#endif\n#include "'+str(role)+'"\n'
            result=subprocess.run([compiler,'-std=c++17',*arguments,'-x','c++','-fsyntax-only','-'],input=probe,capture_output=True,text=True)
            assert (result.returncode==0)==success,result.stderr[:1000]
        check_role(flags,True)
        for name in ('_KERNEL_MODE','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_QGWD_NATIVE_141',
                     'PIONEER_Q38_QGWD_NATIVE_INSPECTION','PIONEER_Q38_HYBRID_FULL_NATIVE',
                     'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                     'PIONEER_Q38_QGWD_CONTROL_INSPECTION','PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION'):
            selected='-D'+name+'=1';assert selected in flags
            check_role([f for f in flags if f!=selected],False)
            check_role([f for f in flags if f!=selected]+['-D'+name+'=0'],False)
        for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
                     'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
                     'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
                     'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                     'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
            check_role(flags+['-D'+name+'=1'],False)
        tag='-DPIONEER_DRIVER_BUILD_TAG='+('0x2026111100010142ULL' if two else '0x2026111000010141ULL')
        check_role([f for f in flags if f!=tag]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110900010136ULL'],False)
        if two:
            check_role([f for f in flags if f!='-DPIONEER_Q38_QGWD_TWO_NATIVE_142=1'],False)
            check_role([f for f in flags if f!=tag]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010141ULL'],False)
        reload_header=root/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
        def check_reload(arguments,success):
            result=subprocess.run([compiler,'-std=c++17',*arguments,'-DB_DEVICE_OP_CODES_END=1000',
                '-x','c++','-fsyntax-only',str(reload_header)],capture_output=True,text=True)
            assert (result.returncode==0)==success,result.stderr[:1000]
        check_reload(flags,True)
        check_reload([f for f in flags if f!='-DPIONEER_Q38_QGWD_NATIVE_141=1'],False)
        if two:check_reload([f for f in flags if f!='-DPIONEER_Q38_QGWD_TWO_NATIVE_142=1'],False)
        check_reload([f for f in flags if f!=tag]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110900010136ULL'],False)
        protocol=Path(directory)/'protocol'
        subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-Werror',*flags,
                        str(here/'pioneer_q38_qgwd_candidate_protocol_test.cpp'),'-o',str(protocol)],check=True)
        subprocess.run([str(protocol)],check=True)
    source=(root/'src/add-ons/kernel/drivers/misc/pioneer_q38_hybrid_full_driver.cpp') if candidate else (here/'pioneer_q38_token_native_context_compile.cpp')
    if two:
        missing=Path(directory)/'missing-two-marker.h'
        data=header.read_text();marker='#define PIONEER_Q38_QGWD_TWO_NATIVE_DATA_142 1\n'
        assert data.count(marker)==1;missing.write_text(data.replace(marker,''))
        old_header='-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"'
        bad_header='-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(missing)+'"'
        result=subprocess.run([*(bad_header if f==old_header else f for f in common),'-fsyntax-only',str(source)],capture_output=True)
        assert result.returncode and b'tag142 requires its distinct two-token candidate DATA' in result.stderr
        one_tag='-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010141ULL'
        old_role=[one_tag if f==tag else f for f in common if f!='-DPIONEER_Q38_QGWD_TWO_NATIVE_142=1']
        result=subprocess.run([*old_role,'-fsyntax-only',str(source)],capture_output=True)
        assert result.returncode and b'requires tag142 native role' in result.stderr
    result=subprocess.run([*common,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
    if result.returncode:raise SystemExit('QGWD full native source compile failed:\n'+result.stderr[:12000])
    nm=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
    table=subprocess.check_output([nm,'--defined-only',str(obj)],text=True)
    forbidden={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
    symbols={line.split()[-1] for line in table.splitlines() if line.split()}
    if candidate:
        assert forbidden <= symbols
    else:
        assert not forbidden & symbols
        for name in ('pioneer_token_context_qgwd_control','pioneer_token_context_scoped_bind','pioneer_token_context_scoped_held',
                     'pioneer_token_context_scoped_runtime','pioneer_token_context_scoped_bootstrap'):
            assert name in table,name
    rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
    demangled=subprocess.check_output([nm,'-C','--defined-only',str(obj)],text=True)
    for name in ('Q38QgwdUploadControlInspection(', 'BindQ38TokenNative(', 'PrepareQ38TokenNativeHeld(',
                 'PrepareQ38TokenNativeRuntime(', 'BootstrapQ38TokenNativeWorkers('):
        fallback={'PrepareQ38TokenNativeHeld(':'PioneerQ38TokenNativeHeld.inc:89:',
                  'PrepareQ38TokenNativeRuntime(':'PioneerQ38TokenNativeRuntime.inc:26:',
                  'BootstrapQ38TokenNativeWorkers(':'PioneerQ38TokenNativeRuntime.inc:52:'}
        found=[r for r in rows if name in r[0] or candidate and name in fallback
               and fallback[name] in r[0] and ':constprop(' in r[0]]
        assert name in demangled and found and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in found),(name,found)
    print('PASS real '+('QGWF' if qgwf else 'QGWE' if qgwe else 'QGWD')+' '+('candidate source/object' if candidate else 'native source/object with renamed driver ABI')+' and bounded selected frames; COMPILE ONLY, NO DEVICE')
