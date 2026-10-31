#!/usr/bin/env python3
"""Authenticate/reproduce fault program DATA and its native-selection perimeter."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker=map(pathlib.Path,sys.argv[1:])
here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_qwen38_token_programs.py'
helpers=runpy.run_path(str(generator))
inputs=[(root,helpers['pins'](here.parent/'build_qwen38_token_fault_marker_image.py','PINS')),
        (worker,helpers['pins'](here.parent/'build_qwen38_pushed_worker_image.py','PINS'))]
for directory,pins in inputs:
    helpers['authenticated'](directory,pins)
digest='4e7a5fa71d546fb859c26376db50159bf6126c9e1185f48a36f7bad51eb11cdd'
committed=here.parent/'PioneerQ38TokenFaultPrograms.h'
assert hashlib.sha256(committed.read_bytes()).hexdigest()==digest
with tempfile.TemporaryDirectory(prefix='pioneer-q38-fault-programs-',dir='/private/tmp') as name:
    out=pathlib.Path(name)
    header=out/'programs.h'
    command=[sys.executable,str(generator),str(root),str(worker),str(header),'--fault-marker']
    subprocess.run(command,check=True)
    assert header.read_bytes()==committed.read_bytes()
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I'+str(here.parent)]
    binary=out/'data-test'
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined',
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_FAULT=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(header)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    refused=0
    for directory,pins in inputs:
        for artifact,_,_ in pins:
            case=out/f'bad-{refused}';case.mkdir()
            for path in directory.iterdir():
                if path.is_file():
                    (case/path.name).symlink_to(path)
            target=case/artifact;target.unlink()  # private symlink,never the real input
            original=(directory/artifact).read_bytes()
            corrupt=bytearray(original);corrupt[len(corrupt)//2]^=1
            for data in (corrupt,original[:-1]):
                target.write_bytes(data)
                output=out/f'NO_OUTPUT-{refused}'
                result=subprocess.run([sys.executable,str(generator),str(case if directory==root else root),
                                       str(case if directory==worker else worker),str(output),'--fault-marker'],capture_output=True)
                assert result.returncode==2 and artifact.encode() in result.stderr and not output.exists(),result.stderr
                refused+=1
    assert refused==30
    selected='#define PIONEER_Q38_TOKEN_FAULT_PROGRAMS 1\n#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1\n'
    include='#include "PioneerQ38TokenSelectedPrograms.h"\n'
    good=selected+include+include+'static_assert(PioneerQ38TokenPrograms::FaultMarker==0x51465431);\n'
    subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=good,text=True,check=True)
    bad_sources=[good.replace('FAULT_PROGRAMS 1','FAULT_PROGRAMS 0'),
                 good.replace('PUSHED_PROGRAMS 1','PUSHED_PROGRAMS 0'),
                 good.replace('#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1\n',''),
                 '#include "PioneerQ38TokenPushedPrograms.h"\n'+good,
                 include+selected+include,
                 selected+include+'#undef PIONEER_Q38_TOKEN_FAULT_PROGRAMS\n'+include]
    for role in ('PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_TOKEN_VALIDATION_CLIENT',
                 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT','PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT',
                 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS','PIONEER_Q38_TOKEN_VALIDATION_NATIVE',
                 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE'):
        bad_sources += ['#define '+role+' '+value+'\n'+good for value in ('0','1')]
    # Native selection without the exact private compile-only parents MUST fail.
    bad_sources += ['#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT '+value+'\n'+good for value in ('0','1')]
    for source in bad_sources:
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert result.returncode and ('error:' in result.stderr or '#error' in result.stderr),result.stderr
    assert len(bad_sources)==28
    print(f'PASS fault DATA fixed header/15 complete artifacts/30 corrupt+truncated refusals/ALL120 immutable code words/28 selector refusals;NO native runtime or hardware grant')
