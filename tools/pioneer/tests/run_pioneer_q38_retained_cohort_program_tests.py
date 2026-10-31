#!/usr/bin/env python3
"""Complete retained-cohort immutable DATA,not execution/native authority."""
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
assert all(p.is_absolute() for p in (root,worker,old_root))
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
inputs=((root,helpers['pins'](here.parent/'build_qwen38_retained_cohort_root.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_precise_beta_worker_image.py','PINS')))
for directory,pins in inputs:helpers['authenticated'](directory,pins)
committed=here.parent/'PioneerQ38TokenRetainedCohortPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest()=='3e5cec29942498a78227effe268f58a7fa567358d8f58dd7d04922a1ec364803'
selectors=('PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS','PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS',
           'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS',
           'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-retained-cohort-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);header=out/'retained-data.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(header),'--retained-conv-closure'],check=True)
    assert header.read_bytes()==committed.read_bytes()
    old=out/'unchanged-frontier.h'
    subprocess.run([sys.executable,str(generator),str(old_root),str(worker),str(old),'--token-frontier'],check=True)
    assert old.read_bytes()==(here.parent/'PioneerQ38TokenFrontierPrograms.h').read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',*['-D'+key+'=1' for key in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_RETAINED_COHORT=1','-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    refused=0
    for directory,pins in inputs:
        for artifact,_,_ in pins:
            case=out/f'bad-{refused}';case.mkdir()
            for path in directory.iterdir():
                if path.is_file():(case/path.name).symlink_to(path)
            target=case/artifact;target.unlink() # this test's own symlink ONLY
            original=(directory/artifact).read_bytes();corrupt=bytearray(original);corrupt[len(corrupt)//2]^=1
            for data in (corrupt,original[:-1]):
                target.write_bytes(data);output=out/f'NO_OUTPUT-{refused}'
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--retained-conv-closure'],capture_output=True)
                assert result.returncode==2 and artifact.encode() in result.stderr and not output.exists(),result.stderr
                refused+=1
    assert refused==30
    base=dict.fromkeys(selectors,'1');role=here.parent/'PioneerQ38TokenRetainedCohortProgramsRole.h'
    def include(path):return '#include "'+str(path)+'"\n'
    positives=denied=0
    def syntax(values,okay,body=None):
        global positives,denied
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positives+=1
        else:assert 'error:' in result.stderr;denied+=1
    syntax(base,True)
    syntax(base,True,include(committed)+include(committed)+'static_assert(PioneerQ38TokenRetainedCohortPrograms::RetainedConvClosure==1);\n')
    for key in selectors:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False,include(committed))
        syntax(base,False,include(committed)+'#undef '+key+'\n#define '+key+' 0\n'+include(committed))
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-set(selectors)
    for key in sorted(foreign):
        for value in ('0','1'):syntax({**base,key:value},False)
        syntax(base,False,include(role)+'#define '+key+' 0\n'+include(role))
    selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
    syntax(base,False,include(selected))
    syntax({},False,include(selected)+''.join('#define '+key+' 1\n' for key in selectors)+include(role))
    print(f'PASS retained-cohort DATA/15 authenticated artifacts/30 corrupt+truncated refusals/{positives} syntax positives/{denied} missing+zero+two+late+native+USER+CPU+Work refusals/old full2bd70b27 frontier header unchanged;NO native selector,physical,model,timing or Work grant')
