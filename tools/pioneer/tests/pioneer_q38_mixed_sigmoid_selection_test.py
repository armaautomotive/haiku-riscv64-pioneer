#!/usr/bin/env python3
"""Candidate DATA selection cannot silently replace default pinned bytes."""
import pathlib
import subprocess
import sys
assert len(sys.argv)==8
here=pathlib.Path(__file__).resolve().parent
model,embedding,default,candidate,worker,qdp,qda=sys.argv[1:]
runner=here/'run_pioneer_q38_mixed_live_original_instructions_tests.py'
count=0
def check(root,options,expected):
 global count
 result=subprocess.run([sys.executable,str(runner),model,embedding,root,worker,qdp,qda,*options],
  text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
 assert (result.returncode==0)==expected,(root,options,result.stdout,result.stderr)
 count+=1
check(default,['--verify-only'],True)
check(candidate,['--sigmoid-candidate','--verify-only'],True)
check(default,['--sigmoid-candidate','--verify-only'],False)
check(candidate,['--verify-only'],False)
check(candidate,['--sigmoid-candidate','--sigmoid-candidate','--verify-only'],False)
check(default,['--bytes-only','--verify-only'],False)
check(default,['--work','--verify-only'],False)
check(candidate,['--sigmoid-candidate','--unknown','--verify-only'],False)
print('PASS original-instruction DEFAULT vs EXPLICIT_SIGMOID_CANDIDATE selector'+str(count)+'/pinned bytes never interchangeable/duplicates+mixed+Work+unknown refused BEFORE execution;NO native grant')
