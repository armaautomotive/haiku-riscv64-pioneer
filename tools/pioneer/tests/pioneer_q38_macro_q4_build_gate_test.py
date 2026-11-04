#!/usr/bin/env python3
"""Compile real shader selection gates,without linking or touching hardware."""
import os
import pathlib
import subprocess
import sys

assert len(sys.argv)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
directory=pathlib.Path(sys.argv[1]);command=(directory/'compile-math-argv.txt').read_text().splitlines()
assert command.count('-o')==1 and command.count('-c')==1
at=command.index('-o');del command[at:at+2]
command.remove('-c');command.insert(1,'-fsyntax-only')
assert pathlib.Path(command[-1]).name=='blackhole_qwen38_macro_q4_decode_math.cpp'
assert subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE).returncode==0
def altered(name,value):
 result=[p for p in command if not p.startswith('-D'+name+'=') and p!='-D'+name]
 if value is not None:result.insert(1,'-D'+name+'='+str(value))
 return result
cases=[]
for name in ('PIONEER_Q38_MACRO_Q4_DECODE','PIONEER_Q38_MACRO_Q4_DECODE_IMAGE',
             'PIONEER_Q38_VECTOR_Q4_METADATA','PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE'):
 for value in (None,0):cases.append(altered(name,value))
for name in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_WORK_Q4_NIBBLE','PIONEER_Q38_WORK_Q4_DIRECT_DECODE',
             'PIONEER_Q38_WORK_RAW_UNPACK_CACHE','PIONEER_Q38_Q4_CONSTANT_ISSUE'):
 cases.append(altered(name,1))
cases.append([command[0],'-ffast-math',*command[1:]])
for i,case in enumerate(cases):
 result=subprocess.run(case,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert result.returncode!=0 and b'#error' in result.stderr,(i,result.stdout,result.stderr)
print('PASS_MACRO_Q4_REAL_SHADER_BUILD_GATES positive1 negative'+str(len(cases))+';NO_LINK_OR_HARDWARE_PERMISSION',flush=True)
