#!/usr/bin/env python3
"""Build+audit diagnostic76 and its bounded CLI. NEVER installs or runs them."""
import hashlib
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
    sys.argv=[str(here/'tests/run_pioneer_dram_read_sweep_native_compile.py'),str(build),str(out/'native-controls')]+saved[3:]
    settings=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
args,features,misc,project,reference,sdk,binpath,cxx=(settings[k] for k in
    ('args','features','misc','project','reference','sdk','binpath','cxx'))
flags=features+['-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_SWEEP_DIAGNOSTIC=1',
               '-DPIONEER_DRAM_SWEEP_QUEUED=1','-DPIONEER_DRAM_SWEEP_WIDE=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110200050076ULL']
subjects=[misc/name for name in ('tenstorrent_pci_probe.cpp','PioneerDramReadSweepNative.inc',
 'PioneerDramReadSweepNativeRole.h','PioneerDramReadSweepOwner.h','BlackholeDramReadSweepImage.h','BlackholeDramQueuedReadSweepImage.h','BlackholeDramWideQueuedReadSweepImage.h',
 'BlackholeDramSweepArcClock.h','BlackholeDramSweepRoutes.h','PioneerDramSweepControl.inc','PioneerDramSweepProtocol.h')]
subjects += [here/'pioneer_dram_sweep.cpp',here/'PioneerDramQueuedReadSweep.h',here/'PioneerDramWideQueuedReadSweep.h']
subjects += [pathlib.Path(__file__).resolve()]
source_identity={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in subjects}
role_input=('#include "'+str(misc/'PioneerDramReadSweepNativeRole.h')+'"\n').encode()
cases=[(flags,True)]
for selector in ('PIONEER_DRAM_SWEEP_QUEUED','PIONEER_DRAM_SWEEP_WIDE'):
    for value in (0,2):
        cases.append(([f for f in flags if not f.startswith('-D'+selector+'=')]
                      +['-D'+selector+'='+str(value)],False))
for required in ('-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_SWEEP_DIAGNOSTIC=1',
                 '-DPIONEER_DRAM_SWEEP_QUEUED=1','-DPIONEER_DRAM_SWEEP_WIDE=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110200050076ULL','-DTT_PIONEER_DRIVER_RELOAD',
                 '-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH'):
    cases.append(([value for value in flags if value!=required],False))
for name in ('PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
 'PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT','PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT',
 'PIONEER_Q38_RESIDENT_OUTPUT_OWNER','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT',
 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE',
 'PIONEER_Q38_WORK_MODE','PIONEER_Q38_NORMAL_CLOCK','PIONEER_Q38_GDN_MUTABLE_STATE','PIONEER_Q38_DECODER_RUNTIME'):
    cases += [(flags+['-D'+name+'='+str(value)],False) for value in (0,1)]
for selected,expected in cases:
    result=subprocess.run([str(cxx),'-x','c++','-E','-']+selected,input=role_input,capture_output=True)
    assert (result.returncode==0)==expected,(selected,result.stderr)
print('physical role perimeter',len(cases),'actual preprocessor cases',flush=True)
obj=out/'diagnostic76.o'
subprocess.run(args+flags+['-fcallgraph-info=su','-c',str(misc/'tenstorrent_pci_probe.cpp'),'-o',str(obj)],check=True)
profile=out/'profile.o'
subprocess.run(args+['-c',str(reference/'src/add-ons/kernel/drivers/misc/PioneerPciProfile.cpp'),'-o',str(profile)],check=True)
release=build/'objects/haiku/riscv64/release'
def library(name):return subprocess.check_output([str(cxx),'-print-file-name='+name],text=True).strip()
binary=out/'tenstorrent_pci_probe-dram-sweep-diagnostic76'
subprocess.run([str(cxx),'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',
    library('crtbeginS.o'),str(release/'system/glue/haiku_version_glue.o'),str(obj),str(profile),
    str(release/'system/kernel/kernel.so'),library('libgcc.a'),library('crtendS.o'),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-C','-S',str(binary)],text=True)
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device','api_version'):
    assert any(row.split()[-1:]==[name] for row in symbols.splitlines()),name
for required in ('PioneerDramReadSweepOwnerType<BlackholeDramWideQueuedReadSweepImage',
                 '::Execute<PioneerDramSweepNativeIO>',
                 'BlackholeDramSweepArc::Queue<PioneerDramSweepClockIO>',
                 'PioneerDramSweepNativeContext::ClosePci','map_physical_memory','delete_area','user_memcpy'):
    assert required in symbols,required
for forbidden in ('PioneerQ38Token','Q38WorkControl','ReferenceProducts','llama_','ggml_',
                  'PioneerResidentSession','EndResidentSession','BlackholePioneerMmioEnd',
                  'BlackholeDramSamplesProbe','PioneerDramSamplesIO::DramWrite'):
    assert forbidden not in symbols,forbidden
(out/'module-symbols.txt').write_text(symbols)
assembly=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-dr',str(obj)],text=True)
(out/'module-assembly.txt').write_text(assembly)
frames=[]
for line in obj.with_suffix('.su').read_text().splitlines():
    row=line.split('\t');assert len(row)==3 and row[2]=='static',line
    assert int(row[1])<=2048,('physical diagnostic frame',row)
    frames.append((int(row[1]),row[0]))
assert frames
graph=obj.with_suffix('.ci').read_text()
nodes={m[1]:m[2] for line in graph.splitlines()
       for m in [re.search(r'node: \{ title: "([^"]+)" label: "([^"]*)"',line)] if m}
edges={name:[] for name in nodes}
for line in graph.splitlines():
    match=re.search(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"',line)
    if match:edges.setdefault(match[1],[]).append(match[2])
assert nodes and edges
weights={name:int(m[1]) if m else 0 for name,label in nodes.items()
         for m in [re.search(r'([0-9]+) bytes \(static\)',label)]}
assert max(weights.values())==max(size for size,_ in frames),(weights,frames)
done={}
def stack(name,active):
    assert name not in active,('unexpected native recursion',name)
    if name in done:return done[name]
    value=weights.get(name,0)+max([stack(child,active|{name}) for child in edges.get(name,[])]+[0])
    done[name]=value;return value
max_owned=max(stack(name,set()) for name in nodes)
assert max_owned<=8192,('native-owned chain leaves insufficient kernel margin',max_owned)
print('compiler driver-TU direct call-chain maximum',max_owned,
      'bytes;profile/external/indirect calls unmeasured in this graph,not whole-kernel stack certification',flush=True)
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
libraries=['-L'+str(p) for p in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
client_obj=out/'client.o';client=out/'pioneer-dram-sweep-diagnostic76'
subprocess.run([str(cxx),'-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
    '-fstack-usage',*includes,'-c',str(here/'pioneer_dram_sweep.cpp'),'-o',str(client_obj)],check=True)
subprocess.run([str(cxx),*includes,*libraries,'-B'+str(release/'system/glue')+'/',
    '-L'+str(release/'system/libroot'),str(client_obj),'-o',str(client)],check=True)
client_symbols=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-C',str(client)],text=True)
assert 'ioctl' in client_symbols and 'close' in client_symbols
assert not any(word in client_symbols for word in ('llama_','ggml_','PioneerDramReadSweepOwner','PioneerStoreContext'))
for path in (binary,client):print(hashlib.sha256(path.read_bytes()).hexdigest()+' '+path.name,flush=True)
print('BUILD_ONLY physical role76/closed selectors/preflight-no-launch/bounded READ-only sweep/',
      'max driver-TU individual frame',max(size for size,_ in frames),'NO INSTALL/RUN/bandwidth/TPS',flush=True)
assert source_identity=={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in subjects},'source changed during build'
(out/'source-identities.txt').write_text(''.join(digest+' '+str(path.relative_to(project))+'\n' for path,digest in
    ((path,source_identity[str(path)]) for path in subjects)))
