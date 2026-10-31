#!/usr/bin/env python3
"""Distinct ROOT BUILD containment and exact emitted leaf; no accelerator IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
old,new=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(path.is_absolute() for path in (old,new))
OLD=(('boot.bin','32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
     ('text.bin','6cb8ae7d5b03525ec233aa22b2230c815cab311771c532e82ba7d7bd7ced9dff'),
     ('terminal.bin','6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
     ('conv.bin','72c325aaaf632e3fc9a5f7c1a0286387982021eec171705f9027c8b16868a09c'),
     ('program.elf','430b89b2fb734bfb66bbe96cc1e8581fcd6476d263a2578b8e869936df4bc1c7'))
NEW=(('boot.bin',OLD[0][1]),('text.bin','39ca2743b8aaa166d7ecc9205b516558fcfac93449f5d4526ad065b39da39205'),
     ('terminal.bin',OLD[2][1]),('conv.bin','d9576bce48afad75fe54a337a865437b07263362f43d8d16688835b56e8f7861'),
     ('program.elf','5585e908bf3326c5410d0998c81ed161c04ab754412cc60891b8eb1f197a58fe'))
for root,pins in ((old,OLD),(new,NEW)):
    for name,digest in pins:assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,(root,name)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
symbols=subprocess.check_output([tool+'nm','-nC',str(new/'program.elf')],text=True)
entries=re.findall(r'^([0-9a-f]+) [tTW] GlobalChain::PublishFrontier\(unsigned int\)$',symbols,re.M)
assert len(entries)==1
old_symbols=subprocess.check_output([tool+'nm','-nC',str(old/'program.elf')],text=True)
assert 'PublishFrontier(' not in old_symbols
role=here.parent/'PioneerQ38TokenFrontierImageRole.h'
base=dict.fromkeys(('PIONEER_Q38_TOKEN_FRONTIER_IMAGE','__riscv','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
                   'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK','PIONEER_Q38_TOKEN_CANCEL_BOUNDARY','PIONEER_Q38_JOB_BOOT_FOLD_IMAGE',
                   'PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION'),'1')
positive=negative=0
def include():return '#include "'+str(role)+'"\n'
def check(values,okay,tail='static_assert(PioneerQ38TokenFrontierImageProfile==1,"selected");\n'):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))+include()+tail
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,tail,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
check(base,True,include())
check({},True,'static_assert(PioneerQ38TokenFrontierImageProfile==0,"default");\n')
for name in base:
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[name]
        else:bad[name]=value
        check(bad,False)
    check(base,False,'#undef '+name+'\n#define '+name+' 0\n'+include())
for name in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','__FAST_MATH__'):
    for value in ('0','1'):check({**base,name:value},False)
    check(base,False,'#define '+name+' 0\n'+include())
check({},False,''.join('#define '+key+' '+value+'\n' for key,value in sorted(base.items()))+include())
check(base,False,'#undef PIONEER_Q38_TOKEN_FRONTIER_IMAGE\n'+include())
full=['--inspect-candidate','--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals','--unroll-quiet',
      '--worker-local-source-guards','--precise-activation','--single-closed-audit','--worker-sealed-admission',
      '--pushed-completion','--unroll-packet-seals','--unroll-packet-seals-eight','--fold-job-boot','--token-frontier']
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-qualification-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);refused=0
    bads=[[value for value in full if value!=missing] for missing in full if missing!='--token-frontier']
    bads += [['--token-frontier'],full+['--token-frontier'],full+['--unknown-frontier']]
    for options in bads:
        target=out/('denied-'+str(refused));before=sorted(out.iterdir())
        result=subprocess.run([sys.executable,str(here.parent/'build_qwen38_token_root.py'),
                               str(out/'MUST_NOT_READ_SDK'),str(target),*options],capture_output=True,text=True)
        assert result.returncode and 'AssertionError' in result.stderr and not target.exists() and before==sorted(out.iterdir())
        refused+=1
    flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
    binary=out/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_token_frontier_instruction_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(new),entries[0]],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print(f'PASS distinct frontier ROOT BUILD/{positive} positive/{negative} missing+zero+two+late+foreign refusals/{refused} pre-output CLI refusals/old full430b control unchanged/new complete5585 authenticated;NO actual publication/full-model/native/readout/physical/timing/Work/TPS grant')
