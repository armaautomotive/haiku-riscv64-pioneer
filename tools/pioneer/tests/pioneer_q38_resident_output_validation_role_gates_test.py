#!/usr/bin/env python3
"""Exact DEBUG68 role gates;preprocessing ONLY,NO driver/card execution."""
import os
import pathlib
import subprocess

here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
header='#include "'+str(kernel/'PioneerQ38ResidentOutputValidationRole.h')+'"\n'
parents=('PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_OUTPUT_OWNER',
 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER','PIONEER_Q38_PROJECTION_RESULT_STASH','PIONEER_Q38_LEARNED_RMS_NATIVE',
 'PIONEER_Q38_EMBEDDING_NATIVE','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q38_DECODER_RUNTIME','PIONEER_Q38_GDN_MUTABLE_STATE',
 'PIONEER_Q38_FFN_NATIVE_GRAPH','PIONEER_Q38_FFN_PERSISTENT_GRAPH','PIONEER_Q38_FFN_GRAPH_INPUT_ROOT','PIONEER_Q38_FFN_MODEL_INIT',
 'PIONEER_Q38_NORMAL_CLOCK','PIONEER_Q38_SPAN_PRELOAD','PIONEER_Q38_SPAN_FAST','PIONEER_Q38_LOCAL_DELTA_PROBE',
 'TT_PIONEER_EXPERIMENTAL_Q38_BANKS','TT_PIONEER_EXPERIMENTAL_Q38_PANEL','TT_PIONEER_EXPERIMENTAL_Q38_AFFINE',
 'TT_PIONEER_EXPERIMENTAL_Q38_BLOCK','TT_PIONEER_EXPERIMENTAL_Q38_ROWS','TT_PIONEER_EXPERIMENTAL_Q38_COHORT',
 'TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED','TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD',
 'TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH','TT_PIONEER_EXPERIMENTAL_Q38_STREAM','TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR')
flags={k:'1' for k in parents};flags['PIONEER_DRIVER_BUILD_TAG']='0x2026102500010068ULL'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count=0
def check(source,roles,expected):
    global count
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-I'+str(kernel),
                           *['-D'+k+'='+v for k,v in roles.items()],'-'],input=source,text=True,capture_output=True,
                          env=clean,timeout=10)
    assert (result.returncode==0)==expected,(roles,result.stderr)
    count+=1
check(header,flags,True);check(header+header,flags,True)
for key in flags:
    missing=dict(flags);del missing[key];check(header,missing,False)
    check(header+'#undef '+key+'\n'+header,flags,False)
    for value in ('','0','2','-1'):
        check(header,{**flags,key:value},False)
        check(header+'#undef '+key+'\n#define '+key+' '+value+'\n'+header,flags,False)
for tag in ('0x2026102500010067ULL','0x2026102500010065ULL','0x2026102400010052ULL','0','1'):
    check(header,{**flags,'PIONEER_DRIVER_BUILD_TAG':tag},False)
for key in ('PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY','PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT',
 'PIONEER_Q38_RESIDENT_DECODER_OWNER','PIONEER_Q38_RESIDENT_DECODER_NATIVE','PIONEER_Q38_GDN_INPUT_OWNER',
 'PIONEER_Q38_GDN_STATE_INIT_NATIVE','PIONEER_Q38_GDN_RECURRENT_NATIVE','PIONEER_Q38_GDN_GATED_NATIVE',
 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE','PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE','PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE',
 'PIONEER_Q38_RESIDUAL_RMS_NATIVE','PIONEER_Q38_FFN_RESIDUAL_NATIVE','PIONEER_Q38_NEXT_RMS_NATIVE',
 'PIONEER_Q38_NEXT_PROJECTION_NATIVE','PIONEER_Q38_NEXT_CONV_NATIVE','PIONEER_Q38_NEXT_GDN_NATIVE',
 'PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE','PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE','PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE',
 'PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE','PIONEER_Q38_PROJECTION_GRAPH_MEMORY','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_WORK_INFERENCE','__FAST_MATH__','PIONEER_Q38_FFN_HOST_PROFILE','PIONEER_Q38_FFN_SEALED_PROOF',
 'PIONEER_Q38_EMBEDDING_QUALIFICATION','PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE','PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
 'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE','PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE',
 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE','PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE',
 'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE','PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE',
 'PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE','PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE',
 'PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE','PIONEER_Q38_PROJECTION_GRAPH_ROOT','PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT',
 'PIONEER_Q38_PROJECTION_NEXT_ROOT','PIONEER_Q38_OUTPUT_TRANSFER_ROOT','PIONEER_Q38_RESIDUAL_RMS_IMAGE',
 'PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE'):
    for value in ('0','1'):
        check(header,{**flags,key:value},False)
        check(header+'#define '+key+' '+value+'\n'+header,flags,False)
for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for value in ('0','unexpected_alias'):
        check(header,{**flags,key:value},False)
native=(kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix=native.split('#if defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE)',1)[0].replace('#include "tenstorrent_pci_probe.h"','',1)
check(prefix,flags,True)
for value in ('0','1','unexpected_alias'):
    check(prefix,{**flags,'PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK':value},False)
for tag in ('0x2026102500010067ULL','0x2026102500010068ULL'):
    check(prefix,{'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':tag},False)
for value in ('0','1'):
    check(prefix,{'PIONEER_Q38_RESIDENT_OUTPUT_OWNER':value},False)
print('PASS exact DEBUG68 role gates'+str(count)+'/complete parents/old native+DEBUG+Work+CPU+image+compile67+client aliases/late changes/ABI alias+internal-witness spoofing/bare reserved tags;preprocessing ONLY,NO native/card grant')
