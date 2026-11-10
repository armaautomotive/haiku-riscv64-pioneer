#!/usr/bin/env python3
"""Build the isolated one-token QGWD diagnostic client; never run it."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

if not __debug__ or len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_HAIKU_SDK NEW_ABS_OUTPUT')
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
if not sdk.is_absolute() or not sdk.is_dir() or not out.is_absolute() or out.exists() or out.is_symlink():
    raise SystemExit('invalid path')
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_q38_qgw8_cache_preflight.py'),str(sdk),str(out)]
    built=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
cmd=list(map(str,built['cmd']));old=str(here/'pioneer_q38_qgw8_cache_preflight.cpp');old_binary=str(built['binary'])
if cmd.count(old)!=1 or cmd.count(old_binary)!=1:raise RuntimeError('unexpected preflight build command')
binary=out/'pioneer-q38-qgwd-diagnostic-client141'
cmd[cmd.index(old)]=str(here/'pioneer_q38_qgw8_full_diagnostic.cpp');cmd[cmd.index(old_binary)]=str(binary)
names=('PIONEER_Q38_QGWD_CLIENT_141','PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION',
 'PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
 'PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION','PIONEER_Q38_QGW8_USER_SESSION_INSPECTION',
 'PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC','PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC',
 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION',
 'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
 'PIONEER_Q38_DENSE_Q8_UPLOAD_INSPECTION','PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION')
cmd[1:1]=['-D'+name+'=1' for name in names]
subprocess.run(cmd,check=True,cwd=out)
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
symbols=subprocess.check_output([str(tool)+'-nm','-C',str(binary)],text=True)
for name in ('llama_','ggml_','ReferenceProducts','SyntheticIO'):
    if name in symbols:raise RuntimeError('unexpected CPU/reference symbol '+name)
print('QGWD_CLIENT_141_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('EXPERIMENTAL BUILD ONLY: NOT INSTALLED, TESTED ON DEVICE, OR QUALIFIED')
