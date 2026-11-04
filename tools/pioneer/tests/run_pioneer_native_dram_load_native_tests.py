#!/usr/bin/env python3
"""Actual LOAD-ONLY82 native/control sources+explicit KernelExport/PCI fixtures."""
import ast
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys
from pioneer_native_dram_load_fixture import load_component

if not __debug__ or len(sys.argv) != 4:
    raise SystemExit('Use non-optimized Python NATIVE_BUILD PRIVATE_EXPORT NEW_OUTPUT')
build, private, output = map(pathlib.Path, sys.argv[1:])
assert build.is_absolute() and build.is_dir() and private.is_absolute() and not private.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
assert stat.S_IMODE(private.stat().st_mode) == 0o700 and private.stat().st_uid == os.geteuid()
pins = {'PioneerNativeBfpModelBandData.h': 'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
        'PioneerNativeBfpModelBandMetadata.h': 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
for name, digest in pins.items():
    info = (private / name).lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_uid == os.geteuid() and info.st_nlink == 1 and stat.S_IMODE(info.st_mode) == 0o600
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
assert hashlib.sha256((build / 'image/text.bin').read_bytes()).hexdigest() == '966507e23929a1d28f62f56de0ea3c3a6c9148d9ff8f59c84138f746eea779a0'
here = pathlib.Path(__file__).resolve().parent;root = here.parents[2];driver = root / 'src/add-ons/kernel/drivers/misc'
output.mkdir(mode=0o700)
prefix = (here / 'pioneer_dram_sweep_native_lifecycle_test.cpp').read_text().split('struct PioneerDramSweepNativeContext;')[0]
prefix = re.sub(r'#include "(\.\./\.\./\.\./[^"]+)"', lambda m: '#include "' + str((here / m[1]).resolve()) + '"', prefix)
prefix = prefix.replace('static std::map<area_id,area_info> views;', 'static std::map<area_id,area_info> views;\nstatic std::map<area_id,uint64> physicalViews;\nstatic std::map<volatile uint32*,volatile uint32*> mirrors;')
prefix = prefix.replace('map_physical_memory(const char*,uint64,size_t bytes', 'map_physical_memory(const char*,uint64 physical,size_t bytes')
prefix = prefix.replace('views[id]={id,*pointer,bytes,protection};return id;', 'views[id]={id,*pointer,bytes,protection};physicalViews[id]=physical;return id;')
prefix = prefix.replace('Touch();assert(p);*p=value;', 'Touch();assert(p);*p=value;if(mirrors.count(p))*mirrors.at(p)=value;')
tree = ast.parse((here / 'run_pioneer_native_dram_model_band_native_tests.py').read_text())
extra = [ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'extra' for t in n.targets)]
assert len(extra) == 1
control = (driver / 'pioneer_native_dram_load_driver.cpp').read_text()
control = control[control.index('namespace PioneerNativeDram82 {'):]
role = '\n#define _KERNEL_MODE 1\n#define TT_PIONEER_DRIVER_RELOAD 1\n#define PIONEER_DRIVER_BUILD_TAG 0x2026110400010082ULL\n#define PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC 1\n'
end_role = '\n#undef PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC\n#undef _KERNEL_MODE\n'
(output / 'endpoints.h').write_text(prefix + extra[0] + role + '#include "' + str(driver / 'PioneerNativeDramModelBandNative.inc') + '"\n' + control + end_role)
(output / 'load-component.h').write_text(load_component(here))
test = (here / 'pioneer_native_dram_model_band_native_test.cpp').read_text()
test = test.replace('#define PIONEER_NATIVE_DRAM_MODEL_LEASE_TEST_ONLY\n#include "pioneer_native_dram_model_band_lease_test.cpp"', '#include "load-component.h"')
test = test.replace('PioneerNativeDram81', 'PioneerNativeDram82')
for old, new in [('PioneerNativeDramModelBandResult', 'PioneerNativeDramLoadResult'),
                 ('PioneerNativeDramModelBandRequest', 'PioneerNativeDramLoadRequest'),
                 ('PioneerNativeDramModelBandNone', 'PioneerNativeDramLoadNone'),
                 ('PioneerNativeDramModelBandTag', 'PioneerNativeDramLoadTag'),
                 ('PIONEER_NATIVE_DRAM_MODEL_BAND_', 'PIONEER_NATIVE_DRAM_LOAD_')]:
    test = test.replace(old, new)
test = test.replace('0x04001400', '0x80001400').replace('0x3142444e', '0x314c444e').replace('static_cast<void>(&NoMore);', '')
test = test.replace('r.gddr_restored=r.niu_restored=r.clock_restored=1;', 'r.gddr_restored=r.niu_restored=r.clock_restored=1;r.payload_checked=46080;r.headers_checked=2560;')
marker = '\tfor(unsigned fault=0;fault<20;fault++) {'
assert marker in test
extra_close = r'''
 {
  NativeFixture f;f.Map(5);f.c->resetStage=2;f.c->fifoStage=1;f.Reset(0x46800);uint32 v=0;
  auto& r=f.Result().compute;r.phase=0x7000c;r.count=0x80001400;r.marker=0x314c444e;r.pc[0]=r.pc[1]=0x70038;
  assert(!f.c->io.NativeL1Read(0x90000,v));r.state=PIONEER_NATIVE_BFP_RESULT;r.completed=1;
  assert(f.c->io.NativeL1Read(0x90000,v));r.marker=0x3142444e;assert(!f.c->io.NativeL1Read(0x90000,v));valid++;refused+=2;
 }
 for(unsigned fault=0;fault<4;fault++){
  NativeFixture f;f.Map();f.Restored();
  if(fault==0)f.Result().payload_checked--;if(fault==1)f.Result().headers_checked--;
  if(fault==2)f.Result().mismatch_kind=1;if(fault==3)f.Result().mismatch_offset=1;
  const unsigned before=ioCalls;assert(!N::Finish(*f.c) && ioCalls==before && !deletes && !pciWrites && !unreserves);refused++;
 }
'''
test = test.replace(marker, extra_close + marker)
test = test.replace('actual native81 endpoint fixtures', 'actual LOAD ONLY82 endpoint fixtures')
(output / 'test.cpp').write_text(test)
sources = [driver / 'PioneerNativeDramModelBandNative.inc', driver / 'pioneer_native_dram_load_driver.cpp',
           here.parent / 'PioneerNativeDramLoadLease.h', here / 'pioneer_native_dram_model_band_native_test.cpp',
           here / 'pioneer_native_dram_load_fixture.py', pathlib.Path(__file__).resolve()]
source_pins = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-ffp-contract=off', '-fsanitize=address,undefined', '-DCOHORT_SCOPE_COUNTER_TEST', '-I' + str(output),
                '-I' + str(build), '-I' + str(private), '-I' + str(here), '-I' + str(here.parent),
                str(output / 'test.cpp'), '-o', str(output / 'test')], check=True)
subprocess.run([str(output / 'test'), str(build / 'image')], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for p, digest in source_pins.items():
    assert hashlib.sha256(p.read_bytes()).hexdigest() == digest
for name, digest in pins.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
(output / 'source-sha256.txt').write_text('\n'.join(digest + '  ' + str(p.relative_to(root)) for p, digest in source_pins.items()) + '\n')
print('PASS actual LOAD ONLY82 native/control source fixtures,full data-check closure gates;NO fabricated successful native prerequisite/ARC/engine/hardware/byte proof;PRIVATE unchanged:', output)
