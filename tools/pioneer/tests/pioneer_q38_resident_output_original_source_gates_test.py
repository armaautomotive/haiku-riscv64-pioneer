#!/usr/bin/env python3
"""Actual original-model observer TU separation;NO model/device execution."""
import hashlib
import os
import pathlib
import subprocess
import sys

if len(sys.argv) not in (1,2):
    raise SystemExit(2)
here=pathlib.Path(__file__).resolve().parent
source=here/'pioneer_q38_decoder_original_source.cpp'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count=0
def check(flags,expected,diagnostic=None):
    global count
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E',*flags,str(source)],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,env=clean,timeout=30)
    assert (result.returncode==0)==expected,(flags,result.stderr)
    if diagnostic:
        assert diagnostic in result.stderr,(flags,result.stderr)
    count+=1
check([],True)
for name in ('PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_OUTPUT_OWNER',
             'PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE','PIONEER_Q38_RESIDENT_OPERATOR_IMAGE',
             'PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE','PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE',
             'PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE','PIONEER_Q38_EMBEDDING_NATIVE',
             'PIONEER_Q38_LEARNED_RMS_NATIVE','PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
             'PIONEER_Q38_RESIDENT_DECODER_NATIVE','PIONEER_Q38_WORK_INFERENCE','_KERNEL_MODE','__FAST_MATH__'):
    for value in ('0','1'):
        check(['-D'+name+'='+value],False,'DEBUG68' if name=='PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE' else None)
print('PASS ACTUAL original-source CPU TU gates'+str(count)+'/defined-zero native68+owner+image+Work+kernel+fast-math refusal;preprocessing ONLY')
if len(sys.argv)==2:
    driver=pathlib.Path(sys.argv[1])
    assert driver.is_file() and not driver.is_symlink()
    assert hashlib.sha256(driver.read_bytes()).hexdigest()=='58064edee490bb5335fa135122a8d89588987a56c0451d751bccf9ebbe84ff3d'
    nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    symbols=subprocess.check_output([nm,'--defined-only',str(driver)],text=True)
    for forbidden in ('DecoderOriginalSource','OriginalSource','CheckResidentOutput','ReadReferencePacked','ResidentOutputImportLearnedBytes'):
        assert forbidden not in symbols,forbidden
    print('PASS existing exact production DEBUG68 driver58064ed contains NONE of the CPU-original observer/test-byte-import symbols;NO install or execution grant')
