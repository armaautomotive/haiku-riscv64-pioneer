#!/usr/bin/env python3
"""Actual diagnostic81 client/metadata,explicit source/device fixtures;NO GPU."""
import ast
import hashlib
import os
import pathlib
import stat
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('Use non-optimized Python PRIVATE_EXPORT')
here = pathlib.Path(__file__).resolve().parent
private = pathlib.Path(sys.argv[1])
assert private.is_absolute() and not private.is_symlink() and stat.S_IMODE(private.stat().st_mode) == 0o700
pin = 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'
assert hashlib.sha256((private / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest() == pin
driver = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
control = driver.split('// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN\n', 1)[1].split('// PIONEER_DRIVER_RELOAD_CONTROL_END', 1)[0]
# Reuse the mature old client's explicit source/device fixture ONLY. Compile
# the actual NEW client,not a rewrite of its numerical/closure comparators.
tree = ast.parse((here / 'pioneer_native_model_band_client_test.py').read_text())
templates = [n.value for n in ast.walk(tree) if isinstance(n, ast.Constant) and isinstance(n.value, str)
             and n.value.startswith('\n#include <cassert>')]
assert len(templates) == 1
program = templates[0]
for old, new in [('PioneerNativeModelBandRequest', 'PioneerNativeDramModelBandRequest'),
                 ('PioneerNativeModelBandTag', 'PioneerNativeDramModelBandTag'),
                 ('PioneerNativeModelBandNone', 'PioneerNativeDramModelBandNone'),
                 ('PIONEER_NATIVE_MODEL_BAND_', 'PIONEER_NATIVE_DRAM_MODEL_BAND_'),
                 ('--fixed-learned-bands', '--fixed-gddr-learned-bands')]:
    assert old in program
    program = program.replace(old, new)
import re
program = re.sub(r'r\.result\.(state|started|completed|cold|pc|polls|count|marker|output|held|l1_restored|tlb_restored)', r'r.result.compute.\1', program)
program = program.replace('auto& v=r.result;', 'auto& v=r.result.compute;')
program = program.replace('v.marker=0x3342464e;', 'v.marker=0x3142444e;')
assert program.count('v.held=v.l1_restored=v.tlb_restored=end;') == 1
program = program.replace('v.held=v.l1_restored=v.tlb_restored=end;', '''
 v.held=v.l1_restored=v.tlb_restored=end;
 r.result.phase=end?10:5;r.result.gddr_restored=r.result.niu_restored=r.result.clock_restored=end;
 r.result.requests=320;r.result.flits=2880;r.result.saved_l1=52812;r.result.saved_gddr=46080;
''')
metadata_faults = '''
  if(fail && scenario>=200 && scenario<=215){
   if(scenario==200)m.version=2;if(scenario==201)m.driver=54;if(scenario==202)m.build_tag^=1;
   if(scenario==203)m.generation=0;if(scenario==204)m.quiesced=1;if(scenario==205)m.cold=1;
   if(scenario==206)m.active_owner=1;if(scenario==207)m.open_count=2;if(scenario==208)m.guard_version=0;
   if(scenario>=209 && scenario<=211)m.reserved[scenario-209]=1;
   if(scenario==212)m.expected_tag=1;if(scenario==213)m.expected_generation=1;if(scenario==214)m.open_count=0;
   if(scenario==215)m.generation++;
   poisoned=true;return 0;
  }
'''
assert program.count('if(fail && scenario==1)m.build_tag^=1;') == 1
program = program.replace('if(fail && scenario==1)m.build_tag^=1;', metadata_faults + 'if(fail && scenario==1)m.build_tag^=1;')
support_faults = '''
  if(fail && scenario>=220 && scenario<=227){
   if(scenario==220)r.version=2;if(scenario==221)r.expected_tag^=1;if(scenario==222)r.expected_generation++;
   if(scenario==223)r.reserved=1;if(scenario==224)r.nonce=1;if(scenario==225)r.accept=1;
   if(scenario==226)r.result.phase=1;if(scenario==227)r.result.compute.cold=1;
   poisoned=true;return 0;
  }
'''
assert program.count('if(fail && scenario==3)r.sample=0;') == 1
program = program.replace('if(fail && scenario==3)r.sample=0;', support_faults + 'if(fail && scenario==3)r.sample=0;')
run_faults = '''
  if(fail && scenario>=90 && scenario<=101){
   if(scenario==90)r.result.phase=4;if(scenario==91)r.result.gddr_restored=1;
   if(scenario==92)r.result.niu_restored=1;if(scenario==93)r.result.clock_restored=1;
   if(scenario==94)r.result.requests--;if(scenario==95)r.result.flits++;
   if(scenario==96)r.result.saved_l1--;if(scenario==97)r.result.saved_gddr++;
   if(scenario==98)r.result.compute.controls[0]|=1;if(scenario==99)r.result.compute.controls[1]=1;
   if(scenario==100)r.result.compute.controls[2]=1;if(scenario==101)r.result.compute.marker=0x3342464e;
   poisoned=true;
  }
'''
assert program.count('return fail && scenario==5?-1:0;') == 1
program = program.replace('return fail && scenario==5?-1:0;', run_faults + 'return fail && scenario==5?-1:0;')
end_faults = '''
 if(fail && scenario>=110 && scenario<=123){
  if(scenario==110)r.result.phase=9;if(scenario==111)r.result.gddr_restored=0;
  if(scenario==112)r.result.niu_restored=0;if(scenario==113)r.result.clock_restored=0;
  if(scenario==114)r.result.requests++;if(scenario==115)r.result.flits++;
  if(scenario==116)r.result.saved_l1--;if(scenario==117)r.result.saved_gddr++;
  if(scenario==118)r.result.compute.controls[0]|=1;if(scenario==119)r.result.compute.controls[1]=1;
  if(scenario==120)r.result.compute.controls[2]=1;if(scenario==121)r.result.compute.marker=0x3342464e;
  if(scenario==122)r.result.compute.controls[0]^=2;if(scenario==123)r.result.compute.polls++;
  poisoned=true;
 }
'''
assert program.count('return fail && scenario==30?-1:0;') == 1
program = program.replace('return fail && scenario==30?-1:0;', end_faults + 'return fail && scenario==30?-1:0;')
program = program.replace('scenario!=1 && scenario!=2', 'scenario!=1 && scenario!=2 && !(scenario>=200 && scenario<=215)')
program = program.replace('scenario!=3 && scenario!=4 && scenario!=63', 'scenario!=3 && scenario!=4 && scenario!=63 && !(scenario>=220 && scenario<=227)')
program = program.replace('(scenario>=30 && scenario<=46)||scenario==52', '(scenario>=30 && scenario<=46)||(scenario>=110 && scenario<=123)||scenario==52')
program = program.replace('CLIENT_SOURCE', str(here.parent / 'pioneer_native_dram_model_band_diagnostic.cpp')).replace('ACTUAL_RELOAD_CONTROL', control)
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-gddr-learned-client.'))
(folder / 'Drivers.h').write_text('#pragma once\n#define B_DEVICE_OP_CODES_END 10000\n')
(folder / 'test.cpp').write_text(program)
compiler = os.environ.get('CXX', 'c++')
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                '-I' + str(folder), '-I' + str(here.parent), '-I' + str(private), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = [(0, 0, 0)]
scenarios = (*range(1, 18), 20, 21, 22, *range(30, 41), 45, 46, 52, 63,
             *range(90, 102), *range(110, 124), *range(200, 215), *range(220, 228))
for scenario in scenarios:
    for target in range(3):
        for word in (range(64) if scenario in (20, 31) else (0,)):
            cases.append((scenario, target, word))
for target in (1, 2):
    cases.append((215, target, 0)) # first metadata establishes the epoch;later drift refuses
for scenario in (50, 51, 53, 60, 61, 62, 70, 80):
    cases.append((scenario, 0, 0))
for scenario, target, word in cases:
    result = subprocess.run([str(folder / 'test'), str(scenario), str(target), str(word)], text=True, capture_output=True,
                            env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    assert result.returncode == 0, (scenario, target, word, result.stdout, result.stderr)
    assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr
    if scenario == 0:
        assert not result.stderr and result.stdout.count('NATIVE_GDDR_BAND_RETURNED_WORD') == 192 and result.stdout.count('PASS_NATIVE_GDDR_BAND sample') == 3
    elif scenario == 20:
        assert result.stderr.count('NATIVE_GDDR_BAND_RETURNED_WORD') == 64 and f'sample{target} pass{word//32} row{word%32} ' in result.stderr
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    result = subprocess.run([compiler, '-std=c++17', '-D' + selection, '-I' + str(folder), '-I' + str(private),
                             '-c', str(here.parent / 'pioneer_native_dram_model_band_diagnostic.cpp'), '-o', str(folder / (selection + '.o'))], text=True, capture_output=True)
    assert result.returncode and 'Learned-band CPU reference is a separate diagnostic' in result.stderr
assert hashlib.sha256((private / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest() == pin
print(f'PASS ACTUAL GDDR learned-band client:{len(cases)} ASan/UBSan cases;actual reload metadata,every64 RUN/END word in all3 samples,journals/counters/restore/control/poll/source/FD/channel refusal,NO next END/sample/query/retry after uncertainty;3 role guards;NO device proof')
print('CPU-only actual GDDR client evidence:', folder)
