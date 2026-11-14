#!/usr/bin/env python3
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv)!=2:raise SystemExit('usage: run_pioneer_q38_ffn_shared_input_reuse_programs_tests.py GENERATED_HEADER')
header=pathlib.Path(sys.argv[1]).resolve()
if not header.is_file():raise SystemExit('candidate program header is missing')
here=pathlib.Path(__file__).resolve().parent
root=here.parent
source=here/'pioneer_q38_ffn_shared_input_reuse_programs_test.cpp'
base=['c++','-std=c++11','-Wall','-Wextra','-Werror','-DPIONEER_Q38_TOKEN_FFN_SHARED_INPUT_REUSE_PROGRAMS=1',
 '-I'+str(header.parent),'-I'+str(root),str(source)]
with tempfile.TemporaryDirectory(prefix='q38-ffn-reuse-programs-') as tmp:
 binary=pathlib.Path(tmp)/'program-data-contract'
 subprocess.run(base+['-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
 for conflict in ('PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS'):
  result=subprocess.run(['c++','-std=c++11','-Werror','-DPIONEER_Q38_TOKEN_FFN_SHARED_INPUT_REUSE_PROGRAMS=1',
   '-D'+conflict+'=1','-I'+str(header.parent),'-I'+str(root),'-c',str(source),'-o',str(pathlib.Path(tmp)/'rejected.o')],
   stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  assert result.returncode!=0,conflict
print('PASS pinned QTP3 candidate bundle spans/word coverage and isolated role rejection; DATA only, not native/model/runtime/TPS qualification')
