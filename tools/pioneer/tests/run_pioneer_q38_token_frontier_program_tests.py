#!/usr/bin/env python3
"""Complete frontier firmware DATA and selection; no native or card IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker,old_root=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(path.is_absolute() for path in (root,worker,old_root))
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
inputs=((root,helpers['pins'](here.parent/'build_qwen38_token_fault_marker_image.py','FRONTIER_PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_precise_beta_worker_image.py','PINS')))
for directory,pins in inputs:helpers['authenticated'](directory,pins)
committed=here.parent/'PioneerQ38TokenFrontierPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest()=='2bd70b273e8c6b2196c0d087ca03d87f34a5567cb1cb361b305483ce4b17c7e5'
selectors=('PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS','PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
           'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-programs-',dir='/private/tmp') as name:
    out=pathlib.Path(name);header=out/'programs.h'
    full=['--job-boot-fold-eight','--token-frontier']
    bad_options=[['--token-frontier'],['--inspect-candidate','--token-frontier'],
                 full+['--job-boot-fold-eight'],full+['--token-frontier'],
                 full+['--inspect-candidate','--inspect-candidate'],full+['--unknown-frontier']]
    for number,options in enumerate(bad_options):
        target=out/f'NO_BUILD-{number}';before=sorted(out.iterdir())
        result=subprocess.run([sys.executable,str(here.parent/'build_qwen38_token_fault_marker_image.py'),
                               str(out/'MUST_NOT_READ_SDK'),str(target),*options],capture_output=True)
        assert result.returncode==2 and not target.exists() and before==sorted(out.iterdir()),result.stderr
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(header),'--token-frontier'],check=True)
    assert header.read_bytes()==committed.read_bytes()
    # Generator changes MUST preserve the complete existing job-fold package.
    old=out/'old-control.h'
    subprocess.run([sys.executable,str(generator),str(old_root),str(worker),str(old),'--job-boot-fold'],check=True)
    assert old.read_bytes()==(here.parent/'PioneerQ38TokenJobBootFoldPrograms.h').read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',*['-D'+key+'=1' for key in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_FRONTIER=1','-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    refused=0
    for directory,pins in inputs:
        for artifact,_,_ in pins:
            case=out/f'bad-{refused}';case.mkdir()
            for path in directory.iterdir():
                if path.is_file():(case/path.name).symlink_to(path)
            target=case/artifact;target.unlink() # test-owned symlink only
            original=(directory/artifact).read_bytes();corrupt=bytearray(original);corrupt[len(corrupt)//2]^=1
            for data in (corrupt,original[:-1]):
                target.write_bytes(data);output=out/f'NO_OUTPUT-{refused}'
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--token-frontier'],capture_output=True)
                assert result.returncode==2 and artifact.encode() in result.stderr and not output.exists(),result.stderr
                refused+=1
    assert refused==30
    base=dict.fromkeys(selectors,'1');role=here.parent/'PioneerQ38TokenFrontierProgramsRole.h'
    def include(path):return '#include "'+str(path)+'"\n'
    selected=here.parent/'PioneerQ38TokenSelectedPrograms.h';accepted=denied=0
    def syntax(values,okay,body=None):
        global accepted,denied
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:accepted+=1
        else:
            assert 'error:' in result.stderr;denied+=1
    syntax(base,True)
    syntax(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenPrograms::TokenFrontier==1);\n')
    syntax({},True,include(selected)+include(selected))
    for key in selectors:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False,include(committed))
        syntax(base,False,include(committed)+'#undef '+key+'\n#define '+key+' 0\n'+include(committed))
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-set(selectors)
    # Inherited parent boundaries must also reject CPU/Work/old USER roles.
    foreign|={'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
              'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT',
              'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE','PIONEER_Q38_GLOBAL_WORKER_IMAGE','__FAST_MATH__'}
    for key in sorted(foreign):
        for value in ('0','1'):syntax({**base,key:value},False,include(committed))
        syntax(base,False,include(committed)+'#define '+key+' 0\n'+include(committed))
    syntax({},False,include(selected)+''.join('#define '+key+' 1\n' for key in selectors)+include(selected))
    syntax(base,False,include(selected)+'#undef PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS\n'+include(selected))
    print(f'PASS complete frontier DATA/15 authenticated artifacts/30 corrupt+truncated refusals/{len(bad_options)} pre-SDK CLI refusals/{accepted} syntax positives/{denied} missing+zero+two+late+native+physical+CPU+Work refusals/old full1b8e header unchanged;NO native owner,reader,physical,timing,Work or TPS grant')
