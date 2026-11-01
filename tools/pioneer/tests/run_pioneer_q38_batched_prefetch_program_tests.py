#!/usr/bin/env python3
"""Complete matching batched14-section DATA;NO native or device authority."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root,worker))
here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
old_root=root/'unchanged-flat';old_worker=worker/'unchanged-prefetch'
saved=sys.argv[:]
try:
    # ALL old DATA packages,source corruption and role controls FIRST.
    sys.argv=[str(here/'run_pioneer_q38_token_prefetch_program_tests.py'),
              str(old_root),str(old_worker),str(old_worker/'unchanged-beta')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
inputs=((root,helpers['pins'](here.parent/'build_qwen38_batched_prefetch_root.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_token_batched_prefetch_worker.py','PINS')))
for directory,records in inputs:helpers['authenticated'](directory,records)
committed=here.parent/'PioneerQ38TokenBatchedPrefetchPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest()=='1ae2a7c9028ad7623390bf7512d38fca0528d731af361d444d72671e8db273f2'
primary='PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS'
selectors=(primary,*old['selectors']);base=dict.fromkeys(selectors,'1')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-batched-prefetch-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);header=out/'new-data.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(header),'--batched-prefetch'],check=True)
    assert header.read_bytes()==committed.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',*['-D'+k+'=1' for k in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_PREFETCH=1','-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    corruptions=0
    for directory,records in inputs:
        for name,_,_ in records:
            case=out/('bad-'+str(corruptions));case.mkdir()
            for path in directory.iterdir():
                if path.is_file():(case/path.name).symlink_to(path)
            target=case/name;target.unlink()  # ONLY this new exact fixture symlink.
            original=(directory/name).read_bytes();damaged=bytearray(original);damaged[len(damaged)//2]^=1
            for data in (damaged,original[:-1]):
                target.write_bytes(data);output=out/('NO_OUTPUT-'+str(corruptions))
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--batched-prefetch'],capture_output=True)
                assert result.returncode==2 and name.encode() in result.stderr and not output.exists(),result.stderr
                corruptions+=1
    assert corruptions==32
    wrong=0
    for r,w,variant in ((old_root,worker,'--batched-prefetch'),(root,old_worker,'--batched-prefetch'),
                        (old_root,old_worker,'--batched-prefetch'),(root,worker,'--tile-prefetch'),
                        (root,worker,'--flat-matrix-grammar')):
        output=out/('wrong-family-'+str(wrong))
        result=subprocess.run([sys.executable,str(generator),str(r),str(w),str(output),variant],capture_output=True)
        assert result.returncode==2 and not output.exists(),result.stderr
        wrong+=1
    role=here.parent/'PioneerQ38TokenBatchedPrefetchProgramsRole.h'
    boundary=here.parent/'PioneerQ38TokenBatchedPrefetchDataBoundary.h'
    positive=negative=0
    def include(path):return '#include "'+str(path)+'"\n'
    def syntax(values,okay,body=None,selection=1):
        global positive,negative
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)+f'static_assert(PioneerQ38TokenBatchedPrefetchDataSelection=={selection});\n'
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positive+=1
        else:assert 'error:' in result.stderr;negative+=1
    syntax({},True,selection=0);syntax(base,True)
    syntax(base,True,include(committed)+include(committed)+'static_assert(PioneerQ38TokenBatchedPrefetchPrograms::BatchedPrefetch==1);\n')
    for key in selectors:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False,include(committed))
        syntax(base,False,include(committed)+'#undef '+key+'\n#define '+key+' 0\n'+include(committed))
    own_guard='PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS_H'
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()
                          +(here.parent/'PioneerQ38TokenPrefetchProgramsRole.h').read_text()))-set(selectors)-{own_guard}
    for key in sorted(foreign):
        for value in ('0','1'):syntax({**base,key:value},False)
        syntax(base,False,include(role)+'#define '+key+' 0\n'+include(role))
    for value in ('0','1'):
        syntax({**base,own_guard:value},False,include(committed)+'static_assert(PioneerQ38TokenBatchedPrefetchPrograms::BatchedPrefetch==1);\n')
    selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
    syntax(base,False,include(selected))
    syntax({},False,include(selected)+''.join('#define '+key+' 1\n' for key in selectors)+include(role))
    syntax(base,False,include(role)+'#undef '+primary+'\n'+include(role))
    for name in ('PioneerQ38TokenPrefetchPrograms.h','PioneerQ38TokenFlatMatrixPrograms.h',
                 'PioneerQ38TokenEmptyReceiptPollPrograms.h','PioneerQ38TokenPreciseBetaPrograms.h','PioneerQ38TokenPushedPrograms.h'):
        legacy=here.parent/name
        syntax(base,False,include(legacy))
        syntax(base,False,include(committed)+include(legacy))
    print(f'PASS distinct matching batched14-section DATA/16 authenticated artifacts/{corruptions} corrupt+truncated/{wrong} wrong-family refusals/'
          f'{positive} syntax positives/{negative} missing+zero+two+late+oldarray+native+BUILD+USER+CPU+Work refusals;'
          'ALL old prefetch+ancestor DATA controls FIRST;ALL120 code words in unchanged snapshot geometry;NO native/model/physical/TPS grant')
