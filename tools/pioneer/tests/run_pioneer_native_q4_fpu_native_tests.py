#!/usr/bin/env python3
"""Actual diagnostic85 native/control bodies over explicit OS/PCI fixtures,NOT hardware."""
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys

if not __debug__ or len(sys.argv) != 4:
    raise SystemExit('Use non-optimized Python NATIVE_BUILD PRIVATE_EXPORT NEW_OUTPUT')
build, private, output = map(pathlib.Path, sys.argv[1:4])
assert build.is_absolute() and build.is_dir() and private.is_absolute() and not private.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
driver = root / 'src/add-ons/kernel/drivers/misc'
pins = {'PioneerNativeQ4FpuData.h': '8e4c2dfa81b436ffc79e6719c41a4090c4ae2501bf1a1f17f53f5759cd5c548b',
        'PioneerNativeQ4FpuMetadata.h': 'd075e5a9e313e221ed626c563d7dd861d50c9334ca96ca7f54255be6193787c9'}
assert stat.S_IMODE(private.stat().st_mode) == 0o700 and private.stat().st_uid == os.geteuid()
for name, digest in pins.items():
    info = (private / name).lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
image = build / 'image'
assert hashlib.sha256((image / 'text.bin').read_bytes()).hexdigest() == '7e3efc36b1447a3880081caf899175a87378f894ea684d7623572625242e04d4'
assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
output.mkdir(mode=0o700)
# Reuse ONLY the explicit kernel/PCI endpoint definitions,NOT the old owner,
# old native methods or a successful ARC/baseline proof. Native.inc/control
# bodies below are included unchanged from the current production sources.
prefix = (here / 'pioneer_dram_sweep_native_lifecycle_test.cpp').read_text().split('struct PioneerDramSweepNativeContext;')[0]
prefix = re.sub(r'#include "(\.\./\.\./\.\./[^"]+)"', lambda m: '#include "' + str((here / m[1]).resolve()) + '"', prefix)
assert prefix.count('static std::map<area_id,area_info> views;') == 1
prefix = prefix.replace('static std::map<area_id,area_info> views;',
                        'static std::map<area_id,area_info> views;\nstatic std::map<area_id,uint64> physicalViews;\nstatic std::map<volatile uint32*,volatile uint32*> mirrors;')
assert prefix.count('map_physical_memory(const char*,uint64,size_t bytes') == 1
prefix = prefix.replace('map_physical_memory(const char*,uint64,size_t bytes',
                        'map_physical_memory(const char*,uint64 physical,size_t bytes')
assert prefix.count('views[id]={id,*pointer,bytes,protection};return id;') == 1
prefix = prefix.replace('views[id]={id,*pointer,bytes,protection};return id;',
                        'views[id]={id,*pointer,bytes,protection};physicalViews[id]=physical;return id;')
assert prefix.count('Touch();assert(p);*p=value;') == 1
prefix = prefix.replace('Touch();assert(p);*p=value;',
                        'Touch();assert(p);*p=value;if(mirrors.count(p))*mirrors.at(p)=value;')
extra = r'''
using bigtime_t=int64_t;
static bigtime_t fakeTime=123456;
static bigtime_t system_time(){Touch();return fakeTime;}
static status_t acquire_sem(sem_id id){assert(id==sLock);semCount=0;return B_OK;}
static status_t AcquireProbeLock(PioneerProbeCookie*){return AcquireProbeLock();}
static unsigned probeFrees=0,reloadCalls=0;
static status_t ProbeOpen(const char*,uint32,void** p){*p=new PioneerProbeCookie;sOpenCookies++;return B_OK;}
static status_t ProbeClose(void*){return B_OK;}
static status_t ProbeFree(void* p){probeFrees++;delete static_cast<PioneerProbeCookie*>(p);sOpenCookies--;return B_OK;}
enum {TT_PCI_PROBE_RELOAD_STATUS=100,TT_PCI_PROBE_RELOAD_QUIESCE=101};
static status_t DriverReloadControl(PioneerProbeCookie*,uint32,void*,size_t){reloadCalls++;return B_OK;}
struct device_hooks {
 status_t(*open)(const char*,uint32,void**);status_t(*close)(void*);status_t(*free)(void*);
 status_t(*control)(void*,uint32,void*,size_t);void *read,*write,*select,*deselect,*readv,*writev;
};
#define B_DEVICE_OP_CODES_END 10000
'''
control = (driver / 'pioneer_native_q4_fpu_driver.cpp').read_text()
control = control[control.index('namespace PioneerNativeQ4Fpu85 {'):]
(output / 'endpoints.h').write_text(prefix + extra + '\n#include "' + str(driver / 'PioneerNativeQ4FpuNative.inc') + '"\n' + control)
sources = [driver / 'PioneerNativeQ4FpuNative.inc', driver / 'pioneer_native_q4_fpu_driver.cpp',
           here / 'pioneer_dram_sweep_native_lifecycle_test.cpp', here / 'pioneer_native_q4_fpu_native_test.cpp',
           here / 'pioneer_native_q4_fpu_lease_test.cpp', pathlib.Path(__file__).resolve(), here.parent / 'PioneerNativeQ4FpuLease.h',
           here.parent / 'PioneerNativeQ4FpuLayout.h', driver / 'PioneerNativeQ4FpuProtocol.h']
source_pins = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
binary = output / 'native-test'
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-ffp-contract=off', '-fsanitize=address,undefined', '-DCOHORT_SCOPE_COUNTER_TEST',
                '-I' + str(output), '-I' + str(build),
                '-I' + str(private), '-I' + str(here.parent), str(here / 'pioneer_native_q4_fpu_native_test.cpp'),
                '-o', str(binary)], check=True)
subprocess.run([str(binary), str(image)], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for name, digest in pins.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
for path, digest in source_pins.items():
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
(output / 'native-source-sha256.txt').write_text('\n'.join(digest + '  ' + str(path.relative_to(root)) for path, digest in source_pins.items()) + '\n')
print('PASS actual native85/control endpoint fixtures;PRIVATE headers unchanged;NO native ARC/prerequisite/engine/hardware/full-model/speed proof:', output)
