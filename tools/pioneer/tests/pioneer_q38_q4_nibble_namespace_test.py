#!/usr/bin/env python3
"""Actual registry header: reject incomplete/mixed roles, with NO SDK or IO."""
import json,os,pathlib,shlex,subprocess
root=pathlib.Path(__file__).resolve().parents[3]
header=root/'src/add-ons/kernel/drivers/misc/PioneerQ38DispatchImages.h'
source='#include '+json.dumps(str(header))+'\n'
base=shlex.split(os.environ.get('CXX','c++'))
assert '-DPIONEER_Q38_Q4_NIBBLE_QUALIFICATION' in base
def compile_flags(flags):
 return subprocess.run([*flags,'-std=c++17','-fsyntax-only','-x','c++','-'],input=source,capture_output=True,text=True)
r=compile_flags(base);assert r.returncode==0,r.stderr
cases=[]
for flag in ('PIONEER_Q38_PEER_PHASE_QUALIFICATION','PIONEER_Q38_COMMAND_REUSE_QUALIFICATION',
             'PIONEER_Q38_TILE_PREFETCH_QUALIFICATION','PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION',
             'PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION','PIONEER_Q38_BATCH4_FETCH_QUALIFICATION',
             'PIONEER_Q38_HOST_PHASE_PROFILE'):
 cases.append([*base,'-D'+flag])
for flag in ('PIONEER_Q38_SENTINEL_POLL_QUALIFICATION','PIONEER_Q38_DIRECT_SRCB_QUALIFICATION'):
 cases.append([value for value in base if value!='-D'+flag])
for flags in cases:
 r=compile_flags(flags)
 assert r.returncode!=0 and 'Q4 nibble OFFLINE registry requires' in r.stderr,(flags,r.stderr)
print(f'PASS actual Q4 nibble registry namespace:one complete role+{len(cases)} incomplete/mixed refusals;NO SDK/output/device/native capability')
