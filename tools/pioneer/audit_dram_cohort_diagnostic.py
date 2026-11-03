#!/usr/bin/env python3
"""Frozen whole physical objects/current sources/code array/late-role audit."""
import hashlib
import importlib.util
import json
import os
import pathlib
import subprocess
import sys
assert len(sys.argv)==5 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,native,image,out=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,native,image,out)) and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent;misc=here.parents[1]/'src/add-ons/kernel/drivers/misc'
pins={'tenstorrent_pci_probe-dram-cohort-diagnostic77':'e50f1dfdb05827ee020a106aea474b3e926ad7a8f17de3decfbd71cd72b2e0e3',
      'pioneer-dram-cohort-diagnostic77':'7d2e3e8ec343c117173d94a706e4c1cbbd75666af094c7fe170a6e33d843e8a4'}
for name,digest in pins.items():assert hashlib.sha256((native/name).read_bytes()).hexdigest()==digest
identity=json.loads((native/'source-identities.json').read_text())
assert all(hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d for p,d in identity.items())
spec=importlib.util.spec_from_file_location('cohort_generator',here/'generate_dram_cohort_read_sweep_image.py')
gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
assert gen.render(image/'program.elf').encode('ascii')==(misc/'BlackholeDramCohortReadSweepImage.h').read_bytes()
out.mkdir(mode=0o700);prefix=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
array=out/'actual-physical-code-array.raw'
subprocess.run([prefix+'objcopy','--dump-section=.rodata._ZZN33BlackholeDramCohortReadSweepImage4CodeEjE4text='+str(array),str(native/'diagnostic77.o')],check=True)
assert array.read_bytes()==(image/'text.bin').read_bytes() and hashlib.sha256(array.read_bytes()).hexdigest()==gen.TEXT_SHA
flags=['-DTT_PIONEER_DRIVER_RELOAD','-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH',
 '-DPIONEER_DRIVER_BUILD_TAG=0x2026110300010077ULL','-DPIONEER_DRAM_COHORT_NATIVE_CONTEXT=1',
 '-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_COHORT_DIAGNOSTIC=1']
private=[f for f in flags if not f.startswith(('-DPIONEER_DRIVER_BUILD_TAG=','-DPIONEER_DRAM_COHORT_DIAGNOSTIC='))]
private+=['-DPIONEER_DRIVER_BUILD_TAG=0x4452434e4f445256ULL','-DPIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY=1']
header='#include "'+str(misc/'PioneerDramReadSweepNativeRole.h')+'"\n'
cases=[(header,flags,True),(header,private,True)]
for key in ('PIONEER_DRAM_COHORT_NATIVE_CONTEXT','PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT','PIONEER_DRAM_COHORT_DIAGNOSTIC'):
    for value in ('0','2',''):
        cases.append((header+'#undef '+key+'\n#define '+key+' '+value+'\n'+header,flags,False))
    cases.append((header+'#undef '+key+'\n'+header,flags,False))
cases.append((header+'#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x4452434e4f445256ULL\n'
 '#undef PIONEER_DRAM_COHORT_DIAGNOSTIC\n#define PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY 1\n'+header,flags,False))
cases.append((header+'#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x2026110300010077ULL\n'
 '#undef PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY\n#define PIONEER_DRAM_COHORT_DIAGNOSTIC 1\n'+header,private,False))
histories=[]
for source,selected,expected in cases:
    r=subprocess.run([prefix+'g++','-std=gnu++17','-x','c++','-fsyntax-only','-',*selected],input=source.encode(),capture_output=True)
    assert (r.returncode==0)==expected,(source,selected,r.stderr)
    histories.append({'source':source,'flags':selected,'expected':expected,'exit':r.returncode,
                      'stdout':r.stdout.decode(),'stderr':r.stderr.decode()})
(out/'role-histories.json').write_text(json.dumps(histories,indent=2)+'\n')
print('PASS_COHORT_PHYSICAL_FROZEN_AUDIT exactWholeModuleClient1 currentSourcePins1 actualCodeArray8760 '
 'ELFHeaderExact1 syntaxPositives2 lateRefusals'+str(len(cases)-2)+
 ';NO physical-native-lifecycle/frequency/bandwidth/capacity/authenticity/model/Work proof',flush=True)
