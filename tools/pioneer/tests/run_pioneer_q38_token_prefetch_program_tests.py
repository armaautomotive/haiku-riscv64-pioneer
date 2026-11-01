#!/usr/bin/env python3
"""Complete new14-section immutable DATA; NO native/model/device IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker,old_worker=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root,worker,old_worker))
here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_qwen38_token_programs.py'
helpers=runpy.run_path(str(generator))
empty=root/'unchanged-empty'
counter=empty/'unchanged-read-source/unchanged-counter'
tid=counter/'unchanged-tid';packet=tid/'unchanged-packet';cohort=packet/'unchanged-cohort'
commit=cohort/'unchanged-commit';retained=commit/'unchanged-retained';frontier=retained/'unchanged-frontier'
saved=sys.argv[:]
try:
    # All previous immutable packages,byte identities,source corruptions
    # and role refusals must still pass FIRST without updating their pins.
    sys.argv=[str(here/'run_pioneer_q38_flat_matrix_program_tests.py'),
              *map(str,(root,old_worker,empty,counter,tid,packet,cohort,commit,retained,frontier))]
    runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
inputs=((root,helpers['pins'](here.parent/'build_qwen38_flat_matrix_grammar_root.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_token_prefetch_worker.py','PINS')))
for directory,records in inputs:helpers['authenticated'](directory,records)
committed=here.parent/'PioneerQ38TokenPrefetchPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest()=='021fdff4100dd6f8a6ff8548f9fa961473503cf6c2cd2e0b6adc44053b007567'
selectors=('PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS','PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS',
           'PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS','PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS',
           'PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS','PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS',
           'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS','PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS',
           'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS','PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS',
           'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS',
           'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-token-prefetch-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);header=out/'new-data.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(header),'--tile-prefetch'],check=True)
    assert header.read_bytes()==committed.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',*['-D'+k+'=1' for k in selectors],
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_PREFETCH=1','-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    corruptions=0
    for directory,records in inputs:
        for name,_,_ in records:
            case=out/('bad-'+str(corruptions));case.mkdir()
            for path in directory.iterdir():
                if path.is_file():(case/path.name).symlink_to(path)
            target=case/name;target.unlink()  # ONLY this fixture's newly created exact symlink.
            original=(directory/name).read_bytes();damaged=bytearray(original);damaged[len(damaged)//2]^=1
            for data in (damaged,original[:-1]):
                target.write_bytes(data);output=out/('NO_OUTPUT-'+str(corruptions))
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--tile-prefetch'],capture_output=True)
                assert result.returncode==2 and name.encode() in result.stderr and not output.exists(),result.stderr
                corruptions+=1
    assert corruptions==32
    # Wrong-generation worker must not produce an old9-section package or
    # a new14-section package;the whole worker identity is not interchangeable.
    for subject,variant in ((old_worker,'--tile-prefetch'),(worker,'--flat-matrix-grammar')):
        output=out/('wrong-worker-'+variant[2:])
        result=subprocess.run([sys.executable,str(generator),str(root),str(subject),str(output),variant],capture_output=True)
        assert result.returncode==2 and not output.exists(),result.stderr
    role=here.parent/'PioneerQ38TokenPrefetchProgramsRole.h'
    base=dict.fromkeys(selectors,'1');positive=negative=0

    def include(path):return '#include "'+str(path)+'"\n'

    def syntax(values,okay,body=None):
        global positive,negative
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positive+=1
        else:assert 'error:' in result.stderr;negative+=1

    syntax({},True,include(role)+'static_assert(PioneerQ38TokenPrefetchDataSelection==0);\n')
    syntax(base,True)
    syntax(base,True,include(committed)+include(committed)
           +'static_assert(PioneerQ38TokenPrefetchPrograms::TilePrefetch==1 && PioneerQ38TokenPrefetchPrograms::RawBuffers==2);\n')
    for key in selectors:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False,include(committed))
        syntax(base,False,include(committed)+'#undef '+key+'\n#define '+key+' 0\n'+include(committed))
    # The role now distinguishes an already included OWN header from a
    # legacy selected namespace. Its conventional include guard is not a
    # capability flag;test forged guards against the ACTUAL DATA below.
    own_guard='PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS_H'
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-set(selectors)-{own_guard}
    for key in sorted(foreign):
        for value in ('0','1'):syntax({**base,key:value},False)
        syntax(base,False,include(role)+'#define '+key+' 0\n'+include(role))
    for value in ('0','1'):
        syntax({**base,own_guard:value},False,include(committed)
               +'static_assert(PioneerQ38TokenPrefetchPrograms::TilePrefetch==1);\n')
    selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
    syntax(base,False,include(selected))
    syntax({},False,include(selected)+''.join('#define '+key+' 1\n' for key in selectors)+include(role))
    syntax(base,False,include(role)+'#undef '+selectors[0]+'\n'+include(role))
    for name in ('PioneerQ38TokenFlatMatrixPrograms.h','PioneerQ38TokenEmptyReceiptPollPrograms.h',
                 'PioneerQ38TokenPreciseBetaPrograms.h','PioneerQ38TokenPushedPrograms.h'):
        old_header=here.parent/name
        syntax(base,False,include(old_header))
        syntax(base,False,include(committed)+include(old_header))
    print(f'PASS current prefetch14-section DATA/16 authenticated artifacts/32 corrupt+truncated+2wrongworker refusals/'
          f'{positive} syntax positives/{negative} missing+zero+two+late+oldbinding+native+BUILD+USER+CPU+Work refusals;'
          'ALL120 code words covered by unchanged proposed snapshot geometry;NO native lifecycle/model/physical/TPS grant')
