#!/usr/bin/env python3
"""Complete copy-free twin-batch14-section DATA;NO native/device grant."""
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
old_root=root/'unchanged-flat';old_worker=worker/'unchanged-batched'
saved=sys.argv[:]
try:
    # ALL old DATA packages,source corruption and role controls FIRST.
    sys.argv=[str(here/'run_pioneer_q38_batched_prefetch_program_tests.py'),str(root),str(old_worker)]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
inputs=((root,helpers['pins'](here.parent/'build_qwen38_batched_prefetch_root.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_token_twin_batch_prefetch_worker.py','PINS')))
for directory,records in inputs:helpers['authenticated'](directory,records)
committed=here.parent/'PioneerQ38TokenTwinBatchPrefetchPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest()=='049613d63e3524be9a5aa1d3ef5f234adf4c61e4142a4d79eaf7e3bd50ae9db1'
primary='PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS'
selectors=(primary,);base={primary:'1'}
with tempfile.TemporaryDirectory(prefix='pioneer-q38-twin-batch-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);header=out/'new-data.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(header),'--twin-batch-prefetch'],check=True)
    assert header.read_bytes()==committed.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',*['-D'+k+'=1' for k in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH=1','-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',
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
                                       str(case if directory==worker else worker),str(output),'--twin-batch-prefetch'],capture_output=True)
                assert result.returncode==2 and name.encode() in result.stderr and not output.exists(),result.stderr
                corruptions+=1
    assert corruptions==32
    wrong=0
    for r,w,variant in ((old_root,worker,'--twin-batch-prefetch'),(root,old_worker,'--twin-batch-prefetch'),
                        (old_root,old_worker,'--twin-batch-prefetch'),(root,worker,'--tile-prefetch'),
                        (root,worker,'--flat-matrix-grammar')):
        output=out/('wrong-family-'+str(wrong))
        result=subprocess.run([sys.executable,str(generator),str(r),str(w),str(output),variant],capture_output=True)
        assert result.returncode==2 and not output.exists(),result.stderr
        wrong+=1
    role=here.parent/'PioneerQ38TokenTwinBatchPrefetchProgramsRole.h'
    boundary=here.parent/'PioneerQ38TokenTwinBatchPrefetchDataBoundary.h'
    positive=negative=0
    def include(path):return '#include "'+str(path)+'"\n'
    def syntax(values,okay,body=None,selection=1):
        global positive,negative
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)+f'static_assert(PioneerQ38TokenTwinBatchPrefetchDataSelection=={selection});\n'
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positive+=1
        else:assert 'error:' in result.stderr;negative+=1
    syntax({},True,selection=0);syntax(base,True)
    syntax(base,True,include(committed)+include(committed)+'static_assert(PioneerQ38TokenTwinBatchPrefetchPrograms::TwinBatchPrefetch==1);\n')
    for key in selectors:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False,include(committed))
        syntax(base,False,include(committed)+'#undef '+key+'\n#define '+key+' 0\n'+include(committed))
    own_guard='PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS_H'
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()))-set(selectors)-{own_guard}
    for key in sorted(foreign):
        for value in ('0','1'):syntax({**base,key:value},False)
        syntax(base,False,include(role)+'#define '+key+' 0\n'+include(role))
    for value in ('0','1'):
        syntax({**base,own_guard:value},False,include(committed)+'static_assert(PioneerQ38TokenTwinBatchPrefetchPrograms::TwinBatchPrefetch==1);\n')
    selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
    syntax(base,False,include(selected))
    syntax({},False,include(selected)+''.join('#define '+key+' 1\n' for key in selectors)+include(role))
    syntax(base,False,include(role)+'#undef '+primary+'\n'+include(role))
    for name in ('PioneerQ38TokenPrograms.h','PioneerQ38TokenPrecisePrograms.h',
                 'PioneerQ38TokenBatchedPrefetchPrograms.h','PioneerQ38TokenPrefetchPrograms.h','PioneerQ38TokenFlatMatrixPrograms.h',
                 'PioneerQ38TokenEmptyReceiptPollPrograms.h','PioneerQ38TokenPreciseBetaPrograms.h','PioneerQ38TokenPushedPrograms.h'):
        legacy=here.parent/name
        syntax(base,False,include(legacy))
        syntax(base,False,include(committed)+include(legacy))
    print(f'PASS distinct copy-free twin-batch14-section DATA/16 authenticated artifacts/{corruptions} corrupt+truncated/{wrong} wrong-family refusals/'
          f'{positive} syntax positives/{negative} missing+zero+two+late+oldarray+native+BUILD+USER+CPU+Work refusals;'
          'ALL old batched+prefetch+ancestor DATA controls FIRST;ALL120 code words in unchanged snapshot geometry;NO native/model/physical/TPS grant')
