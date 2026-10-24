#!/usr/bin/env python3
"""Actual QOG1 source guard/strict builders/frame gate;NO board access."""
import os
import pathlib
import subprocess
import sys
import tempfile
here=pathlib.Path(__file__).resolve().parent
directory=here.parent
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
prefix=(directory/'blackhole_qwen38_gdn_output_gather.cpp').read_text().split('#define PIONEER_ATTENTION_FREESTANDING',1)[0]
count=0
def pre(flags,expected):
    global count
    result=subprocess.run([os.environ.get('CXX','c++'),'-E','-x','c++',*['-D'+k+'='+v for k,v in flags.items()],'-'],
        input=prefix,capture_output=True,text=True,env=clean,timeout=5)
    assert (result.returncode==0)==expected,(flags,result.stderr)
    count+=1
pre({},True)
for key in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE',
    'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE','PIONEER_Q38_GDN_RENDEZVOUS_NATIVE','PIONEER_Q38_GDN_GATED_NATIVE',
    'PIONEER_Q38_GDN_RECURRENT_NATIVE','PIONEER_Q38_GDN_STATE_INIT_NATIVE','PIONEER_Q38_GDN_INPUT_OWNER',
    'PIONEER_Q38_RESIDENT_DECODER_NATIVE','PIONEER_Q38_RESIDENT_DECODER_OWNER','PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
    'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE','PIONEER_Q38_EMBEDDING_NATIVE','PIONEER_Q38_LEARNED_RMS_NATIVE'):
    for value in ('0','1'):
        pre({key:value},False)
with tempfile.TemporaryDirectory(prefix='pioneer-gdn-output-gather-gates-') as temporary:
    root=pathlib.Path(temporary)
    for name,runner in (('build_qwen38_gdn_output_gather_image.sh','sh'),('build_qwen38_gdn_output_gather_native.py',sys.executable)):
        for key in ('PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE','PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE',
                    'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE','PIONEER_UNKNOWN_ROLE'):
            for value in ('1','2','-1','','false','00'):
                result=subprocess.run([runner,str(directory/name),str(root/'NO_SDK'),str(root/'NO_OUTPUT')],
                    capture_output=True,text=True,env={**clean,key:value},timeout=5)
                assert result.returncode==2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count+=1
    commands=[line for line in (directory/'build_qwen38_gdn_output_gather_image.sh').read_text().splitlines() if line.startswith('awk ')]
    assert len(commands)==1
    for size in (0,512,2048,2049,8192):
        usage=root/'program.su';usage.write_text('actual:1:1:gather\t'+str(size)+'\tstatic\n')
        result=subprocess.run(['sh','-c',commands[0].replace('"$2/program.su"','"'+str(usage)+'"')],capture_output=True,env=clean)
        assert (result.returncode==0)==(size<=2048)
        count+=1
print(f'PASS QOG1 actual image/native-reference-Work separation/strict early builder/frame gates{count};NO device or registry grant')
