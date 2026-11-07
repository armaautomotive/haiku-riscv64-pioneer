#!/usr/bin/env python3
"""QGW8 head plan and freestanding coordinator helpers; no linked ROOT/device."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
flags=['-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
       '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
def check(header,args,valid):
 r=subprocess.run([compiler,'-std=c++17','-x','c++','-fsyntax-only',*args,str(root/header)],capture_output=True)
 assert (r.returncode==0)==valid,r.stderr.decode()[:1000]
for header in ('PioneerQ38HybridHeadLayout.h','PioneerQ38HybridHeadModelPlan.h'):
 check(header,flags,True)
 for flag in flags:
  reduced=[v for v in flags if v!=flag];check(header,reduced,False);check(header,reduced+[flag[:-1]+'0'],False)
 for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_HYBRID_FULL_NATIVE',
  'PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
  'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','__FAST_MATH__'):
  check(header,flags+['-D'+name+'=1'],False)
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-head-plan-',dir='/private/tmp') as name:
 tmp=Path(name);binary=tmp/'plan';obj=tmp/'head.o'
 subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
  '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_q38_hybrid_head_plan_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 crossflags=['-std=c++11','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
  '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections',
  '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax',
  '-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
 subprocess.run([tool+'g++',*crossflags,'-c',str(here/'pioneer_q38_hybrid_head_plan_compile.cpp'),'-o',str(obj)],check=True)
 assert not subprocess.check_output([tool+'nm','-u',str(obj)]).strip()
 dis=subprocess.check_output([tool+'objdump','-d',str(obj)],text=True)
 assert not re.search(r'\b(?:sb|divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|ebreak|s11)\b',dis)
 stacks=(tmp/'head.su').read_text().splitlines();assert stacks
 for line in stacks:
  fields=line.split('\t');assert len(fields)==3 and fields[2]=='static' and int(fields[1])<=2048,line
 size=subprocess.check_output([tool+'size','-A',str(obj)],text=True)
 sections=[line.split() for line in size.splitlines() if line.startswith('.text.')]
 code=sum(int(row[1]) for row in sections)
 print(f'PASS freestanding RV32 helpers: {code} isolated text bytes, no undefined symbols or disallowed opcodes; max individual static frame {max(int(line.split(chr(9))[1]) for line in stacks)}; not linked ROOT or total stack proof')
print('PASS QGW8 head plan roles and synthetic full-vocabulary coverage; NO source/device/TPS')
