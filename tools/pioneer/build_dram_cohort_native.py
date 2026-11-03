#!/usr/bin/env python3
"""Compile/audit actual private eight-participant kernel context, NO link/run."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,sys.argv[1:3]);saved=sys.argv[:]
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
try:
    sys.argv=[str(here/'tests/run_pioneer_dram_read_sweep_native_compile.py'),str(build),str(out/'unchanged-controls')]+saved[3:]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
args,features,misc,binpath,cxx=(old[k] for k in ('args','features','misc','binpath','cxx'))
flags=features+['-DPIONEER_DRIVER_BUILD_TAG=0x4452434e4f445256ULL']
source=here/'tests/pioneer_dram_cohort_native_compile.cpp';obj=out/'cohort-DRCNODRV.o'
subjects=[misc/p for p in ('tenstorrent_pci_probe.cpp','PioneerDramReadSweepNative.inc','PioneerDramReadSweepNativeRole.h',
 'PioneerDramCohortNativeRole.h','PioneerDramCohortNative.inc','PioneerDramCohortReadSweepOwner.h','BlackholeDramCohortReadSweepImage.h')]
subjects += [here/'PioneerDramCohortReadSweep.h',source,pathlib.Path(__file__).resolve()]
identity={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in subjects}
command=args+flags+['-fcallgraph-info=su','-c',str(source),'-o',str(obj)]
(out/'compile-argv.json').write_text(json.dumps(command,indent=2)+'\n')
subprocess.run(command,check=True)
nm=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-S','--defined-only',str(obj)],text=True)
demangled=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-C','-S',str(obj)],text=True)
assembly=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-dr',str(obj)],text=True)
(out/'native-symbols.txt').write_text(demangled);(out/'native-assembly.txt').write_text(assembly)
defined={row[-1] for row in map(str.split,nm.splitlines()) if len(row)==4}
normal=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
assert not set(normal)&defined
assert all('pioneer_dram_cohort_'+name in defined for name in normal)
for name in ('private_kernel_initialize','private_open','private_run','private_control'):assert 'pioneer_dram_cohort_'+name in defined
for name in ('PioneerDramCohortNativeIO::CohortNativeScope','PioneerDramCohortNativeIO::CohortResetWrite',
 'PioneerDramCohortReadSweepOwner::Execute<PioneerDramCohortNativeIO>',
 'PioneerDramSweepNativeContext::ClosePci','PioneerDramSweepClockIO::Check',
 'get_area_info','get_thread_info','get_sem_count','map_physical_memory','delete_area','system_time'):
    assert name in demangled,name
for name in ('llama_','ggml_','ReferenceProducts','PioneerQ38Token','EndResidentSession','BlackholePioneerMmioEnd'):
    assert name not in demangled,name
# Audit five ACTUAL renamed inert entry bodies,not only their names.
symbols={r[-1]:(int(r[0],16),int(r[1],16)) for r in map(str.split,nm.splitlines()) if len(r)==4}
table=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-t',str(obj)],text=True)
sections={r[-1]:r[-3] for r in map(str.split,table.splitlines()) if len(r)>=6 and r[-1] in symbols}
text=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-dr','-j','.text',str(obj)],text=True)
instructions={int(m[1],16):m[2] for row in text.splitlines() for m in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+',row)] if m}
relocations=[(int(m[1],16),m[2]) for row in text.splitlines() for m in [re.match(r'^\s*([0-9a-f]+):\s+R_\S+\s+(\S+)',row)] if m]
def code(name):
    name='pioneer_dram_cohort_'+name;start,size=symbols[name];assert sections[name]=='.text'
    assert not any(start<=a<start+size for a,_ in relocations)
    words=[w for a,w in sorted(instructions.items()) if start<=a<start+size]
    assert sum(len(w)//2 for w in words)==size
    return words
for name in ('init_hardware','init_driver'):assert code(name)==code('expected_refusal')
for name in ('publish_devices','find_device'):assert code(name)==code('expected_null')
assert code('uninit_driver')==code('expected_inert')
frames=[]
for row in obj.with_suffix('.su').read_text().splitlines():
    f=row.split('\t');assert len(f)==3 and f[2]=='static' and int(f[1])<=2048,row
    frames.append((int(f[1]),f[0]))
assert frames
role=features+['-DPIONEER_DRIVER_BUILD_TAG=0x4452434e4f445256ULL',
 '-DPIONEER_DRAM_COHORT_NATIVE_CONTEXT=1','-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY=1']
role_input=('#include "'+str(misc/'PioneerDramReadSweepNativeRole.h')+'"\n').encode()
required=['PIONEER_DRIVER_BUILD_TAG','PIONEER_DRAM_COHORT_NATIVE_CONTEXT','PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT',
 'PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY','TT_PIONEER_DRIVER_RELOAD','TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH']
cases=[(role,True)]
for key in required:
    cases.append(([a for a in role if not (a=='-D'+key or a.startswith('-D'+key+'='))],False))
for key in ('PIONEER_DRAM_COHORT_NATIVE_CONTEXT','PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT','PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY'):
    for value in ('0','2'):
        cases.append(([a for a in role if not a.startswith('-D'+key+'=')]+['-D'+key+'='+value],False))
for key in ('PIONEER_DRAM_SWEEP_DIAGNOSTIC','PIONEER_DRAM_SWEEP_QUEUED','PIONEER_DRAM_SWEEP_WIDE',
 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT','PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT',
 'PIONEER_Q38_RESIDENT_OUTPUT_OWNER','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
 'PIONEER_Q38_WORK_MODE','PIONEER_Q38_NORMAL_CLOCK','PIONEER_Q38_GDN_MUTABLE_STATE','PIONEER_Q38_DECODER_RUNTIME'):
    cases += [(role+['-D'+key+'='+str(v)],False) for v in (0,1)]
for flags,expected in cases:
    r=subprocess.run([str(cxx),'-x','c++','-E','-']+flags,input=role_input,capture_output=True)
    assert (r.returncode==0)==expected,(flags,r.stderr)
assert identity=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in subjects},'source changed during build'
(out/'source-identities.json').write_text(json.dumps(identity,indent=2)+'\n')
print('PASS_PRIVATE_COHORT_SDK '+hashlib.sha256(obj.read_bytes()).hexdigest()+' maxIndividualFrame'+str(max(n for n,_ in frames))+
      ' roleCases'+str(len(cases))+' actualNativeBodies1 inertRenamedABI1 oldControlsFirst1;'
      'NO LINK/INSTALL/native-endpoint-lifecycle/hardware/bandwidth/model/Work permission',flush=True)
