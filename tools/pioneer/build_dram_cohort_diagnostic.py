#!/usr/bin/env python3
"""Build bounded physical77/client AFTER ordinary/private controls;no install/run."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_dram_cohort_native.py'),str(build),str(out/'private-controls')]+saved[3:]
    private=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
settings=private['old']
args,features,misc,project,reference,sdk,binpath,cxx=(settings[k] for k in
    ('args','features','misc','project','reference','sdk','binpath','cxx'))
assert hashlib.sha256((out/'private-controls/cohort-DRCNODRV.o').read_bytes()).hexdigest()=='da21a10338471db5e5acaff1fa8057bc817422ba2b6484676708576242bf6d1e', 'private control changed'
flags=features+['-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_COHORT_NATIVE_CONTEXT=1',
 '-DPIONEER_DRAM_COHORT_DIAGNOSTIC=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110300010077ULL']
subjects=[misc/p for p in ('tenstorrent_pci_probe.cpp','PioneerDramReadSweepNative.inc','PioneerDramReadSweepNativeRole.h',
 'PioneerDramCohortNativeRole.h','PioneerDramCohortNative.inc','PioneerDramCohortReadSweepOwner.h',
 'BlackholeDramCohortReadSweepImage.h','PioneerDramCohortProtocol.h','PioneerDramCohortControl.inc')]
subjects += [here/p for p in ('PioneerDramCohortReadSweep.h','PioneerDramCohortHostBracket.h','pioneer_dram_cohort.cpp')]
subjects += [pathlib.Path(__file__).resolve()]
identity={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in subjects}
role_input=('#include "'+str(misc/'PioneerDramReadSweepNativeRole.h')+'"\n').encode()
cases=[(flags,True)]
required=['PIONEER_DRIVER_BUILD_TAG','PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT','PIONEER_DRAM_COHORT_NATIVE_CONTEXT',
 'PIONEER_DRAM_COHORT_DIAGNOSTIC','TT_PIONEER_DRIVER_RELOAD','TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH']
for key in required:
    cases.append(([f for f in flags if not (f=='-D'+key or f.startswith('-D'+key+'='))],False))
for key in required[1:4]:
    for value in (0,2):cases.append(([f for f in flags if not f.startswith('-D'+key+'=')]+['-D'+key+'='+str(value)],False))
for key in ('PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY','PIONEER_DRAM_SWEEP_DIAGNOSTIC','PIONEER_DRAM_SWEEP_QUEUED','PIONEER_DRAM_SWEEP_WIDE',
 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_WORK_MODE','PIONEER_Q38_NORMAL_CLOCK',
 'PIONEER_Q38_GDN_MUTABLE_STATE','PIONEER_Q38_DECODER_RUNTIME'):
    cases += [(flags+['-D'+key+'='+str(v)],False) for v in (0,1)]
for selected,expected in cases:
    r=subprocess.run([str(cxx),'-x','c++','-E','-']+selected,input=role_input,capture_output=True)
    assert (r.returncode==0)==expected,(selected,r.stderr)
print('COHORT_PHYSICAL_ROLE cases'+str(len(cases)),flush=True)
obj=out/'diagnostic77.o'
command=args+flags+['-fcallgraph-info=su','-c',str(misc/'tenstorrent_pci_probe.cpp'),'-o',str(obj)]
(out/'compile-argv.json').write_text(json.dumps(command,indent=2)+'\n')
subprocess.run(command,check=True)
profile=out/'profile.o'
subprocess.run(args+['-c',str(reference/'src/add-ons/kernel/drivers/misc/PioneerPciProfile.cpp'),'-o',str(profile)],check=True)
release=build/'objects/haiku/riscv64/release'
def library(name):return subprocess.check_output([str(cxx),'-print-file-name='+name],text=True).strip()
binary=out/'tenstorrent_pci_probe-dram-cohort-diagnostic77'
subprocess.run([str(cxx),'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',
 library('crtbeginS.o'),str(release/'system/glue/haiku_version_glue.o'),str(obj),str(profile),
 str(release/'system/kernel/kernel.so'),library('libgcc.a'),library('crtendS.o'),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-C','-S',str(binary)],text=True)
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device','api_version'):
    assert any(row.split()[-1:]==[name] for row in symbols.splitlines()),name
for name in ('PioneerDramCohortReadSweepOwner::Execute<PioneerDramCohortNativeIO>',
 'PioneerDramCohortReadSweepOwner::Preflight<PioneerDramCohortNativeIO>',
 'BlackholeDramSweepArc::Queue<PioneerDramSweepClockIO>','PioneerDramSweepNativeContext::ClosePci',
 'map_physical_memory','delete_area','user_memcpy','system_time'):
    assert name in symbols,name
for name in ('PioneerQ38Token','Q38WorkControl','ReferenceProducts','llama_','ggml_','PioneerResidentSession',
 'EndResidentSession','BlackholePioneerMmioEnd','BlackholeDramSamplesProbe','PioneerDramSamplesIO::DramWrite',
 'RunPioneerDramReadSweepNative'):
    assert name not in symbols,name
(out/'module-symbols.txt').write_text(symbols)
assembly=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-dr',str(obj)],text=True)
(out/'module-assembly.txt').write_text(assembly)
frames=[]
for row in obj.with_suffix('.su').read_text().splitlines():
    fields=row.split('\t');assert len(fields)==3 and fields[2]=='static' and int(fields[1])<=2048,row
    frames.append((int(fields[1]),fields[0]))
assert frames
graph=obj.with_suffix('.ci').read_text()
nodes={m[1]:m[2] for row in graph.splitlines() for m in [re.search(r'node: \{ title: "([^"]+)" label: "([^"]*)"',row)] if m}
edges={name:[] for name in nodes}
for row in graph.splitlines():
    m=re.search(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"',row)
    if m:edges.setdefault(m[1],[]).append(m[2])
weights={name:int(m[1]) if m else 0 for name,label in nodes.items() for m in [re.search(r'([0-9]+) bytes \(static\)',label)]}
assert nodes and max(weights.values())==max(n for n,_ in frames)
done={}
def stack(name,active):
    assert name not in active,('recursion',name)
    if name in done:return done[name]
    value=weights.get(name,0)+max([stack(child,active|{name}) for child in edges.get(name,[])]+[0])
    done[name]=value;return value
known=max(stack(name,set()) for name in nodes);assert known<=8192
unresolved=sorted({child for name in edges for child in edges[name] if child not in weights or not weights[child]})
(out/'partial-stack.json').write_text(json.dumps({'maxKnownDirectChain':known,'maxIndividual':max(n for n,_ in frames),
 'unresolved':unresolved,'wholeKernelStackQualified':False},indent=2)+'\n')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
libraries=['-L'+str(p) for p in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
client_obj=out/'client.o';client=out/'pioneer-dram-cohort-diagnostic77'
subprocess.run([str(cxx),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
 '-fstack-usage',*includes,'-c',str(here/'pioneer_dram_cohort.cpp'),'-o',str(client_obj)],check=True)
subprocess.run([str(cxx),*includes,*libraries,'-B'+str(release/'system/glue')+'/',
 '-L'+str(release/'system/libroot'),str(client_obj),'-o',str(client)],check=True)
client_symbols=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-C',str(client)],text=True)
assert 'ioctl' in client_symbols and 'close' in client_symbols
assert not any(name in client_symbols for name in ('llama_','ggml_','PioneerDramCohortReadSweepOwner','PioneerStoreContext'))
for p in (binary,client):print(hashlib.sha256(p.read_bytes()).hexdigest()+' '+p.name,flush=True)
assert identity=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in subjects},'source changed during build'
(out/'source-identities.json').write_text(json.dumps(identity,indent=2)+'\n')
print('BUILD_ONLY_COHORT77 privateControlExact1 ordinaryControlsExact1 fixedPublicDATA1232 '
 'maxIndividualFrame'+str(max(n for n,_ in frames))+' knownDirectChain'+str(known)+' unresolved'+str(len(unresolved))+
 ';NO install/run/native-lifecycle/physical/bandwidth/frequency/model/Work proof',flush=True)
