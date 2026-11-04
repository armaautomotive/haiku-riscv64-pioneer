#!/usr/bin/env python3
"""Actual fixed LOAD-ONLY82 client/metadata;explicit fake endpoints,NO GPU."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv) not in (2, 3) or (len(sys.argv) == 3 and sys.argv[2] != '--alignment64-control'):
    raise SystemExit('Use non-optimized Python PRIVATE_EXPORT [--alignment64-control]')
aligned = len(sys.argv) == 3
variant = ['-DPIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64=1'] if aligned else []
here = pathlib.Path(__file__).resolve().parent;private = pathlib.Path(sys.argv[1])
pin = 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'
assert hashlib.sha256((private / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest() == pin
driver = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
control = driver.split('// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN\n', 1)[1].split('// PIONEER_DRIVER_RELOAD_CONTROL_END', 1)[0]
tree = ast.parse((here / 'pioneer_native_model_band_client_test.py').read_text())
templates = [n.value for n in ast.walk(tree) if isinstance(n, ast.Constant) and isinstance(n.value, str) and n.value.startswith('\n#include <cassert>')]
assert len(templates) == 1
program = templates[0]
for old, new in [('PioneerNativeModelBandRequest', 'PioneerNativeDramLoadRequest'), ('PioneerNativeModelBandTag', 'PioneerNativeDramLoadTag'),
                 ('PioneerNativeModelBandNone', 'PioneerNativeDramLoadNone'), ('PIONEER_NATIVE_MODEL_BAND_', 'PIONEER_NATIVE_DRAM_LOAD_'),
                 ('--fixed-learned-bands', '--fixed-load-only-band')]:
    program = program.replace(old, new)
program = re.sub(r'r\.result\.(state|started|completed|cold|pc|polls|count|marker|output|held|l1_restored|tlb_restored)', r'r.result.compute.\1', program)
program = program.replace('auto& v=r.result;', 'auto& v=r.result.compute;')
program = program.replace('v.count=0x04001400;v.marker=0x3342464e;', 'v.count=PioneerNativeDramLoadVariant::Count;v.marker=PioneerNativeDramLoadVariant::Marker;')
program = program.replace('v.held=v.l1_restored=v.tlb_restored=end;', '''v.held=v.l1_restored=v.tlb_restored=end;
 r.result.phase=end?10:5;r.result.gddr_restored=r.result.niu_restored=r.result.clock_restored=end;
 r.result.requests=320;r.result.flits=2880;r.result.saved_l1=PioneerNativeDramLoadVariant::L1Words;r.result.saved_gddr=46080;
 r.result.payload_checked=end?46080:0;r.result.headers_checked=end?PioneerNativeDramLoadVariant::HeaderWords:0;''')
program = program.replace('PioneerNativeBfpModelBandMetadata::Golden(sample)[word%32]', '0x7fc00001')
program = program.replace('opened==3 && closed==3 && queries==3 && supports==3 && runs==3 && ends==3', 'opened==1 && closed==1 && queries==1 && supports==1 && runs==1 && ends==1')
program = program.replace('scenario==53 && flushes==6', 'scenario==53 && flushes==3')
metadata_faults = r'''
  if(fail && scenario>=200 && scenario<=214){
   if(scenario==200)m.version=2;if(scenario==201)m.driver=54;if(scenario==202)m.build_tag^=1;
   if(scenario==203)m.generation=0;if(scenario==204)m.quiesced=1;if(scenario==205)m.cold=1;
   if(scenario==206)m.active_owner=1;if(scenario==207)m.open_count=2;if(scenario==208)m.guard_version=0;
   if(scenario>=209 && scenario<=211)m.reserved[scenario-209]=1;
   if(scenario==212)m.expected_tag=1;if(scenario==213)m.expected_generation=1;if(scenario==214)m.open_count=0;
   poisoned=true;return 0;
  }
'''
program = program.replace('if(fail && scenario==1)m.build_tag^=1;', metadata_faults + 'if(fail && scenario==1)m.build_tag^=1;')
support_faults = r'''
  if(fail && scenario>=220 && scenario<=227){
   if(scenario==220)r.version=2;if(scenario==221)r.expected_tag^=1;if(scenario==222)r.expected_generation++;
   if(scenario==223)r.reserved=1;if(scenario==224)r.nonce=1;if(scenario==225)r.accept=1;
   if(scenario==226)r.result.phase=1;if(scenario==227)r.result.compute.cold=1;poisoned=true;return 0;
  }
'''
program = program.replace('if(fail && scenario==3)r.sample=0;', support_faults + 'if(fail && scenario==3)r.sample=0;')
run_faults = r'''
  if(fail && scenario>=90 && scenario<=106){
   if(scenario==90)r.result.phase=4;if(scenario==91)r.result.gddr_restored=1;if(scenario==92)r.result.niu_restored=1;
   if(scenario==93)r.result.clock_restored=1;if(scenario==94)r.result.requests--;if(scenario==95)r.result.flits++;
   if(scenario==96)r.result.saved_l1--;if(scenario==97)r.result.saved_gddr++;
   if(scenario==98)r.result.compute.controls[0]|=1;if(scenario==99)r.result.compute.controls[1]=1;if(scenario==100)r.result.compute.controls[2]=1;
   if(scenario==101)r.result.compute.count=0x04001400;if(scenario==102)r.result.payload_checked=1;
   if(scenario==103)r.result.headers_checked=1;if(scenario==104)r.result.mismatch_kind=1;if(scenario==105)r.result.mismatch_offset=1;
   if(scenario==106)r.result.compute.marker=0x3142444e;poisoned=true;
  }
'''
end_faults = r'''
 if(fail && scenario>=110 && scenario<=127){
  if(scenario==110)r.result.phase=9;if(scenario==111)r.result.gddr_restored=0;if(scenario==112)r.result.niu_restored=0;
  if(scenario==113)r.result.clock_restored=0;if(scenario==114)r.result.requests++;if(scenario==115)r.result.flits++;
  if(scenario==116)r.result.saved_l1--;if(scenario==117)r.result.saved_gddr++;
  if(scenario==118)r.result.compute.controls[0]|=1;if(scenario==119)r.result.compute.controls[1]=1;if(scenario==120)r.result.compute.controls[2]=1;
  if(scenario==121)r.result.compute.marker=0x3142444e;if(scenario==122)r.result.compute.controls[0]^=2;if(scenario==123)r.result.compute.polls++;
  if(scenario==124)r.result.payload_checked--;if(scenario==125)r.result.headers_checked--;
  if(scenario==126)r.result.mismatch_kind=2;if(scenario==127)r.result.mismatch_offset=1;poisoned=true;
 }
'''
program = program.replace('return fail && scenario==5?-1:0;', run_faults + 'return fail && scenario==5?-1:0;')
if aligned:
    program = program.replace('scenario>=90 && scenario<=106', 'scenario>=90 && scenario<=109')
    program = program.replace('if(scenario==106)r.result.compute.marker=0x3142444e;poisoned=true;',
                              'if(scenario==106)r.result.compute.marker=0x3142444e;'
                              'if(scenario==107)r.result.compute.count=0x80001400;'
                              'if(scenario==108)r.result.compute.marker=0x314c444e;'
                              'if(scenario==109)r.expected_tag=UINT64_C(0x2026110400010082);poisoned=true;')
program = program.replace('return fail && scenario==30?-1:0;', end_faults + 'return fail && scenario==30?-1:0;')
program = program.replace('(scenario>=30 && scenario<=46)||scenario==52', '(scenario>=30 && scenario<=46)||(scenario>=110 && scenario<=127)||scenario==52')
program = program.replace('scenario!=1 && scenario!=2', 'scenario!=1 && scenario!=2 && !(scenario>=200 && scenario<=214)')
program = program.replace('scenario!=3 && scenario!=4 && scenario!=63', 'scenario!=3 && scenario!=4 && scenario!=63 && !(scenario>=220 && scenario<=227)')
program = program.replace('CLIENT_SOURCE', str(here.parent / 'pioneer_native_dram_load_diagnostic.cpp')).replace('ACTUAL_RELOAD_CONTROL', control)
if aligned:
    program = program.replace('--fixed-load-only-band', '--fixed-load64-only-band')
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-load-only-client.'))
(folder / 'Drivers.h').write_text('#pragma once\n#define B_DEVICE_OP_CODES_END 10000\n')
(folder / 'test.cpp').write_text(program)
compiler = os.environ.get('CXX', 'c++')
subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                *variant, '-I' + str(folder), '-I' + str(here.parent), '-I' + str(private), str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
cases = [(0, 0)]
for scenario in (*range(1, 18), 20, 21, 22, *range(30, 41), 45, 46, 52, 63, *range(90, 107), *range(110, 128), *range(200, 215), *range(220, 228)):
    for word in (range(64) if scenario in (20, 31) else (0,)):
        cases.append((scenario, word))
for scenario in (50, 51, 53, 60, 61, 62, 70, 80):
    cases.append((scenario, 0))
if aligned:
    cases.extend((scenario, 0) for scenario in (107, 108, 109))
for scenario, word in cases:
    result = subprocess.run([str(folder / 'test'), str(scenario), '0', str(word)], text=True, capture_output=True,
                            env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    assert result.returncode == 0, (scenario, word, result.stdout, result.stderr)
    assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr
    assert 'RETURNED_WORD' not in result.stdout + result.stderr, 'No private/model words can be printed'
    if not scenario:
        assert not result.stderr and result.stdout.count('PASS_NATIVE_DRAM_LOAD%s_ONLY sample0' % ('64' if aligned else '')) == 1
    elif scenario in (104, 105, 126, 127):
        assert 'mismatch_kind' in result.stderr and 'mismatch_offset' in result.stderr and 'NO_PRIVATE_WORD_DUMP' in result.stderr
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    result = subprocess.run([compiler, '-std=c++17', '-D' + selection, *variant, '-I' + str(folder), '-I' + str(private),
                             '-c', str(here.parent / 'pioneer_native_dram_load_diagnostic.cpp'), '-o', str(folder / (selection + '.o'))], text=True, capture_output=True)
    assert result.returncode and 'Learned-band CPU reference is a separate diagnostic' in result.stderr
assert hashlib.sha256((private / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()).hexdigest() == pin
print(f'PASS ACTUAL LOAD-ONLY{83 if aligned else 82} client:{len(cases)} ASan/UBSan cases;actual reload metadata,all64 sentinel mutations RUN/END,byte-check/counter/restore/marker/role/source/FD/channel gates;no private word dump,no second END/retry;3 forbidden roles;NO hardware proof')
print('CPU-only actual LOAD-ONLY client evidence:', folder)
