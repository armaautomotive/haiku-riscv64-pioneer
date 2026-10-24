#!/usr/bin/env python3
"""Exact native54 execution role,old-byte separation and inert public routing."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
source = (here.parents[2]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX','c++')
count = 0
def preprocess(text, flags, expected):
    global count
    result = subprocess.run([cxx,'-x','c++','-E',*['-D'+k+'='+v for k,v in flags.items()],'-'],input=text,capture_output=True,text=True,env=clean,timeout=5)
    assert (result.returncode == 0) == expected, (flags,result.stderr)
    count += 1
valid = dict.fromkeys(('PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE','PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE','PIONEER_Q38_GDN_RENDEZVOUS_NATIVE',
    'PIONEER_Q38_GDN_GATED_NATIVE','PIONEER_Q38_GDN_RECURRENT_NATIVE','PIONEER_Q38_GDN_STATE_INIT_NATIVE',
    'PIONEER_Q38_GDN_INPUT_OWNER','PIONEER_Q38_RESIDENT_DECODER_NATIVE','PIONEER_Q38_RESIDENT_DECODER_OWNER',
    'PIONEER_Q38_DECODER_RUNTIME','PIONEER_Q38_PROJECTION_EXECUTION_OWNER','PIONEER_Q38_PROJECTION_RESULT_STASH',
    'PIONEER_Q38_LEARNED_RMS_NATIVE','PIONEER_Q38_EMBEDDING_NATIVE','TT_PIONEER_DRIVER_RELOAD'),'1')
valid['PIONEER_DRIVER_BUILD_TAG']='0x2026102400010054ULL'
preprocess(prefix,{},True)
preprocess(prefix,valid,True)
for key in valid:
    for value in (None,'0','2','-1'):
        flags=dict(valid)
        if value is None:
            del flags[key]
        else:
            flags[key]=value
        preprocess(prefix,flags,False)
for tag in ('0x2026102200010038ULL','0x2026102300010043ULL','0x2026102400010048ULL',
            '0x2026102400010049ULL','0x2026102400010050ULL','0x2026102400010051ULL','0x2026102400010052ULL','0x2026102400010053ULL'):
    preprocess(prefix,{**valid,'PIONEER_DRIVER_BUILD_TAG':tag},False)
preprocess(prefix,{'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':valid['PIONEER_DRIVER_BUILD_TAG']},False)
for key in ('PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE','PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE',
    'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
    'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE','PIONEER_Q38_EMBEDDING_QUALIFICATION',
    'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE','PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
    'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_FFN_HOST_PROFILE','PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0','1'):
        preprocess(prefix,{**valid,key:value},False)
for name in ('Embedding','LearnedRms','Projection','ResidentDecoder','GdnInput','GdnRecurrent','GdnRendezvous'):
    client = 'GdnRendezvous' if name == 'GdnRendezvous' else 'GdnRecurrent' if name == 'GdnRecurrent' else 'GdnInput'
    flags={'PIONEER_Q38_'+{'GdnInput':'GDN_INPUT','GdnRecurrent':'GDN_RECURRENT','GdnRendezvous':'GDN_RENDEZVOUS'}[client]+'_VALIDATION_CLIENT':'1',
           'PIONEER_Q38_CPU_VALIDATION':'1','PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    text='#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    preprocess(text,flags,True)
    for value in ('0','1','2','-1'):
        preprocess(text,{**flags,'PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE':value},False)
with tempfile.TemporaryDirectory(prefix='pioneer-gdn-output-gates-') as temporary:
    root=pathlib.Path(temporary)
    for name in ('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image',
                 'ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image'):
        for value in ('1','2','-1','','false','0\n1'):
            result=subprocess.run(['sh',str(directory/('build_qwen38_'+name+'.sh')),str(root/'NO_SDK'),str(root/'NO_OUTPUT')],
                capture_output=True,text=True,env={**clean,'PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE':value},timeout=5)
            assert result.returncode==2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count+=1
    for name,runner in (('build_qwen38_gdn_output_execution_native.py',sys.executable),
                        ('build_qwen38_gdn_rendezvous_validation_client.sh','sh'),
                        ('build_qwen38_gdn_rendezvous_validation_native.py',sys.executable)):
        for key in ('PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_UNKNOWN_ROLE'):
            for value in ('1','2','','false'):
                result=subprocess.run([runner,str(directory/name),str(root/'NO_SDK'),str(root/'NO_OUTPUT')],
                    capture_output=True,text=True,env={**clean,key:value},timeout=5)
                assert result.returncode==2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count+=1
    begin,end='// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_BEGIN','// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin)==source.count(end)==1
    route=source.split(begin,1)[1].split(end,1)[0]
    code='#include <cassert>\nusing status_t=int;constexpr int B_NOT_ALLOWED=-1;int touches=0;\n'
    code+='int Route(unsigned operation){(void)operation;\n'+route+'\n touches++;return 8;}\n'
    code+='int main(){for(unsigned op=0;op<131072;op++)assert(Route(op)==B_NOT_ALLOWED);assert(!touches);}\n'
    path,binary=root/'routes.cpp',root/'routes'
    path.write_text(code)
    subprocess.run([cxx,'-std=c++17','-Wall','-Wextra','-Werror',str(path),'-o',str(binary)],check=True,env=clean)
    subprocess.run([str(binary)],check=True,env=clean)
print(f'PASS native54 exact execution/tag/parent/mixed/reference/builder gates{count}/131072 ACTUAL inert routes;NO DEBUG/Work/numeric input or public selector')
