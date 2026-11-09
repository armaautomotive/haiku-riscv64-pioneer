#!/usr/bin/env python3
"""Build unqualified tag128, tag129 or tag131 QGW8 client; never install or run."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

if not __debug__ or len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3] not in ('--hidden-129','--trace-131')) \
 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute Haiku build, new output, optional --hidden-129 or --trace-131 required')
hidden=len(sys.argv)==4 and sys.argv[3]=='--hidden-129'
trace=len(sys.argv)==4 and sys.argv[3]=='--trace-131'
sdk,out=map(Path,sys.argv[1:3]);here=Path(__file__).resolve().parent
if not sdk.is_absolute() or not sdk.is_dir() or not out.is_absolute() or out.exists():raise SystemExit('invalid path')
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_q38_qgw8_cache_preflight.py'),str(sdk),str(out)]
    built=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
cmd=list(map(str,built['cmd']))
old=str(here/'pioneer_q38_qgw8_cache_preflight.cpp');old_binary=str(built['binary'])
assert cmd.count(old)==1 and cmd.count(old_binary)==1
binary=out/('pioneer-q38-qgw8-trace-candidate131' if trace else 'pioneer-q38-qgw8-hidden-candidate129' if hidden else 'pioneer-q38-qgw8-full-candidate128')
cmd[cmd.index(old)]=str(here/'pioneer_q38_qgw8_full_diagnostic.cpp')
cmd[cmd.index(old_binary)]=str(binary)
names=('PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128','PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION',
 'PIONEER_Q38_QGW8_USER_SESSION_INSPECTION','PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC',
 'PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC','PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
 'PIONEER_Q38_QGW8_SPAN_INSPECTION','PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_HYBRID_TRACKED_UPLOAD')
if hidden:names+=('PIONEER_Q38_QGW8_HIDDEN_CLIENT_129',)
if trace:names+=('PIONEER_Q38_QGW8_TRACE_CLIENT_131',)
cmd[1:1]=['-D'+name+'=1' for name in names]
subprocess.run(cmd,check=True,cwd=out)
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
symbols=subprocess.check_output([str(tool)+'-nm','-C',str(binary)],text=True)
for name in ('llama_','ggml_','ReferenceProducts','SyntheticIO'):
    if name in symbols:raise RuntimeError('unexpected CPU/reference symbol '+name)
print('QGW8_TRACE_CLIENT_131_SHA256' if trace else 'QGW8_HIDDEN_CLIENT_129_SHA256' if hidden else 'QGW8_CLIENT_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('EXPERIMENTAL BUILD ONLY: NOT INSTALLED, TESTED ON DEVICE, OR QUALIFIED')
