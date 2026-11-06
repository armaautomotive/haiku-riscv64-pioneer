#!/usr/bin/env python3
"""Actual diagnostic104 native/control bodies over explicit OS/PCI fixtures,NOT hardware."""
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys

trace=sys.argv[-1:] == ['--factored-trace']
configured=sys.argv[-1:] == ['--configured-compare']
manual=configured or sys.argv[-1:] == ['--manual-last-compare']
comparison=manual or sys.argv[-1:] == ['--factored-compare']
paired=sys.argv[-1:] == ['--pair-issue']
timing=trace or comparison or paired or sys.argv[-1:] == ['--timing']
if timing:sys.argv.pop()
if not __debug__ or len(sys.argv) != 4:
    raise SystemExit('Use non-optimized Python NATIVE_BUILD PRIVATE_EXPORT NEW_OUTPUT')
build, private, output = map(pathlib.Path, sys.argv[1:4])
assert build.is_absolute() and build.is_dir() and private.is_absolute() and not private.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
driver = root / 'src/add-ons/kernel/drivers/misc'
pins = {'PioneerNativeCompactQ4StreamData.h': '4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
        'PioneerNativeCompactQ4StreamMetadata.h': 'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'}
assert stat.S_IMODE(private.stat().st_mode) == 0o700 and private.stat().st_uid == os.geteuid()
for name, digest in pins.items():
    info = (private / name).lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
image = build / 'image5120'
large = image if trace else build / 'image17408'
assert hashlib.sha256((image / 'text.bin').read_bytes()).hexdigest() == ('43a1b152f0c7e958b907c1eb7127628b5c67aebe64345227dd71f3c8d17de2ed' if configured else '6542780a82026f4a3a7f410468bb1081a7143991aeb202d39f379a67153dcedd' if manual else 'f281b1419c35b1cb818e7c0278adef916ef6d19e1a2932453ee460510f560dad' if trace else '1e2cc3d675161216bcf5593a79ca483e37c384dab815b34009161cbf36dce280' if comparison else '04b846ca7cb1fcbf0457abfa1790fda9ee68662d58215e76af5d3984bffee261' if paired else '8156dd4e49912270887cf45f98e9b7c2892a0549a72c909dbefc502aa2205512' if timing else 'f67b7b5bc07bddcf5f3ddfccfbe1013c2680650ea1994231565f050d1f0d89c5')
assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
assert hashlib.sha256((large / 'text.bin').read_bytes()).hexdigest() == ('43a1b152f0c7e958b907c1eb7127628b5c67aebe64345227dd71f3c8d17de2ed' if configured else '6542780a82026f4a3a7f410468bb1081a7143991aeb202d39f379a67153dcedd' if manual else 'f281b1419c35b1cb818e7c0278adef916ef6d19e1a2932453ee460510f560dad' if trace else '1e2cc3d675161216bcf5593a79ca483e37c384dab815b34009161cbf36dce280' if comparison else '1fc0e0076948d68a8c8112de23145162bca6de9a30a8f2b7025d3b1f6465d4da' if paired else '9723a83e4550cd66e36613f49d0959251ac0d5f755b903568e8725d4a345ac3f' if timing else 'c95e0cc41971b37dd89786a2c3747b6d8ef6ebbde3a2ec837c63198982373b92')
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
control = (driver / 'pioneer_native_compact_q4_stream_driver.cpp').read_text()
control = control[control.index('namespace PioneerNativeCompactQ4Stream104 {'):]
(output / 'endpoints.h').write_text(prefix + extra + '\n#include "' + str(driver / 'PioneerNativeCompactQ4StreamNative.inc') + '"\n' + control)
sources = [driver / 'PioneerNativeCompactQ4StreamNative.inc', driver / 'pioneer_native_compact_q4_stream_driver.cpp',
           here / 'pioneer_dram_sweep_native_lifecycle_test.cpp', here / 'pioneer_native_compact_q4_stream_native_test.cpp',
           here / 'pioneer_native_compact_q4_stream_lease_test.cpp', pathlib.Path(__file__).resolve(), here.parent / 'PioneerNativeCompactQ4StreamLease.h',
           here.parent / 'PioneerNativeCompactQ4StreamLayout.h', driver / 'PioneerNativeCompactQ4StreamProtocol.h']
sources += [driver/'PioneerNativeCompactQ4TimingRole.h',here.parent/'PioneerQ38Q4KernelTiming.h',
            here/'pioneer_q38_q4_kernel_timing_image_test.cpp']
sources += [driver/'PioneerNativeCompactQ4PairIssueRole.h']
sources += [driver/'PioneerNativeFactoredQ4TraceRole.h',driver/'PioneerNativeFactoredQ4CompareRole.h']
source_pins = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
binary = output / 'native-test'
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-ffp-contract=off', '-fsanitize=address,undefined', '-DCOHORT_SCOPE_COUNTER_TEST',
                *(['-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1'] if timing else []),
                *(['-DPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING=1'] if paired else []),
                *(['-DPIONEER_NATIVE_FACTORED_Q4_COMPARE=1'] if comparison else []),
                *(['-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=1'] if manual else []),
                *(['-DPIONEER_NATIVE_FACTORED_Q4_CONFIGURED=1'] if configured else []),
                *(['-DPIONEER_NATIVE_FACTORED_Q4_TRACE=1'] if trace else []),
                '-I' + str(output), '-I' + str(build),
                '-I' + str(private), '-I' + str(here.parent), str(here / 'pioneer_native_compact_q4_stream_native_test.cpp'),
                '-o', str(binary)], check=True)
subprocess.run([str(binary), str(image), str(large)], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for name, digest in pins.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
for path, digest in source_pins.items():
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
(output / 'native-source-sha256.txt').write_text('\n'.join(digest + '  ' + str(path.relative_to(root)) for path, digest in source_pins.items()) + '\n')
print('PASS actual native'+('114' if configured else '113' if manual else '112' if trace else '111' if comparison else '108' if paired else '107' if timing else '104')+'/control endpoint fixtures;PRIVATE headers unchanged;NO native ARC/prerequisite/engine/hardware/full-model/speed proof:', output)
