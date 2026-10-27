#!/usr/bin/env python3
"""Independent recurrent CPU oracle/source;never native or accelerator code."""
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) in (2,3) and (len(sys.argv)==2 or sys.argv[2]=='--client-preflight')
client=len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-original-source-',dir='/private/tmp') as temp:
 binary=pathlib.Path(temp)/'test'
 flags=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
 if client:
  objects=[]
  for name,path,selection in [('client',here/'pioneer_q38_recurrent_block_validation_original_preflight_test.cpp',
   ['-DPIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT=1','-DPIONEER_Q38_CPU_VALIDATION=1']),
   ('source',here.parent/'pioneer_q38_recurrent_block_original_source.cpp',[])]:
   obj=pathlib.Path(temp)/(name+'.o');subprocess.run([*flags,*selection,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
  subprocess.run([*flags,*map(str,objects),'-o',str(binary)],check=True)
 else:
  subprocess.run([*flags,str(here/'pioneer_q38_recurrent_block_original_source_test.cpp'),
   str(here.parent/'pioneer_q38_recurrent_block_original_source.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),sys.argv[1]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
