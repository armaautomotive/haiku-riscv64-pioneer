#!/usr/bin/env python3
"""Compile real protocol/late-binding checks only; no device or private data."""
import os
from pathlib import Path
import subprocess
import tempfile
import sys
configured=sys.argv[1:]==['--configured'];assert not sys.argv[1:] or configured
variant=114 if configured else 113;tag='2026110600010114' if configured else '2026110600010113'
assert __debug__
root=Path(__file__).resolve().parents[3]
header=root/'src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h'
base=['-DPIONEER_NATIVE_FACTORED_Q4_COMPARE=1','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1','-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=1']
if configured:base+=['-DPIONEER_NATIVE_FACTORED_Q4_CONFIGURED=1']
include='#define B_DEVICE_OP_CODES_END 10000\n#include "'+str(header)+'"\n'
positive=include+f'static_assert(PioneerNativeCompactQ4StreamVariant::Role=={variant} && PioneerNativeCompactQ4StreamVariant::Tag==0x{tag}ULL && PioneerNativeCompactQ4StreamVariant::Opcode=={0x8a00 if configured else 0x8900} && PioneerNativeCompactQ4StreamVariant::ImageBytes=={12108 if configured else 11788} && sizeof(PioneerNativeCompactQ4StreamRequest)==456,"exact{variant}");\n'
kernel=['-D_KERNEL_MODE=1','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC=1','-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG=0x'+tag+'ULL']
with tempfile.TemporaryDirectory(prefix='pioneer-manual-role-',dir='/private/tmp') as directory:
 source=Path(directory)/'role.cpp'
 def run(text,flags,okay):
  source.write_text(text)
  r=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only',*flags,str(source)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
  assert (r.returncode==0)==okay,(flags,r.stderr.decode())
 run(positive,base,True);run(positive,base+kernel,True)
 failures=0
 for macro in ('PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST','PIONEER_NATIVE_FACTORED_Q4_COMPARE','PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING',*(['PIONEER_NATIVE_FACTORED_Q4_CONFIGURED'] if configured else [])):
  run(include+'#undef '+macro+'\n#include "'+str(header)+'"\n',base,False);failures+=1
 legacy=[p for p in base if 'MANUAL_LAST' not in p and 'CONFIGURED' not in p]
 run(include+'#define PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST 1\n#include "'+str(header)+'"\n',legacy,False);failures+=1
 if configured:
  run(include+'#define PIONEER_NATIVE_FACTORED_Q4_CONFIGURED 1\n#include "'+str(header)+'"\n',[p for p in base if 'CONFIGURED' not in p],False);failures+=1
 for tag in ('2026110600010111','2026110600010112','2026110600010107',*(['2026110600010113'] if configured else [])):
  run(positive,base+kernel+['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x'+tag+'ULL'],False);failures+=1
 for role in ('PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_NATIVE_FACTORED_Q4_TRACE',*(['PIONEER_Q38_FACTORED_CONFIGURED_COMPUTE'] if configured else [])):
  for value in (0,1):run(positive,base+['-D'+role+'='+str(value)],False);failures+=1
 for flag in ('-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=0','-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=2'):
  run(positive,[*legacy,flag],False);failures+=1
 if configured:
  for value in (0,2):
   run(positive,[p for p in base if 'CONFIGURED' not in p]+['-DPIONEER_NATIVE_FACTORED_Q4_CONFIGURED='+str(value)],False);failures+=1
 assert failures==(26 if configured else 19)
print(f'PASS_COMPARE{variant}_PROTOCOL positive2_refusals{failures} exactTagOpcodeImageABI_lateSelectionRemoval_oldTags_WorkReferencesTraceExcluded;NO_HARDWARE')
