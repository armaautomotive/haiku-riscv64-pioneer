#!/usr/bin/env python3
"""Build isolated QGWD diagnostic clients; never install or run them."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

args=sys.argv[1:];qgwf='--qgwf-143' in args
if qgwf:
    if args.count('--qgwf-143')!=1 or '--two-token-142' in args:raise SystemExit('ambiguous QGWF client selector')
    args.remove('--qgwf-143');args.append('--two-token-142')
timing='--upload-timing' in args
if timing:
    if args.count('--upload-timing')!=1:raise SystemExit('duplicate upload timing selector')
    args.remove('--upload-timing')
two='--two-token-142' in args
if two:
    if args.count('--two-token-142')!=1:raise SystemExit('duplicate two-token selector')
    args.remove('--two-token-142')
if timing and not two:raise SystemExit('upload timing requires the isolated two-token diagnostic')
if not __debug__ or len(args)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: [--two-token-142] ABS_HAIKU_SDK NEW_ABS_OUTPUT')
sdk,out=map(Path,args);here=Path(__file__).resolve().parent
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
binary=out/('pioneer-q38-qgwf-upload-timing-client143' if qgwf and timing else 'pioneer-q38-qgwf-diagnostic-client143' if qgwf else 'pioneer-q38-qgwd-upload-timing-client142' if timing else 'pioneer-q38-qgwd-diagnostic-client142' if two else 'pioneer-q38-qgwd-diagnostic-client141')
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
if two:cmd.insert(1,'-DPIONEER_Q38_QGWD_TWO_CLIENT_142=1')
if qgwf:cmd.insert(1,'-DPIONEER_Q38_QGWF_CLIENT_143=1')
if timing:cmd.insert(1,'-DPIONEER_Q38_UPLOAD_TIMING_DIAGNOSTIC=1')
subprocess.run(cmd,check=True,cwd=out)
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
symbols=subprocess.check_output([str(tool)+'-nm','-C',str(binary)],text=True)
for name in ('llama_','ggml_','ReferenceProducts','SyntheticIO'):
    if name in symbols:raise RuntimeError('unexpected CPU/reference symbol '+name)
print(('QGWF_CLIENT_143' if qgwf else 'QGWD_CLIENT_'+('142' if two else '141'))+'_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('EXPERIMENTAL BUILD ONLY: NOT INSTALLED, TESTED ON DEVICE, OR QUALIFIED')
