#!/usr/bin/env python3
"""Build separate physical DEBUG69;NO installation,card access or Work grant.

Revalidate unchanged old65 and the inert NODRV witness FIRST. Link only the
actual production translation unit with its distinct conventional ABI.
"""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

if 'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE' in os.environ:
    raise SystemExit(2)

if any(k in os.environ for k in ('PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE','PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE')):
    raise SystemExit(2)

if 'PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE' in os.environ:
    raise SystemExit(2)

if any(k in os.environ for k in ('PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE','PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE','PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE','PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE','PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE','PIONEER_Q38_MIXED_WORKER_IMAGE','PIONEER_Q38_MIXED_CAPSULE_IMAGE','PIONEER_Q38_DENSE_PARTITION_IMAGE','PIONEER_Q38_DENSE_PROJECTION_IMAGE','PIONEER_Q38_DENSE_ATTENTION_IMAGE','PIONEER_Q38_DENSE_PREPARE_IMAGE','PIONEER_Q38_RESIDENT_ADVANCE_IMAGE','PIONEER_Q38_RESIDENT_CONTINUE_IMAGE')):
    raise SystemExit(2)

if len(sys.argv)!=3 or any(k in os.environ for k in ('PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT','PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE')) \
        or any(k.startswith('PIONEER_') and v!='0' for k,v in os.environ.items()):
    raise SystemExit(2)
qualified,output=map(pathlib.Path,sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
output.mkdir(mode=0o700)
saved=list(sys.argv)
try:
    sys.argv=[str(here/'tests/run_pioneer_q38_resident_layer_native_context_compile.py'),
              str(qualified),str(output/'context-NODRV-control')]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
args=list(base['args'])
roles=''.join((kernel/name).read_text() for name in (
    'PioneerQ38ResidentLayerValidationRole.h','PioneerQ38ResidentLayerOwnerRole.h','PioneerQ38ResidentLayerNativeRole.h'))
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',roles))
assert {'PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT',
        'PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT','PIONEER_Q38_RESIDENT_LAYER_OWNER',
        'PIONEER_Q38_RESIDENT_OUTPUT_OWNER','TT_PIONEER_DRIVER_RELOAD'}<=parents
assert len(parents)==31,parents
for name in sorted(parents):
    key='-D'+name
    args=[a for a in args if a!=key and not a.startswith(key+'=')]+[key+'=1']
tags=[i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags)==1
args[tags[0]]='-DPIONEER_DRIVER_BUILD_TAG=0x2026102600010069ULL'
assert not any(a.startswith(('-DPIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY',
                            '-DPIONEER_Q38_CPU_VALIDATION','-DPIONEER_Q38_WORK_INFERENCE')) for a in args)
obj=output/'resident-layer-debug69.o'
args[args.index('-c')+1]=str(kernel/'tenstorrent_pci_probe.cpp')
args[args.index('-o')+1]=str(obj)
subprocess.run(args,check=True,env=base['clean'])
current=base['frames'](obj.with_suffix('.su'))
for name,size in current.items():
    assert size<=2048 or name in base['previous'] and size<=base['previous'][name],(name,size)
sizes=[size for name,size in current.items() if 'ResidentLayer' in name or 'ProbeControl(' in name]
assert sizes and max(sizes)<=2048
for required in ('Q38ResidentLayerValidationControl(','ProbeControl('):
    assert any(required in name for name in current),(required,'actual physical control must be emitted')
toolchain=pathlib.Path(args[0]).parent
normal={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
def names(path):
    table=subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-nm'),'--defined-only',str(path)],text=True)
    return {row[-1] for row in map(str.split,table.splitlines()) if row}
def audit(path):
    defined=names(path);assert normal<=defined
    assert not any(n.startswith(('pioneer_q38_layer_context_only_','pioneer_layer_context_',
                                'pioneer_q38_output_context_only_','pioneer_output_debug_')) for n in defined)
    for forbidden in ('DecoderOriginalSource','OriginalSource','CompareResidentLayer','ReadReferencePacked',
                      'SiluInstructionReference','ResidentLayerImportLearnedBytes','ResidentLayerValidationSession'):
        assert not any(forbidden in n for n in defined),(path,forbidden,'CPU observer/client/TEST bytes must never enter driver')
audit(obj)
original=base['original'];old_object=original[original.index('-o')+1]
links=[a for a in base['commands'] if '-shared' in a and old_object in a];assert len(links)==1
link=list(links[0])
assert link[:4]==[args[0],'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[-2]=='-o' and len(link[4:-2])==7
assert [pathlib.Path(p).name for p in link[4:-2]]==[
    'crtbeginS.o','haiku_version_glue.o','tenstorrent_pci_probe.o','PioneerPciProfile.o','kernel.so','libgcc.a','crtendS.o']
assert all(pathlib.Path(p).is_file() for p in link[4:-2])
candidate=output/'tenstorrent_pci_probe-resident-layer-DEBUG69'
link=[str(obj) if a==old_object else a for a in link];link[-1]=str(candidate)
subprocess.run(link,check=True,env=base['clean']);audit(candidate)
print('PASS ACTUAL production DEBUG69/namespace20/fixed6521088-byte QLV1/closed10-operation dispatcher;conventional ABI,NO test/NODRV/old68 alias;maxNewFrame='+str(max(sizes)))
print(hashlib.sha256(candidate.read_bytes()).hexdigest()+'  '+str(candidate))
print('BUILT ONLY;public-control/source/client containment and physical qualification REQUIRED BEFORE Work;NO full-model or inference-speed grant')
