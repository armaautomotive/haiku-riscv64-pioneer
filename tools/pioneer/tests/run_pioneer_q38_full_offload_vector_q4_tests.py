#!/usr/bin/env python3
"""Authenticate new compute DATA and exact native/USER role boundaries."""
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
for directory,producer in ((root,'build_qwen38_full_offload_batched_root.py'),
                           (worker,'build_qwen38_vector_q4_metadata_worker.py')):
    authenticated(directory,pins(here.parent/producer,'PINS'))
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc';role=kernel/'PioneerQ38FullOffloadRole.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()))
assert len(required)==23
base={name:'1' for name in required}
base.update(PIONEER_DRIVER_BUILD_TAG='0x2026110400010092ULL',PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',
            PIONEER_Q38_FULL_OFFLOAD_METRICS='1',PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4='1')
definitions=lambda values:''.join('#define '+name+' '+value+'\n' for name,value in sorted(values.items()))
include=lambda path:'#include "'+str(path)+'"\n'
header=here.parent/'PioneerQ38FullOffloadVectorQ4Programs.h'
assert hashlib.sha256(header.read_bytes()).hexdigest()=='3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389'
with tempfile.TemporaryDirectory(prefix='pioneer-full-vector-q4-tests-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);generated=out/'programs.h'
    subprocess.run([sys.executable,str(here.parent/'generate_qwen38_full_offload_vector_q4_programs.py'),
                    str(worker),str(generated)],check=True)
    assert generated.read_bytes()==header.read_bytes()
    (out/'full_offload_vector_q4_role_defines.inc').write_text(definitions(base))
    binary=out/'data';common=[os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror']
    subprocess.run([*common,'-O1','-g','-fsanitize=address,undefined','-I'+str(out),
                    str(here/'pioneer_q38_full_offload_vector_q4_data_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(root),str(worker)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
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
    syntax(base,True,include(selected)+include(selected)+'static_assert(PioneerQ38TokenPrograms::VectorQ4Metadata==1);\n')
    for key in sorted(base):
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False)
        syntax(base,False,include(header)+'#undef '+key+'\n'+include(header))
    for tag in ('0x2026110400010088ULL','0x2026110400010089ULL','0x2026110400010090ULL','0x2026110400010091ULL'):
        syntax({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False)
    for key in ('PIONEER_Q38_FULL_OFFLOAD_RESIDENT','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
                'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE','PIONEER_Q38_VECTOR_Q4_METADATA',
                'PIONEER_Q38_TOKEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS',
                'PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS','__FAST_MATH__'):
        for value in ('0','1'):
            syntax({**base,key:value},False)
            syntax(base,False,include(header)+'#define '+key+' '+value+'\n'+include(header))
    for old in ('PioneerQ38FullOffloadPrograms.h','PioneerQ38FullOffloadBatchedPrograms.h'):
        syntax(base,False,include(here.parent/old)+include(header))
        # The original tag88 header is intentionally unbound DATA only.
        # Merely declaring that unused namespace is not execution. Require
        # rejection when actual selected/native binding is rechecked too.
        syntax(base,False,include(header)+include(here.parent/old)+include(selected))
    user={'PIONEER_Q38_FULL_OFFLOAD_CLIENT':'1','PIONEER_Q38_FULL_OFFLOAD_BATCHED':'1',
          'PIONEER_Q38_FULL_OFFLOAD_METRICS':'1','PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4':'1',
          'B_DEVICE_OP_CODES_END':'1000'}
    protocol=kernel/'PioneerQ38FullOffloadProtocol.h'
    body='#include <cstdint>\n'+include(protocol)+'static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110400010092));\n'
    syntax(user,True,body)
    for key in ('PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4','PIONEER_Q38_FULL_OFFLOAD_BATCHED','PIONEER_Q38_FULL_OFFLOAD_METRICS'):
        for value in (None,'0','2'):
            bad=user.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            syntax(bad,False,body)
    print(f'PASS full-card vector Q4 source/generated DATA and native/USER binding:{positive} positives/{negative} missing,wrong,late,old-package,CPU,hybrid and shader refusals;NO hardware or measured throughput',flush=True)
