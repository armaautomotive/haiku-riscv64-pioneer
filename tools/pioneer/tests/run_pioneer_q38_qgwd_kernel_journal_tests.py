#!/usr/bin/env python3
"""Build QGWD journal templates for Haiku RISC-V; never link a driver."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
if len(sys.argv)!=6:
    raise SystemExit('usage: run_pioneer_q38_qgwd_kernel_journal_tests.py ABS_SDK ABS_ROOT_A ABS_ROOT_B ABS_WORKER_A ABS_WORKER_B')
sdk,root_a,root_b,worker_a,worker_b=map(Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() for p in (sdk,root_a,root_b,worker_a,worker_b))
here=Path(__file__).resolve().parent;root=here.parent;generator=root/'generate_qwen38_hybrid_programs.py'
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
with tempfile.TemporaryDirectory(prefix='qgwd-kernel-journal-',dir='/private/tmp') as directory:
    tmp=Path(directory);a=tmp/'a.h';b=tmp/'b.h'
    for image,worker,out in ((root_a,worker_a,a),(root_b,worker_b,b)):
        subprocess.run([sys.executable,str(generator),'--qgwd-dense-q8-kernel-compile',str(image),str(worker),str(out)],check=True)
    assert a.read_bytes()==b.read_bytes()
    assert hashlib.sha256(a.read_bytes()).hexdigest()=='f5691d5f12cb1c59fc26a7b4044c510d2eb9cd66aa80481c549be0f10b7c6159'
    flags=['-D_KERNEL_MODE=1','-DPIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY=1','-DPIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY=1',
           '-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR','-DPIONEER_Q38_SPAN_FAST',
           '-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
           '-DPIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION=1','-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1',
           '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(a)+'"','-DPIONEER_Q38_QGW8_LAYOUT_INSPECTION=1',
           '-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
           '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1','-DPIONEER_Q38_QGWD_PROGRAMS_INSPECTION=1',
           '-DPIONEER_Q38_QGWD_PLAN_SIMULATION=1','-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1',
           '-DPIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE=1','-DPIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION=1',
           '-DPIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION=1','-DPIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION=1',
           '-DPIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION=1','-DPIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION=1']
    flags+=['-DPIONEER_Q38_QGWD_CONTROL_INSPECTION=1']
    common=[tool+'g++','-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
            '-fno-exceptions','-fno-rtti','-ffreestanding','-fstack-usage','-mcmodel=medany','-fpic',
            '-I'+str(root),'-I'+str(headers),'-idirafter',str(headers/'posix'),*flags]
    source=str(here/'pioneer_q38_token_memory_kernel_compile.cpp');obj=tmp/'journal.o'
    result=subprocess.run([*common,'-c',source,'-o',str(obj)],capture_output=True,text=True)
    if result.returncode:
        raise SystemExit('kernel compile failed:\n'+result.stderr[:6000])
    frames=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in frames)
    table=subprocess.check_output([tool+'nm','--defined-only',str(obj)],text=True)
    for name in ('pioneer_q38_token_memory_template_prepare','pioneer_q38_qgwd_bind_template',
                 'pioneer_q38_qgwd_native_journal_bind_template',
                 'pioneer_q38_qgwd_manifest_install_template','pioneer_q38_qgwd_manifest_verify_template',
                 'pioneer_q38_qgwd_runtime_template','pioneer_q38_qgwd_bootstrap_template',
                 'pioneer_q38_qgwd_start_template','pioneer_q38_qgwd_hold_template',
                 'pioneer_q38_qgwd_restore_l1_template','pioneer_q38_qgwd_restore_images_template',
                 'pioneer_q38_qgwd_restore_peers_template','pioneer_q38_qgwd_restore_root_template',
                 'pioneer_q38_qgwd_restore_shared_template','pioneer_q38_qgwd_read_template'):
        assert name in table,name
    forbidden={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
    assert not any(line.split()[-1] in forbidden for line in table.splitlines() if line.split())
    for name in ('PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION'):
        result=subprocess.run([*common,'-D'+name+'=1','-fsyntax-only',source],capture_output=True)
        assert result.returncode,name
    for flag in ('-D_KERNEL_MODE=1','-DPIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY=1',
                 '-DPIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY=1'):
        result=subprocess.run([*(f for f in common if f != flag),'-fsyntax-only',source],capture_output=True)
        assert result.returncode,'missing '+flag
        result=subprocess.run([*(f for f in common if f != flag),flag[:-1]+'0',
                               '-fsyntax-only',source],capture_output=True)
        assert result.returncode,'zero '+flag
    print('PASS pinned QGWD journal and runtime lifecycle Haiku RISC-V compile; max static frame',
          max(int(row[1]) for row in frames),'; NOT A LOADABLE DRIVER')
