#!/usr/bin/env python3
"""BUILD ONLY: queued-block full-model candidate; reproduce tag101 first."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Whole native checks required')
assert len(sys.argv)==3
build,out=map(pathlib.Path,sys.argv[1:])
assert build.is_absolute() and build.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
out.mkdir(mode=0o700)
sys.argv=[str(here/'build_qwen38_instruction_prefetch_full_offload_native.py'),str(build),str(out/'control101')]
parent=runpy.run_path(sys.argv[0])
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(out/'control101/tenstorrent_pci_probe-native')=='fbca121f3394491e702f66213f6d5e9d3e3fc38a749b07a9821e50a9543c64eb'
flags=parent['flags'][:]
old='-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010101ULL'
assert flags.count(old)==1
flags[flags.index(old)]='-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010102ULL'
flags+=['-DPIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4=1']
common,includes=parent['common'],parent['includes']
source=parent['source']
commands=[]
def run(command):
    commands.append(command)
    subprocess.run(command,check=True,cwd=out)
run([*common,*flags,*includes,'-MMD','-MF',str(out/'native.d'),'-c',str(source),'-o',str(out/'native.o')])
refusals=parent['refusals']+[
    ('missing-queued',['-UPIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4']),
    ('zero-queued',['-UPIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4','-DPIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4=0']),
    ('old-prefetch-package',['-DPIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH_Q4_PROGRAMS_H=1']),
    ('offboard-queued-image',['-DPIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE=1']),
    ('offboard-queued-test',['-DPIONEER_Q38_QUEUED_Q4_BLOCK_WORKER_TEST=1']),
    ('missing-prefetch',['-UPIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH']),
    ('zero-prefetch',['-UPIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH','-DPIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH=0']),
    ('old-replay-package',['-DPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY_Q4_PROGRAMS_H=1']),
    ('worker-image-alias',['-DPIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE=1']),
    ('missing-replay',['-UPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY']),
    ('zero-replay',['-UPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY','-DPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY=0']),
    ('old-profile-tag',['-UPIONEER_DRIVER_BUILD_TAG',old]),
    ('old-profile-package',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H=1']),
    ('old-counter-package',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_COUNTER_Q4_PROGRAMS_H=1'])]
for label,extra in refusals:
    result=subprocess.run([*common,*flags,*extra,*includes,'-E',str(source)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    assert result.returncode and any(t in result.stderr for t in (
        'Full offload requires','Full offload cannot alias','Full-card','Tag97 requires',
        'Tag98 requires','Tag99 requires','Tag100 requires','Tag101 requires','Tag102 requires')),label
(out/'role-refusals.txt').write_text(''.join('PASS '+label+'\n' for label,_ in refusals))
cxx,tool,release=parent['cxx'],parent['tool'],parent['release']
compiler_path=parent['compiler_path']
binary=out/'tenstorrent_pci_probe-native'
run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
     str(release/'system/glue/haiku_version_glue.o'),str(out/'native.o'),str(out/'control101/control100/control99/profile.o'),
     str(release/'system/kernel/kernel.so'),compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',str(binary)])
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(binary)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(binary)],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped(',
             'Q38FullOffloadCounter(','ObserveQ38TokenNativeFrontier(','ObserveCommittedFrontier<',
             'Q38TokenPhaseProfileControl(','ReadQ38TokenNativePhaseProfile(','ReadPhaseProfile<'):
    assert name in table,name
for name in ('llama_','ggml_','Q38WorkControl','ReferenceProducts','OriginalSource','SyntheticIO','NativeQ38PeerInstructionModel'):
    assert name not in table and name not in undefined,name
raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(out/'native.o')],text=True)
symbols={r[-1]:(int(r[0],16),int(r[1],16)) for r in map(str.split,raw.splitlines()) if len(r)==4}
rows=subprocess.check_output([tool+'objdump','-t',str(out/'native.o')],text=True)
sections={r[-1]:r[-3] for r in map(str.split,rows.splitlines()) if len(r)>=6 and r[-1] in symbols}
namespace='PioneerQ38FullOffloadQueuedQ4Programs'
header=(here/(namespace+'.h')).read_bytes()
assert hashlib.sha256(header).hexdigest()=='a43ddbef9de5155b94a4db36dbcc67b15181a3650d56cbdd3eb04b328f9e263b'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==15
for digest,label in arrays:
    names=[n for n in symbols if namespace in n and n.endswith(str(len(label))+label+'E')]
    assert len(names)==1,label
    name=names[0];first,size=symbols[name];target=out/(label+'.raw')
    run([tool+'objcopy','-O','binary','-j',sections[name],str(out/'native.o'),str(target)])
    assert hashlib.sha256(target.read_bytes()[first:first+size]).hexdigest()==digest,label
frames=[r.split('\t') for r in (out/'native.su').read_text().splitlines()]
assert frames and all(len(r)==3 and r[2]=='static' for r in frames)
token_frames=[int(r[1]) for r in frames if 'Token' in r[0] or 'FullOffload' in r[0]]
assert token_frames and max(token_frames)<=2048
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print(sha(binary),binary.name,flush=True)
print('PASS BUILD_ONLY queued-block tag102/selection10/all15 embedded arrays exact;tag101/tag100/tag99/default43 reproduced;'+
      str(len(refusals))+' role refusals;no CPU inference/reference;maxIndividualTokenFrame'+str(max(token_frames)),flush=True)
print('NOT installed or hardware-qualified;NO speed result',flush=True)
