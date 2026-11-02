#!/usr/bin/env python3
"""New complete returning-GDN DATA + boundaries; no driver/card/model execution."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,old_worker,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root,old_worker,worker))
# Full unchanged corrected-output DATA tests and all their controls FIRST.
subprocess.run([sys.executable,str(here/'run_pioneer_q38_batched_output_closure_program_tests.py'),str(root),str(old_worker)],check=True)
generator=here.parent/'generate_qwen38_token_programs.py';helpers=runpy.run_path(str(generator))
inputs=((root,helpers['pins'](here.parent/'build_qwen38_batched_output_closure_root.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_wide_gdn_worker_image.py','PINS')))
for directory,records in inputs:helpers['authenticated'](directory,records)
header=here.parent/'PioneerQ38TokenWideGdnPrograms.h'
assert hashlib.sha256(header.read_bytes()).hexdigest()=='d285c9adb421fc0010d8ad48371eb58ca1480fc0d787f7c147afd4a93457a787'
primary='PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS';role=here.parent/'PioneerQ38TokenWideGdnProgramsRole.h'
boundary=here.parent/'PioneerQ38TokenWideGdnDataBoundary.h'
include=lambda p:'#include "'+str(p)+'"\n'
with tempfile.TemporaryDirectory(prefix='pioneer-q38-wide-gdn-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);generated=out/'new-data.h'
    subprocess.run([sys.executable,str(generator),str(root),str(worker),str(generated),'--wide-gdn'],check=True)
    assert generated.read_bytes()==header.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'data-test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined','-D'+primary+'=1',
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_WIDE_GDN=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(generated)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True)
    corruptions=wrong=positive=negative=0
    for directory,records in inputs:
        for name,_,_ in records:
            case=out/('bad-'+str(corruptions));case.mkdir()
            for file,_,_ in records:(case/file).symlink_to(directory/file)
            target=case/name;target.unlink() # exact new fixture link ONLY
            original=(directory/name).read_bytes();damaged=bytearray(original);damaged[len(damaged)//2]^=1
            for raw in (damaged,original[:-1]):
                target.write_bytes(raw);output=out/('NO_OUTPUT-'+str(corruptions))
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--wide-gdn'],capture_output=True)
                assert result.returncode==2 and name.encode() in result.stderr and not output.exists(),result.stderr
                corruptions+=1
    assert corruptions==32
    for r,w,mode in ((root,old_worker,'--wide-gdn'),(root/'unchanged-batched',worker,'--wide-gdn'),
                     (root,worker,'--batched-output-closure'),(root,worker,'--twin-batch-prefetch')):
        output=out/('WRONG-'+str(wrong));result=subprocess.run([sys.executable,str(generator),str(r),str(w),str(output),mode],capture_output=True)
        assert result.returncode==2 and not output.exists(),result.stderr
        wrong+=1
    def syntax(values,okay,body=None,selection=1):
        global positive,negative
        source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
        source+=body if body is not None else include(role)+include(role)+f'static_assert(PioneerQ38TokenWideGdnDataSelection=={selection});\n'
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positive+=1
        else:
            assert 'error:' in result.stderr;negative+=1
    base={primary:'1'}
    syntax({},True,selection=0);syntax(base,True)
    syntax(base,True,include(header)+include(header)+'static_assert(PioneerQ38TokenWideGdnPrograms::WideGdn==1);\n')
    for value in (None,'0','2'):syntax({} if value is None else {primary:value},False,include(header))
    own_guard='PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS_H'
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',boundary.read_text()))-{primary,own_guard}
    for key in sorted(foreign):
        for value in ('0','1'):
            syntax({**base,key:value},False)
            syntax(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
    syntax({},False,include(role)+'#define '+primary+' 1\n'+include(role),selection=0)
    syntax(base,False,include(role)+'#undef '+primary+'\n'+include(role))
    syntax(base,False,include(here.parent/'PioneerQ38TokenSelectedPrograms.h'))
    for name in ('PioneerQ38TokenBatchedOutputClosurePrograms.h','PioneerQ38TokenTwinBatchPrefetchPrograms.h',
                 'PioneerQ38TokenBatchedPrefetchPrograms.h','PioneerQ38TokenPrefetchPrograms.h','PioneerQ38TokenPrograms.h'):
        syntax(base,False,include(here.parent/name));syntax(base,False,include(header)+include(here.parent/name))
    print(f'PASS new returning-GDN14-section DATA/16 authenticated artifacts/{corruptions} corrupt+truncated/{wrong} wrong-family/'
          f'{positive} syntax positives/{negative} missing,zero,two,late,old-array,native,BUILD,USER,CPU+Work refusals/'
          'ALL unchanged corrected-output+ancestors FIRST/all120 code words+all119 Delta ranges in unchanged journal;'
          'NO native lifecycle,original-model,hardware,Work or TPS grant',flush=True)
