#!/usr/bin/env python3
"""Frozen complete sampler DATA and actual role refusals;NO device access."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root,worker))
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
inputs=((root,helpers['pins'](here.parent/'build_qwen38_token_phase_profile_root.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_token_matrix_phase_worker.py','PINS')))
for directory,records in inputs:helpers['authenticated'](directory,records)
header=here.parent/'PioneerQ38TokenMatrixPhasePrograms.h'
assert hashlib.sha256(header.read_bytes()).hexdigest()=='e42b21bd0a5c0f061f3bbbd2cb620e988d03f561b62fdf444624959ddb03ca09'
primary='PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS';role=here.parent/'PioneerQ38TokenMatrixPhaseProgramsRole.h'
boundary=here.parent/'PioneerQ38TokenMatrixPhaseDataBoundary.h';include=lambda p:'#include "'+str(p)+'"\n'
with tempfile.TemporaryDirectory(prefix='pioneer-q38-matrix-phase-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);generated=out/'new-data.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(generated),'--token-matrix-phase'],check=True)
    assert generated.read_bytes()==header.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'data-test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined','-D'+primary+'=1',
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_MATRIX_PHASE=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(generated)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True)
    corruptions=wrong=positive=negative=0
    for directory,records in inputs:
        for name,_,_ in records:
            case=out/('bad-'+str(corruptions));case.mkdir()
            for file,_,_ in records:(case/file).symlink_to(directory/file)
            target=case/name;target.unlink() # ONLY the exact new test link
            original=(directory/name).read_bytes();damaged=bytearray(original);damaged[len(damaged)//2]^=1
            for raw in (damaged,original[:-1]):
                target.write_bytes(raw);output=out/('NO_OUTPUT-'+str(corruptions))
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--token-matrix-phase'],capture_output=True)
                assert result.returncode==2 and name.encode() in result.stderr and not output.exists(),result.stderr
                corruptions+=1
    assert corruptions==32
    for r,w,mode in ((root,worker/'unchanged-constant-q4','--token-matrix-phase'),
                     (root,worker,'--constant-q4-issue'),(root,worker,'--token-phase-profile')):
        output=out/('WRONG-'+str(wrong));result=subprocess.run([sys.executable,str(generator),str(r),str(w),str(output),mode],capture_output=True)
        assert result.returncode==2 and not output.exists(),result.stderr
        wrong+=1
    def syntax(values,okay,body=None,selection=1):
        global positive,negative
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)
        if okay:source+=f'static_assert(PioneerQ38TokenMatrixPhaseDataSelection=={selection});\n'
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positive+=1
        else:
            assert 'error:' in result.stderr;negative+=1
    base={primary:'1'}
    syntax({},True,selection=0);syntax(base,True)
    syntax(base,True,include(header)+include(header)+'static_assert(PioneerQ38TokenMatrixPhasePrograms::MatrixPhaseVersion==2);\n')
    for value in (None,'0','2'):syntax({} if value is None else {primary:value},False,include(header))
    own_guard='PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS_H'
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()))-{primary,own_guard}
    for key in sorted(foreign):
        for value in ('0','1'):
            syntax({**base,key:value},False)
            syntax(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
    syntax({},False,include(role)+'#define '+primary+' 1\n'+include(role))
    syntax(base,False,include(role)+'#undef '+primary+'\n'+include(role))
    syntax(base,False,include(here.parent/'PioneerQ38TokenSelectedPrograms.h'))
    for name in ('PioneerQ38TokenConstantQ4Programs.h','PioneerQ38TokenPhaseProfilePrograms.h','PioneerQ38TokenWideGdnPrograms.h',
                 'PioneerQ38TokenTwinBatchPrefetchPrograms.h','PioneerQ38TokenPrograms.h'):
        syntax(base,False,include(here.parent/name));syntax(base,False,include(header)+include(here.parent/name))
    print(f'PASS new full returning matrix sampler14-section DATA/16 artifacts/{corruptions} corrupt+truncated/{wrong} wrong-family/'
          f'{positive} positive/{negative} production-only syntax refusals/all120 code words+119 Delta ranges/unchanged journal;'
          'NO native lifecycle,held sampler readout,model,hardware,Work or TPS grant',flush=True)
