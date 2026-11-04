#!/usr/bin/env python3
"""Authenticate complete full-card firmware and test its actual DATA binding."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root,worker=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(here.parent))
from generate_qwen38_token_programs import authenticated,pins
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root,worker))
inputs=((root,pins(here.parent/'build_qwen38_full_offload_batched_root.py','PINS')),
        (worker,pins(here.parent/'build_qwen38_wide_gdn_worker_image.py','PINS')))
for directory,records in inputs:authenticated(directory,records)
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=kernel/'PioneerQ38FullOffloadRole.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()))
assert len(required)==23
base={name:'1' for name in required}
base.update(PIONEER_DRIVER_BUILD_TAG='0x2026110400010089ULL',PIONEER_Q38_FULL_OFFLOAD_BATCHED='1')
definitions=lambda values:''.join('#define '+name+' '+value+'\n' for name,value in sorted(values.items()))
include=lambda path:'#include "'+str(path)+'"\n'
header=here.parent/'PioneerQ38FullOffloadBatchedPrograms.h'
assert hashlib.sha256(header.read_bytes()).hexdigest()=='91624761b95d281cb9dd4493790b27a1c5d88d7c327d6df894711561b76b4c84'
with tempfile.TemporaryDirectory(prefix='pioneer-full-batched-data-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);generated=out/'programs.h'
    subprocess.run([sys.executable,str(here.parent/'generate_qwen38_full_offload_batched_programs.py'),
                    str(root),str(generated)],check=True)
    assert generated.read_bytes()==header.read_bytes()
    (out/'full_offload_batched_role_defines.inc').write_text(definitions(base))
    binary=out/'data'
    common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror']
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined','-I'+str(out),
                    str(here/'pioneer_q38_full_offload_batched_data_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True)
    positive=negative=0
    def syntax(values,okay,body=None):
        global positive,negative
        source=definitions(values)+(body if body is not None else include(header)+include(header))
        result=subprocess.run([*common,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,(values,body,result.stderr)
        if okay:positive+=1
        else:
            assert 'error:' in result.stderr;negative+=1
    syntax(base,True)
    selected=here.parent/'PioneerQ38TokenSelectedPrograms.h'
    syntax(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::WideGdn==1);\n')
    for key in sorted(base):
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False)
        syntax(base,False,include(header)+'#undef '+key+'\n'+include(header))
    for tag in ('0x2026110400010088ULL','0x2026103100010075ULL'):
        syntax({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False)
    foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-set(base)
    foreign.update(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',
                            (here.parent/'PioneerQ38FullOffloadBatchedProgramsRole.h').read_text()))
    foreign-=set(base)|{'PIONEER_Q38_TOKEN_FOOTPRINT_H'}
    for key in sorted(foreign):
        for value in ('0','1'):
            syntax({**base,key:value},False)
            syntax(base,False,include(header)+'#define '+key+' '+value+'\n'+include(header))
    print(f'PASS full-card batched source/generated identity and binding:{positive} positives/{negative} missing,wrong,late,old-firmware,CPU,hybrid and shader refusals;NO hardware or throughput result')
