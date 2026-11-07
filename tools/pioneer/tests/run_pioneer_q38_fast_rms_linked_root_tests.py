#!/usr/bin/env python3
"""Build both QGW5 roots and execute their actual RMS entries in simulation."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,prompt,ffn=map(Path,sys.argv[1:]);assert all(p.is_absolute() for p in (sdk,prompt,ffn))
assert sdk.is_dir() and prompt.is_file() and ffn.is_file()
here=Path(__file__).resolve().parent;root=here.parent;sys.path.insert(0,str(root))
from PioneerQ38HybridProgramPins import ROOT_PINS
candidate_pins=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',128732,'3bb3afa7692ee05b542414d9ec103cf370b22ea1a25c0f8af32f8be820d5fdc9'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',21328,'7598cd9172ec498c83a1fcdc1a193ce99047d7aef3aa8d0cd8559a939d6e27c0'),
 ('bf16.bin',33768,'11500c932d04dfefa8b75958176fd15e2b9e4dbe7e1703d811e7643d6b094982'),
 ('program.elf',216896,'48841d0a9c320d3c449310787751c59b16e8b7c807316001f04d7a9994d5a380'),
)
flags=['--inspect-candidate','--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals',
 '--unroll-quiet','--worker-local-source-guards','--precise-activation','--single-closed-audit',
 '--worker-sealed-admission','--bf16-fpu','--hybrid-bfp']
def run(*args):subprocess.run([str(a) for a in args],check=True)
def check_image(directory,pins):
    for name,size,digest in pins:
        data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
    nm=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
    assert (directory/'program.symbols').read_bytes()==subprocess.check_output([nm,'-n',str(directory/'program.elf')])
with tempfile.TemporaryDirectory(prefix='pioneer-fast-rms-linked-root-',dir='/private/tmp') as temporary:
    temp=Path(temporary);old=temp/'precise';new=temp/'fast';builder=root/'build_qwen38_token_root.py'
    selections=('PIONEER_Q38_FAST_RMS_DIAGNOSTIC','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
                'PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN','PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP')
    source='#include "'+str(root/'PioneerQ38StateMath.h')+'"\n'
    for missing in selections:
        flags_without=['-D'+name+'=1' for name in selections if name!=missing]
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC=1',
                               *flags_without,'-x','c++','-E','-'],input=source,capture_output=True,text=True)
        assert result.returncode and 'Approximate RMS component requires the exact separate hybrid inspection image' in result.stderr,missing
    include='#include "'+str(root/'PioneerQ38StateMath.h')+'"\n'
    late=include+'#define PIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC 1\n'+include
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-x','c++','-fsyntax-only','-'],
                          input=late,capture_output=True,text=True)
    assert result.returncode and 'late approximate RMS component selection' in result.stderr
    removed=include+'#undef PIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC\n'+include
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC=1',
                           *['-D'+name+'=1' for name in selections],'-x','c++','-fsyntax-only','-'],
                          input=removed,capture_output=True,text=True)
    assert result.returncode and 'late approximate RMS component removal' in result.stderr
    refused=subprocess.run([sys.executable,str(builder),str(sdk),str(temp/'invalid'),'--fast-rms'],capture_output=True)
    assert refused.returncode and not (temp/'invalid').exists()
    run(sys.executable,builder,sdk,old,*flags)
    run(sys.executable,builder,sdk,new,*flags,'--fast-rms')
    check_image(old,ROOT_PINS);check_image(new,candidate_pins)
    binary=temp/'test'
    run(os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
        '-fsanitize=address,undefined',here/'pioneer_q38_fast_rms_linked_root_test.cpp',root/'PioneerModelWeightCache.cpp','-o',binary)
    env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
    subprocess.run([str(binary),str(old),str(new),str(prompt),str(ffn)],check=True,env=env)
    with prompt.open('rb') as source:bad=bytearray(source.read(20881))
    assert len(bad)==20880;bad[16]^=1;corrupt=temp/'corrupt.bin';corrupt.write_bytes(bad)
    refused=subprocess.run([str(binary),str(old),str(new),str(corrupt),str(ffn)],capture_output=True,env=env)
    assert refused.returncode==2 and not refused.stdout
print('PASS pinned exact and experimental linked ROOT images; corrupted fixture refused; NO physical card or model inference')
