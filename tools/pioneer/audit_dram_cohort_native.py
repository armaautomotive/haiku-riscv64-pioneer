#!/usr/bin/env python3
"""Frozen private SDK object/code-array/partial-stack/late-role audit only."""
import hashlib
import importlib.util
import json
import os
import pathlib
import re
import subprocess
import sys

assert len(sys.argv)==5 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,native,image,out=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,native,image,out)) and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent;misc=here.parents[1]/'src/add-ons/kernel/drivers/misc'
obj=native/'cohort-DRCNODRV.o'
assert hashlib.sha256(obj.read_bytes()).hexdigest()=='5028658b279b948ad32f1fd555336ce9b136d9e4907b4fdacdadb39082caa5f9'
identity=json.loads((native/'source-identities.json').read_text())
assert all(hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==digest for p,digest in identity.items())
spec=importlib.util.spec_from_file_location('cohort_generator',here/'generate_dram_cohort_read_sweep_image.py')
gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
assert gen.render(image/'program.elf').encode('ascii')==(misc/'BlackholeDramCohortReadSweepImage.h').read_bytes()
out.mkdir(mode=0o700);tool=sdk/'cross-tools-riscv64/bin';prefix=str(tool/'riscv64-unknown-haiku-')
array=out/'actual-native-code-array.raw'
subprocess.run([prefix+'objcopy','--dump-section=.rodata._ZZN33BlackholeDramCohortReadSweepImage4CodeEjE4text='+str(array),str(obj)],check=True)
assert array.read_bytes()==(image/'text.bin').read_bytes() and len(array.read_bytes())==8644
assert hashlib.sha256(array.read_bytes()).hexdigest()==gen.TEXT_SHA
# Compiler direct edges only. Unknown/indirect/kernel/IRQ/caller frames remain
# explicitly unqualified even if every known chain fits this local budget.
graph=obj.with_suffix('.ci').read_text()
nodes={m[1]:m[2] for row in graph.splitlines() for m in [re.search(r'node: \{ title: "([^"]+)" label: "([^"]*)"',row)] if m}
edges={name:[] for name in nodes}
for row in graph.splitlines():
    m=re.search(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"',row)
    if m:edges.setdefault(m[1],[]).append(m[2])
weights={name:int(m[1]) if m else 0 for name,label in nodes.items()
         for m in [re.search(r'([0-9]+) bytes \(static\)',label)]}
assert nodes and edges and max(weights.values())==1296
done={}
def stack(name,active):
    assert name not in active,('unexpected recursion',name)
    if name in done:return done[name]
    value=weights.get(name,0)+max([stack(child,active|{name}) for child in edges.get(name,[])]+[0])
    done[name]=value;return value
known=max(stack(name,set()) for name in nodes)
assert known<=8192
unresolved=sorted({child for name in edges for child in edges[name] if child not in weights or not weights[child]})
report={'frozenObject':'5028658b279b948ad32f1fd555336ce9b136d9e4907b4fdacdadb39082caa5f9',
        'maxKnownDirectChainBytes':known,'maxIndividualFrame':1296,'unresolved':unresolved,
        'wholeKernelStackQualified':False,'nativeLifecycleOrHardwareQualified':False}
(out/'partial-stack.json').write_text(json.dumps(report,indent=2)+'\n')
flags=['-DTT_PIONEER_DRIVER_RELOAD','-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH',
 '-DPIONEER_DRIVER_BUILD_TAG=0x4452434e4f445256ULL','-DPIONEER_DRAM_COHORT_NATIVE_CONTEXT=1',
 '-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY=1']
header='#include "'+str(misc/'PioneerDramReadSweepNativeRole.h')+'"\n'
cases=[(header,flags,True)]
for changed in ('PIONEER_DRAM_COHORT_NATIVE_CONTEXT','PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT','PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY'):
    for value in ('0','2',''):
        cases.append((header+'#undef '+changed+'\n#define '+changed+' '+value+'\n'+header,flags,False))
cases.append((header+'#undef PIONEER_DRAM_COHORT_NATIVE_CONTEXT\n'+header,flags,False))
legacy=[x for x in flags if not x.startswith(('-DPIONEER_DRAM_COHORT_NATIVE_CONTEXT=','-DPIONEER_DRIVER_BUILD_TAG='))]
legacy+=['-DPIONEER_DRIVER_BUILD_TAG=0x4452534e4f445256ULL']
cases.append((header,legacy,True))
cases.append((header+'#undef PIONEER_DRIVER_BUILD_TAG\n#define PIONEER_DRIVER_BUILD_TAG 0x4452434e4f445256ULL\n'
              '#define PIONEER_DRAM_COHORT_NATIVE_CONTEXT 1\n'+header,legacy,False))
for source,selected,expected in cases:
    r=subprocess.run([prefix+'g++','-std=gnu++17','-x','c++','-fsyntax-only','-',*selected],input=source.encode(),capture_output=True)
    assert (r.returncode==0)==expected,(source,selected,r.stderr)
print('PASS_COHORT_FROZEN_NATIVE_AUDIT actualCodeArrayBytes8644 fullELFAndHeader1 frozenWholeObject1 currentSourcePins1 '
      'syntaxRolePositives2 lateRefusals'+str(len(cases)-2)+' knownDirectChain'+str(known)+' unresolved'+str(len(unresolved))+
      ';wholeKernelStackQualified0 NO native-lifecycle/clock-frequency/physical/bandwidth/model/Work grant',flush=True)
