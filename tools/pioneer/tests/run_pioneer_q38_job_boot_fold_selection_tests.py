#!/usr/bin/env python3
"""Selected boot-fold DATA/emitted equivalence and containment;no card IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
old,candidate=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (old,candidate))
def literal(path,name):
    return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body if isinstance(n,ast.Assign)
                and any(isinstance(t,ast.Name) and t.id==name for t in n.targets))
for directory,pins in ((old,literal(here/'run_pioneer_q38_packet_seal_unroll_instruction_tests.py','candidate_pins_eight')),
                       (candidate,literal(here/'run_pioneer_q38_job_boot_fold_connected_tests.py','CANDIDATE_PINS'))):
    for name,size,digest in pins:
        data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
patch=(here.parent/'experiments/q38-job-boot-fold-20261030.patch').read_bytes()
assert hashlib.sha256(patch).hexdigest()=='03b97e8267af4b831a4f7b7c758278514904c4ab562c3ccbf06b5e13c8f8110f'
def frozen_source(name):
    start='diff --git a/tools/pioneer/tests/'+name+' b/tools/pioneer/tests/'+name+'\n'
    chunk=patch.decode().split(start,1)[1].split('\ndiff --git ',1)[0]
    assert 'new file mode 100644' in chunk
    return '\n'.join(line[1:] for line in chunk.splitlines() if line.startswith('+') and not line.startswith('+++'))+'\n'
cross='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
entries=[]
for directory in (old,candidate):
    symbols=subprocess.check_output([cross+'nm','-n',str(directory/'program.elf')],text=True)
    assert symbols==(directory/'program.symbols').read_text()
    demangled=subprocess.check_output([cross+'nm','-nC',str(directory/'program.elf')],text=True)
    found=re.findall(r'^([0-9a-f]+) [TW] PioneerQ38PushedWorkerLayout::JobShape\(',demangled,re.M)
    assert len(found)==1;entries.append(found[0])
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror']
host={'PIONEER_Q38_JOB_BOOT_FOLD_IMAGE':'1','PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST':'1',
      'PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION':'1','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION':'1'}
header=here.parent/'PioneerQ38PushedWorkerLayout.h'
refused=0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-selected-boot-fold-',dir='/private/tmp') as name:
    out=pathlib.Path(name)
    for source in ('pioneer_q38_job_boot_fold_test.cpp','pioneer_q38_job_boot_fold_instruction_test.cpp'):
        text=frozen_source(source).replace('#include "../PioneerQ38PushedWorkerLayout.h"','#include "'+str(header)+'"')
        text=text.replace('#include "pioneer_tensix_image_test.cpp"','#include "'+str(here/'pioneer_tensix_image_test.cpp')+'"')
        (out/source).write_text(text)
    flags=['-D'+k+'='+v for k,v in sorted(host.items())]
    env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
    for source,args in (('pioneer_q38_job_boot_fold_test.cpp',[]),
                        ('pioneer_q38_job_boot_fold_instruction_test.cpp',[str(old),str(candidate),*entries])):
        binary=out/(source+'.bin')
        subprocess.run([*common,'-fsanitize=address,undefined',*flags,str(out/source),'-o',str(binary)],check=True)
        subprocess.run([str(binary),*args],check=True,env=env)
    def check(values,okay,tail=''):
        device_check='__riscv' in values and 'PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST' not in values
        # Positive device checks cover the role directly without borrowing
        # HOST libc headers. The complete real device DATA header and body
        # are compiled separately by the pinned ROOT builds above.
        selected_header=here.parent/'PioneerQ38JobBootFoldRole.h' if device_check else header
        source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+'#include "'+str(selected_header)+'"\n'+tail
        # Use the REAL ELF-target compiler for positive device-role syntax;
        # pretending __riscv on Mach-O would reject valid ELF section names.
        compiler=([cross+'g++','-std=c++11','-Wall','-Wextra','-Werror']
                  if device_check else common)
        result=subprocess.run([*compiler,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,result.stderr
    check({},True,'static_assert(PioneerQ38JobBootFoldProfile==0,"default unchanged");\n')
    check(host,True,'static_assert(PioneerQ38JobBootFoldProfile==1,"selected only");\n')
    for role in ('PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
        device={k:v for k,v in host.items() if k!='PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST'}
        check({**device,'__riscv':'1',role:'1'},True)
    for key in host:
        for value in (None,'0','2'):
            bad=host.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            check(bad,False);refused+=1
        check(host,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(header)+'"\n');refused+=1
    for key in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                'PIONEER_Q38_GLOBAL_WORKER_IMAGE','__FAST_MATH__','__riscv',
                'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
        for value in ('0','1'):check({**host,key:value},False);refused+=1
    check({},False,'#define PIONEER_Q38_JOB_BOOT_FOLD_IMAGE 1\n#include "'+str(header)+'"\n');refused+=1
    check(host,False,'#undef PIONEER_Q38_JOB_BOOT_FOLD_IMAGE\n#undef PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST\n#include "'+str(header)+'"\n');refused+=1
    # BUILD selection only:all missing predecessors must refuse before mkdir,
    # tool invocation or any model/firmware/native side effect.
    base=['--inspect-candidate','--f32-hidden','--unroll-dense-seals','--boundary-seals','--unroll-quiet',
          '--worker-local-source-guards','--precise-activation','--single-closed-audit','--worker-sealed-admission',
          '--pushed-completion','--unroll-packet-seals','--unroll-packet-seals-eight','--fold-job-boot']
    cli=0;sdk='/Volumes/HaikuBuildLocal/generated.riscv64'
    for script,parent in (('build_qwen38_token_root.py',True),('build_qwen38_global_token_components.py',False)):
        full=base+(['--cancel-boundary'] if parent else [])
        bads=[[value for value in full if value!=missing] for missing in full if missing!='--fold-job-boot']
        bads += [['--fold-job-boot'],full+['--fold-job-boot'],full+['--unknown-job-fold']]
        for values in bads:
            destination=out/f'denied-{cli}';before=sorted(p.relative_to(out) for p in out.rglob('*'))
            result=subprocess.run([sys.executable,str(here.parent/script),sdk,str(destination),*values],capture_output=True,text=True)
            assert result.returncode and 'AssertionError' in result.stderr,(script,values,result.stderr)
            assert not destination.exists() and before==sorted(p.relative_to(out) for p in out.rglob('*'));cli+=1
    for values in (['--job-boot-fold-eight','--job-boot-fold-eight'],
                   ['--inspect-candidate','--inspect-candidate'],
                   ['--job-boot-fold-eight','--inspect-candidate','--job-boot-fold-eight'],
                   ['--job-boot-fold-eight','--unknown-job-fold'],
                   ['--inspect-candidate','--unknown-job-fold'],['--unknown-job-fold']):
        destination=out/f'denied-{cli}';before=sorted(p.relative_to(out) for p in out.rglob('*'))
        result=subprocess.run([sys.executable,str(here.parent/'build_qwen38_token_fault_marker_image.py'),
                               str(out/'MUST_NOT_READ_SDK'),str(destination),*values],capture_output=True,text=True)
        assert result.returncode==2 and not result.stdout and not result.stderr
        assert not destination.exists() and before==sorted(p.relative_to(out) for p in out.rglob('*'));cli+=1
    print(f'PASS_SELECTED_JOB_BOOT_FOLD 4 positive roles/{refused} missing+zero+foreign+late refusals/{cli} pre-output BUILD CLI refusals;frozen DATA and actual emitted equivalence above;NO native/physical/Work/TPS grant',flush=True)
