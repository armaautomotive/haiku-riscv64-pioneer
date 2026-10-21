#!/usr/bin/env python3
"""Prepared compact Q4 has no native/source capability and is offline-only."""
import os, pathlib, subprocess, tempfile
directory = pathlib.Path(__file__).resolve().parent.parent
base = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
required = {f'PIONEER_Q38_{name}':'1' for name in (
    'PREPARED_Q4_BLOCK','BLOCK_SCALE_REUSE','BLOCK_AFFINE_DST',
    'INLINE_CONFIG','WORK_DEFER_AFFINE','NATIVE_OPERAND_STORE','WORK_ZERO_MINIMUM',
    'WORK_INPUT_SUM_CACHE','QUEUED_MATRIX_ENTRY','WORK_SKIP_DOT_CHECK',
    'REPLAY_OPERAND','CACHED_AFFINE','STATIC_VECTOR','PIPELINED_MATRIX')}
required.update({f'PIONEER_Q38_{name}':'0' for name in ('NATIVE_Q8','Q4_GROUP32','PREPARED_Q4_AFFINE','PREDECODED_Q4','PREDECODED_Q4_STICKY')})
cases=[]
for key,correct in required.items():
    for wrong in ('0','1','01','invalid'):
        if wrong!=correct and not(key=='PIONEER_Q38_PREPARED_Q4_BLOCK' and wrong=='0'):
            cases.append({**required,key:wrong})
with tempfile.TemporaryDirectory(prefix='pioneer-q38-prepared-block-gate-') as name:
    root=pathlib.Path(name);sdk=root/'MUST_NOT_OPEN_SDK';libs=root/'MUST_NOT_READ_LIBS'
    count=0
    for flags in cases:
        output=root/('MUST_NOT_CREATE_'+str(count))
        r=subprocess.run(['sh',str(directory/'build_qwen38_int_sfpu_persistent_image.sh'),str(sdk),str(output)],
                         env={**base,**flags},capture_output=True,text=True)
        assert r.returncode==2 and str(sdk) not in r.stderr and not output.exists(),(flags,r.stderr)
        count+=1
    for script in ('build_qwen38_banks_driver.sh','build_qwen38_inference.sh'):
        for value in ('1','01','invalid','-1'):
            for q8 in ('0','1'):
                output=root/('MUST_NOT_CREATE_'+str(count))
                args=[str(sdk),str(output)] if script.endswith('banks_driver.sh') else [str(sdk),str(libs),str(output)]
                r=subprocess.run(['sh',str(directory/script),*args],env={**base,**required,
                    'PIONEER_Q38_PREPARED_Q4_BLOCK':value,'PIONEER_Q38_NATIVE_Q8_LAYOUT':q8},capture_output=True,text=True)
                assert r.returncode==2 and str(sdk) not in r.stderr and not output.exists(),(script,value,r.stderr)
                count+=1
print(f'PASS {count} prepared-block incomplete image-chain/native/model/source refusals BEFORE SDK/output;NO device access')
