#!/usr/bin/env python3
"""Compile real QGWD native driver body with renamed ABI; never link/run."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: run_pioneer_q38_qgwd_native_source_compile.py ABS_SDK ABS_PROGRAM_HEADER')
sdk,header=map(Path,sys.argv[1:])
assert sdk.is_absolute() and sdk.is_dir() and header.is_absolute() and header.is_file()
assert hashlib.sha256(header.read_bytes()).hexdigest()=='90dd22aada31d665a42fda9fa158afd888d13e04c45365203ae5f42c0fb6daff'
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
flags+=['-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL','-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-fno-exceptions','-fno-rtti','-fstack-usage','-ffreestanding','-finline','-fno-semantic-interposition',
 '-fno-tree-vectorize','-fno-use-cxa-atexit','-fno-strict-aliasing','-fno-delete-null-pointer-checks',
 '-mcmodel=medany','-fpic',*includes,*flags]
with tempfile.TemporaryDirectory(prefix='qgwd-native-source-',dir='/private/tmp') as directory:
    obj=Path(directory)/'native.o'
    result=subprocess.run([*common,'-c',str(here/'pioneer_q38_token_native_context_compile.cpp'),'-o',str(obj)],capture_output=True,text=True)
    if result.returncode:raise SystemExit('QGWD full native source compile failed:\n'+result.stderr[:12000])
    nm=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
    table=subprocess.check_output([nm,'--defined-only',str(obj)],text=True)
    forbidden={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
    assert not any(line.split()[-1] in forbidden for line in table.splitlines() if line.split())
    for name in ('pioneer_token_context_qgwd_control','pioneer_token_context_scoped_bind','pioneer_token_context_scoped_held',
                 'pioneer_token_context_scoped_runtime','pioneer_token_context_scoped_bootstrap'):
        assert name in table,name
    rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
    for name in ('Q38QgwdUploadControlInspection(', 'BindQ38TokenNative(', 'PrepareQ38TokenNativeHeld(',
                 'PrepareQ38TokenNativeRuntime(', 'BootstrapQ38TokenNativeWorkers('):
        found=[r for r in rows if name in r[0]]
        assert found and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in found),(name,found)
    print('PASS real QGWD native source/object with renamed driver ABI and bounded selected frames; COMPILE ONLY, NO DEVICE')
