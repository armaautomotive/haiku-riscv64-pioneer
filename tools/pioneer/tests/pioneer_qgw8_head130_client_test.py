#!/usr/bin/env python3
"""Actual head130 client with simulated source, device and reload metadata."""
import ast
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv)!=2 or not Path(sys.argv[1]).is_absolute():
    raise SystemExit('usage: ABS_PRIVATE_HEAD_EXPORT')
private=Path(sys.argv[1]); here=Path(__file__).resolve().parent
pin='664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'
if hashlib.sha256((private/'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest()!=pin:
    raise SystemExit('private metadata hash refused')
driver=(here.parents[2]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
control=driver.split('// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN\n',1)[1].split('// PIONEER_DRIVER_RELOAD_CONTROL_END',1)[0]
tree=ast.parse((here/'pioneer_native_model_band_client_test.py').read_text())
templates=[n.value for n in ast.walk(tree) if isinstance(n,ast.Constant) and isinstance(n.value,str)
           and n.value.startswith('\n#include <cassert>')]
if len(templates)!=1: raise RuntimeError('old fixture is not unique')
program=templates[0]
program=program.replace('#include "PioneerNativeModelBandSource.h"',
                        '#include <string>\n#include "PioneerNativeBfpModelBandMetadata.h"\n#define PIONEER_QGW8_HEAD_BAND_SOURCE_H 1')
for old,new in [('PioneerNativeModelBandSource','PioneerQgw8HeadBandSource'),
                ('PioneerNativeModelBandRequest','PioneerNativeDramModelBandRequest'),
                ('PioneerNativeModelBandTag','PioneerNativeDramModelBandTag'),
                ('PioneerNativeModelBandNone','PioneerNativeDramModelBandNone'),
                ('PIONEER_NATIVE_MODEL_BAND_','PIONEER_NATIVE_DRAM_MODEL_BAND_'),
                ('--fixed-learned-bands','--fixed-qgw8-head-bands')]:
    if old not in program: raise RuntimeError('fixture transform missing: '+old)
    program=program.replace(old,new)
program=re.sub(r'r\.result\.(state|started|completed|cold|pc|polls|count|marker|output|held|l1_restored|tlb_restored)',r'r.result.compute.\1',program)
program=program.replace('auto& v=r.result;','auto& v=r.result.compute;')
program=program.replace('v.marker=0x3342464e;','v.marker=0x3436424e;')
if program.count('v.held=v.l1_restored=v.tlb_restored=end;')!=1: raise RuntimeError('fixture closure transform')
program=program.replace('v.held=v.l1_restored=v.tlb_restored=end;', '''
 v.held=v.l1_restored=v.tlb_restored=end;
 r.result.phase=end?10:5;r.result.gddr_restored=r.result.niu_restored=r.result.clock_restored=end;
 r.result.requests=320;r.result.flits=2880;r.result.saved_l1=55372;r.result.saved_gddr=46080;
''')
program=program.replace('CLIENT_SOURCE',str(here.parent/'pioneer_qgw8_head_band130_diagnostic.cpp')).replace('ACTUAL_RELOAD_CONTROL',control)
with tempfile.TemporaryDirectory(prefix='qgw8-head130-client-',dir='/private/tmp') as tmp:
    folder=Path(tmp);(folder/'Drivers.h').write_text('#pragma once\n#define B_DEVICE_OP_CODES_END 10000\n')
    (folder/'test.cpp').write_text(program)
    flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
           '-fsanitize=address,undefined','-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1',
           '-DPIONEER_QGW8_HEAD_BAND_130=1','-I'+str(folder),'-I'+str(here.parent),'-I'+str(private)]
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(folder/'test.cpp'),'-o',str(folder/'test')],check=True)
    cases=[(0,0,0)]
    for scenario in (*range(1,18),20,21,22,*range(30,41),45,46,52,63):
        for target in range(3):
            for word in (range(64) if scenario in (20,31) else (0,)):
                cases.append((scenario,target,word))
    cases.extend((scenario,0,0) for scenario in (50,51,53,60,61,62,70,80))
    for scenario,target,word in cases:
        result=subprocess.run([str(folder/'test'),str(scenario),str(target),str(word)],text=True,capture_output=True,
                              env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
        if result.returncode or 'AddressSanitizer' in result.stderr or 'runtime error:' in result.stderr:
            raise RuntimeError((scenario,target,word,result.stdout,result.stderr))
        if scenario==0 and (result.stderr or result.stdout.count('HEAD130_SAMPLE sample')!=3 or 'PASS_HEAD130' not in result.stdout):
            raise RuntimeError('success transcript mismatch')
        if scenario==20 and 'HEAD130_NUMERICAL_MISMATCH' not in result.stderr:
            raise RuntimeError('numerical mismatch not reported')
if hashlib.sha256((private/'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest()!=pin:
    raise RuntimeError('private metadata changed')
print('PASS actual head130 client: %d synthetic source/FD/reload/RUN/END/output-fault cases; NO DEVICE'%len(cases))
