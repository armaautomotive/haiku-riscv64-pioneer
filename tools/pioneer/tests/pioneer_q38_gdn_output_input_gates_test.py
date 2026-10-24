#!/usr/bin/env python3
"""Actual QOI1 guards/builders/frame gate;no device or image installation."""
import os
import pathlib
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parent
directory=here.parent
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count=0
def pre(text,flags,expected):
    global count
    result=subprocess.run([os.environ.get('CXX','c++'),'-E','-x','c++',*['-D'+k+'='+v for k,v in flags.items()],'-'],
        input=text,capture_output=True,text=True,env=clean,timeout=5)
    assert (result.returncode==0)==expected,(flags,result.stderr)
    count+=1
feature='PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE'
helper=(directory/'blackhole_qwen38_gdn_output_input.cpp').read_text().split('#define PIONEER_ATTENTION_FREESTANDING',1)[0]
parent=(directory/'blackhole_qwen38_persistent.cpp').read_text().split('#define PIONEER_Q38_SHARED_ROWS',1)[0]
valid={feature:'1','PIONEER_Q38_PROJECTION_PERSISTENT':'1'}
for text,flags in ((helper,valid),(parent,{**valid,'PIONEER_Q38_FFN_PERSISTENT':'1'})):
    pre(text,flags,True)
    for key in flags:
        for value in ('0','2','-1'):
            pre(text,{**flags,key:value},False)
    for key in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE',
        'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE','PIONEER_Q38_GDN_RENDEZVOUS_NATIVE','PIONEER_Q38_GDN_GATED_NATIVE',
        'PIONEER_Q38_GDN_RECURRENT_NATIVE','PIONEER_Q38_GDN_STATE_INIT_NATIVE','PIONEER_Q38_GDN_INPUT_OWNER',
        'PIONEER_Q38_RESIDENT_DECODER_NATIVE','PIONEER_Q38_RESIDENT_DECODER_OWNER','PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
        'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE','PIONEER_Q38_EMBEDDING_NATIVE','PIONEER_Q38_LEARNED_RMS_NATIVE'):
        for value in ('0','1'):
            pre(text,{**flags,key:value},False)
pre(helper,{},False)
pre(parent,{},True) # macro-OFF original entry remains original,not an op6 image
pre(parent,valid,False) # requires the real persistent family too
for name in ('Embedding','LearnedRms','Projection','ResidentDecoder','GdnInput','GdnRecurrent','GdnRendezvous'):
    key='GDN_RENDEZVOUS' if name=='GdnRendezvous' else 'GDN_RECURRENT' if name=='GdnRecurrent' else 'GDN_INPUT'
    flags={'PIONEER_Q38_'+key+'_VALIDATION_CLIENT':'1','PIONEER_Q38_CPU_VALIDATION':'1','PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    text='#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    pre(text,flags,True)
    for value in ('0','1','2'):
        pre(text,{**flags,feature:value},False)
with tempfile.TemporaryDirectory(prefix='pioneer-gdn-output-input-gates-') as temporary:
    root=pathlib.Path(temporary)
    for name in ('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image',
                 'ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image'):
        for value in ('1','2','-1','','false','0\n1'):
            result=subprocess.run(['sh',str(directory/('build_qwen38_'+name+'.sh')),str(root/'NO_SDK'),str(root/'NO_OUTPUT')],
                capture_output=True,text=True,env={**clean,feature:value},timeout=5)
            assert result.returncode==2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists(),(name,value,result.stderr)
            count+=1
    for name,runner in (('build_qwen38_gdn_output_input_image.sh','sh'),('build_qwen38_gdn_output_input_native.py',sys.executable)):
        for key in (feature,'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','PIONEER_UNKNOWN_ROLE'):
            for value in ('1','2','','false','00'):
                result=subprocess.run([runner,str(directory/name),str(root/'NO_SDK'),str(root/'NO_OUTPUT')],
                    capture_output=True,text=True,env={**clean,key:value},timeout=5)
                assert result.returncode==2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count+=1
    commands=[line for line in (directory/'build_qwen38_gdn_output_input_image.sh').read_text().splitlines() if line.startswith('awk ')]
    assert len(commands)==1
    for size in (0,512,2048,2049,8192):
        usage=root/'activation.su';usage.write_text('actual:1:1:new_input\t'+str(size)+'\tstatic\n')
        result=subprocess.run(['sh','-c',commands[0].replace('"$2/activation.su"','"'+str(usage)+'"')],capture_output=True,env=clean)
        assert (result.returncode==0)==(size<=2048)
        count+=1
print(f'PASS QOI1 image/helper/parent/mixed/reference/early builder/frame gates{count};NO native role/CPU-reference/Work mix or device IO')
