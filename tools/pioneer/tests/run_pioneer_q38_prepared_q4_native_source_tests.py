#!/usr/bin/env python3
"""Compile/run actual native binding/startup over synthetic cache/bank endpoints."""
import os,pathlib,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(sys.argv[1])
assert out.is_absolute() and not out.exists();out.mkdir(mode=0o700)
base=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations',
 '-ffp-contract=off','-fsanitize=address,undefined','-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1',
 '-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1','-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1']
for name,flags in (
 ('baseline',[]),
 ('scattered-cache',['-DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1']),
 ('scattered-l1-authoritative',['-DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1',
  '-DPIONEER_Q38_GDN_L1_NO_GDDR_WRITEBACK_EXPERIMENT=1'])):
 binary=out/name
 subprocess.run([*base,*flags,str(here/'pioneer_q38_prepared_q4_native_source_test.cpp'),
  str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
