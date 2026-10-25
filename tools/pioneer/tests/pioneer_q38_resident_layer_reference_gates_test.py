#!/usr/bin/env python3
"""Separate complete-layer CPU oracle: no device, native or Work capability."""
import os
import pathlib
import re
import subprocess

here = pathlib.Path(__file__).resolve().parent
header = here.parent/'PioneerQ38ResidentLayerValidationReference.h'
source = header.read_text()
prefix = source.split('#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE_H', 1)[0]
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
valid = {'PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE':'1', 'PIONEER_Q38_CPU_VALIDATION':'1'}
count = 0


def check(flags, expected, text=prefix):
    global count
    result = subprocess.run([os.environ.get('CXX','c++'), '-x', 'c++', '-std=c++17', '-E',
                             *['-D'+k+'='+v for k,v in flags.items()], '-'], input=text, text=True,
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, env=clean, timeout=30)
    assert (result.returncode == 0) == expected, (flags,result.stderr)
    count += 1


check(valid, True)
check({}, False)
for name in valid:
    for value in (None,'0','2','-1'):
        flags = dict(valid)
        if value is None:
            del flags[name]
        else:
            flags[name] = value
        check(flags, False)
for name in sorted(set(re.findall(r'defined\(([^)]+)\)',prefix))-set(valid)):
    for value in ('0','1'):
        check({**valid,name:value}, False)
full = {**valid,'PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE':'1','PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT':'1'}
include = '#include "'+str(header)+'"\n'
check(full, True, include)
for name in full:
    for value in (None,'0','2','-1'):
        flags = dict(full)
        if value is None:
            del flags[name]
        else:
            flags[name] = value
        check(flags, False, include)
for name in ('PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT','PIONEER_Q38_RESIDENT_OUTPUT_OWNER'):
    check(full, False, include+'#define '+name+' 0\n'+include)
print('PASS complete-layer original CPU observer gates'+str(count)+':exact/missing/defined0/mixed/late roles;NO native/device/Work capability')
