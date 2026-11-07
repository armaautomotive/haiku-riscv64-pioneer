#!/usr/bin/env python3
"""Rebuild pinned QGW5 DATA and test sections; never access a device."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

args=sys.argv[1:];journal='--journal' in args
if journal:
    assert args.count('--journal')==1
    args.remove('--journal')
assert __debug__ and len(args)==1 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk=Path(args[0]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent;sys.path.insert(0,str(root))
from PioneerQ38HybridProgramPins import ROOT_PINS, WORKER_PINS
flags=['--inspect-candidate','--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals',
 '--unroll-quiet','--worker-local-source-guards','--precise-activation','--single-closed-audit',
 '--worker-sealed-admission','--bf16-fpu','--hybrid-bfp']
def run(*args,quiet=False):
    return subprocess.run([str(a) for a in args],check=True,stdout=subprocess.DEVNULL if quiet else None)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw5-programs-',dir='/private/tmp') as temporary:
    tmp=Path(temporary);image=tmp/'root';worker=tmp/'worker';out=tmp/'program.h';duplicate=tmp/'program-again.h'
    run(sys.executable,root/'build_qwen38_token_root.py',sdk,image,*flags,quiet=True)
    run(sys.executable,root/'build_q38_hybrid_worker.py',sdk,worker,quiet=True)
    generator=root/'generate_qwen38_hybrid_programs.py'
    run(sys.executable,generator,image,worker,out)
    run(sys.executable,generator,image,worker,duplicate)
    before=out.read_bytes();assert before==duplicate.read_bytes()
    # Existing output must never be overwritten, even by valid input.
    refused=subprocess.run([sys.executable,str(generator),str(image),str(worker),str(out)],capture_output=True)
    assert refused.returncode and out.read_bytes()==before
    # Each pinned section must also be the exact corresponding ELF section.
    objcopy=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-objcopy')
    for folder,pins in ((image,ROOT_PINS),(worker,WORKER_PINS)):
        for name,size,digest in pins:
            data=(folder/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
            if name.endswith('.bin'):
                extracted=tmp/('elf-'+folder.name+'-'+name)
                run(objcopy,'-O','binary','-j','.'+name[:-4],folder/'program.elf',extracted)
                assert extracted.read_bytes()==data,(folder.name,name)
    # Alter one independently pinned section and one complete ELF. Both must
    # fail before opening an output; do not touch the authentic images.
    for folder,pins,name in ((worker,WORKER_PINS,'gdn.bin'),(image,ROOT_PINS,'program.elf')):
        bad=tmp/('bad-'+folder.name);bad.mkdir()
        for filename,_,_ in pins:shutil.copyfile(folder/filename,bad/filename)
        with (bad/name).open('r+b') as f:
            b=f.read(1);assert b;f.seek(0);f.write(bytes([b[0]^1]))
        target=tmp/('refused-'+folder.name+'.h')
        args=(bad,worker) if folder==image else (image,bad)
        refused=subprocess.run([sys.executable,str(generator),*map(str,args),str(target)],capture_output=True)
        assert refused.returncode and not target.exists()
    binary=tmp/'test';compiler=os.environ.get('CXX','c++')
    run(compiler,'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
        '-I'+str(root),'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(out)+'"',
        here/'pioneer_q38_hybrid_programs_test.cpp','-o',binary)
    run(binary,image,worker)
    selection=here/'pioneer_q38_hybrid_program_selection_test.cpp'
    selectflags=['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1',
                 '-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1',
                 '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(out)+'"']
    selected=tmp/'selection'
    run(compiler,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root),*selectflags,selection,'-o',selected)
    run(selected)
    for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),'-D'+role,
            '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(out)+'"',
            '-E',str(here/'pioneer_q38_hybrid_programs_test.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Hybrid token journal geometry is experimental DATA only' in result.stderr,role
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),*selectflags,'-D'+role,
            '-E',str(selection)],capture_output=True,text=True)
        assert result.returncode and 'Hybrid QGW5 program selection is an offline journal fixture only' in result.stderr,role
    for missing in range(len(selectflags)):
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),
            *(flag for i,flag in enumerate(selectflags) if i!=missing),'-fsyntax-only',str(selection)],capture_output=True,text=True)
        assert result.returncode,('missing selector',missing)
    for extra in ('PIONEER_Q38_TOKEN_BF16_PROGRAMS_H','PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS',
                  'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_PROGRAMS'):
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),*selectflags,'-D'+extra,
            '-E',str(selection)],capture_output=True,text=True)
        assert result.returncode and 'Hybrid QGW5 program selection is an offline journal fixture only' in result.stderr,extra
    for old in ('PIONEER_Q38_TOKEN_BF16_PROGRAMS_H','PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS_H',
                'PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS_H','PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS_H',
                'PIONEER_Q38_TOKEN_PROGRAMS_H','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H'):
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),'-D'+old,
            '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(out)+'"',
            '-E',str(here/'pioneer_q38_hybrid_programs_test.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Exact offline QGW5 program DATA selector required' in result.stderr,old
    # The old program headers must also refuse if they are included AFTER the
    # hybrid package; selection order must not allow two immutable families.
    for role,late in (
        ('PioneerQ38TokenBf16ProgramsRole.h',''),
        ('PioneerQ38TokenBf16ProfileProgramsRole.h','#define PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS 1\n'),
        ('PioneerQ38TokenBf16PairIssueProgramsRole.h','#define PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS 1\n#define PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS 1\n'),
        ('PioneerQ38TokenBf16DeferredProgramsRole.h','#define PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS 1\n#define PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS 1\n')):
        source='#include "'+str(selection)+'"\n'+late+'#include "'+str(root/role)+'"\n'
        result=subprocess.run([compiler,'-std=c++17','-I'+str(root),*selectflags,'-x','c++','-E','-'],
                              input=source,capture_output=True,text=True)
        assert result.returncode and 'program DATA cannot alias' in result.stderr,role
    if journal:
        for mode in ('--positive','--hybrid-faults','--hybrid-manifest'):
            run(sys.executable,here/'run_pioneer_q38_token_memory_tests.py',
                '--bf16-programs','--hybrid-journal','--hybrid-programs',out,mode)
print('PASS complete QGW5 image section/ELF binding, immutable output, corruption refusals and role exclusions; NO DEVICE')
