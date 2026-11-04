#!/usr/bin/env python3
"""Build/inspect the distinct full-offload candidate; NEVER install or run it."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__
args=sys.argv[1:];batched='--batched' in args
if batched:
    assert args.count('--batched')==1;args.remove('--batched')
metrics='--metrics' in args
if metrics:
    assert batched and args.count('--metrics')==1;args.remove('--metrics')
vector_q4='--vector-q4' in args
if vector_q4:
    assert metrics and args.count('--vector-q4')==1;args.remove('--vector-q4')
assert len(args)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,args)
assert build.is_absolute() and build.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
root=here.parent.parent
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release'
tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx=tool+'g++'
includes=[]
for path in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/path)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),
           '-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix'),'-I'+str(here)]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
        '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
        '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic','-D_KERNEL_MODE']
native=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in
        ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')]
native+=['-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG='+
         ('0x2026110400010092ULL' if vector_q4 else '0x2026110400010090ULL' if metrics else '0x2026110400010089ULL' if batched else '0x2026110400010088ULL')]
if batched:native+=['-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1']
if metrics:native+=['-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1']
if vector_q4:native+=['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1']
driver=root/'src/add-ons/kernel/drivers/misc'
commands=[]
def run(command):
    commands.append(command)
    subprocess.run(command,check=True,cwd=out)
for name,source,flags in [('profile',driver/'PioneerPciProfile.cpp',[]),
                          ('native',driver/'pioneer_q38_full_offload_driver.cpp',native),
                          ('default',driver/'tenstorrent_pci_probe.cpp',[])]:
    run([*common,*flags,*includes,'-MMD','-MF',str(out/(name+'.d')),'-c',str(source),'-o',str(out/(name+'.o'))])
refusals=[('missing-reload',['-UTT_PIONEER_DRIVER_RELOAD']),
          ('old-diagnostic-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026103100010075ULL']),
          ('fast-math',['-ffast-math'])]
if batched:
    refusals += [('missing-batched',['-UPIONEER_Q38_FULL_OFFLOAD_BATCHED']),
                 ('zero-batched',['-UPIONEER_Q38_FULL_OFFLOAD_BATCHED','-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=0']),
                 ('original-full-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010088ULL'])]
else:
    refusals += [('batched-with-original-tag',['-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1'])]
if metrics:
    refusals += [('missing-metrics',['-UPIONEER_Q38_FULL_OFFLOAD_METRICS']),
                 ('zero-metrics',['-UPIONEER_Q38_FULL_OFFLOAD_METRICS','-DPIONEER_Q38_FULL_OFFLOAD_METRICS=0']),
                 ('old-batched-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010089ULL'])]
else:
    refusals += [('metrics-with-old-tag',['-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1'])]
if vector_q4:
    refusals += [('missing-vector-q4',['-UPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4']),
                 ('zero-vector-q4',['-UPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4','-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0']),
                 ('old-metrics-vector-q4',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010090ULL']),
                 ('resident-vector-q4',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0'])]
else:
    refusals += [('vector-q4-with-old-tag',['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1'])]
for macro in ('PIONEER_Q38_TOKEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE',
              'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
              'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_NATIVE_Q8_LAYOUT',
              'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
              'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE','PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE',
              'PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT'):
    refusals.append((macro+'-defined0',['-D'+macro+'=0']))
for label,extra in refusals:
    result=subprocess.run([*common,*native,*extra,*includes,'-E',str(driver/'pioneer_q38_full_offload_driver.cpp')],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    assert result.returncode and any(t in result.stderr for t in
        ('Full offload requires','Full offload cannot alias','Full-card metrics requires','Full-card vector Q4 requires')),label
(out/'role-refusals.txt').write_text('\n'.join('PASS '+label for label,_ in refusals)+'\n')
def compiler_path(name):
    return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
for name in ('native','default'):
    run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
         str(release/'system/glue/haiku_version_glue.o'),str(out/(name+'.o')),str(out/'profile.o'),
         str(release/'system/kernel/kernel.so'),compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',str(out/('tenstorrent_pci_probe-'+name))])
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert sha(out/'tenstorrent_pci_probe-default')=='2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
if not batched:
    assert sha(out/'tenstorrent_pci_probe-native')=='3c5769d2af54798c8fdbe672f3ed25f785cde0b3752253fe591fa06e66169a76'
elif not metrics:
    assert sha(out/'tenstorrent_pci_probe-native')=='de688dc5e682fa7714ac53450657c92052b139831eed7dce91b2300f82449eb6'
elif not vector_q4:
    assert sha(out/'tenstorrent_pci_probe-native')=='2551b50a52f6a2f7c98ca391fa1742640d5d777f23379663733679980bd643d7'
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(out/'tenstorrent_pci_probe-native')],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(out/'tenstorrent_pci_probe-native')],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):
    assert name in table,name
if metrics:
    for name in ('Q38FullOffloadCounter(','ObserveQ38TokenNativeFrontier(','ObserveCommittedFrontier<'):
        assert name in table,name
for name in ('llama_','ggml_','Q38WorkControl','ReferenceProducts','OriginalSource','SyntheticIO','NativeQ38PeerInstructionModel'):
    assert name not in table and name not in undefined,name
# The already-existing immutable G3 package must be present,not silently
# replaced by a CPU backend or a different diagnostic package.
raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(out/'native.o')],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
rows=subprocess.check_output([tool+'objdump','-t',str(out/'native.o')],text=True)
sections={r[-1]:r[-3] for r in map(str.split,rows.splitlines()) if len(r)>=6 and r[-1] in symbols}
old_header=(here/('PioneerQ38TokenWideGdnPrograms.h' if batched else 'PioneerQ38TokenPushedPrograms.h')).read_bytes()
assert hashlib.sha256(old_header).hexdigest()==('d285c9adb421fc0010d8ad48371eb58ca1480fc0d787f7c147afd4a93457a787'
    if batched else '692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f')
old_arrays={label:digest for digest,label in re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',old_header.decode())}
namespace='PioneerQ38FullOffloadVectorQ4Programs' if vector_q4 else 'PioneerQ38FullOffloadBatchedPrograms' if batched else 'PioneerQ38FullOffloadPrograms'
header=(here/(namespace+'.h')).read_bytes()
assert hashlib.sha256(header).hexdigest()==('3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389' if vector_q4 else '91624761b95d281cb9dd4493790b27a1c5d88d7c327d6df894711561b76b4c84'
    if batched else 'fcff4279355cf5cfa1de048aa6bb9ed5fa4d7dca0f3dc9a7669cbd421be2a1cd')
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==(14 if batched else 13)
for digest,label in arrays:
    if label.startswith('Peer') and not vector_q4:assert digest==old_arrays[label]
    names=[n for n in symbols if namespace in n and n.endswith(str(len(label))+label+'E')]
    assert len(names)==1
    name=names[0];first,size=symbols[name];target=out/(label+'.raw')
    run([tool+'objcopy','-O','binary','-j',sections[name],str(out/'native.o'),str(target)])
    assert hashlib.sha256(target.read_bytes()[first:first+size]).hexdigest()==digest
frames=[r.split('\t') for r in (out/'native.su').read_text().splitlines()]
assert frames and all(len(r)==3 and r[2]=='static' for r in frames)
token_frames=[int(r[1]) for r in frames if 'Token' in r[0] or 'FullOffload' in r[0]]
assert token_frames and max(token_frames)<=2048
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
for name in ('native','default'):
    print(sha(out/('tenstorrent_pci_probe-'+name)),'tenstorrent_pci_probe-'+name,flush=True)
print('PASS BUILD_ONLY distinct full-card '+('vector Q4 tag92/all14 arrays exact/new vector calculation peers/unchanged5-EOG ROOT' if vector_q4 else 'metrics tag90/all14 arrays exact/ten wide peers unchanged' if metrics else 'batched tag89/all14 arrays exact/ten wide peers unchanged' if batched else 'tag88/all13 arrays exact/nine G3 peers unchanged')+
      ',full5-EOG firmware/default43 BYTE-UNCHANGED,'+str(len(refusals))+' role refusals,no CPU model/reference/ggml Work imports;maxIndividualTokenFrame'+str(max(token_frames)),flush=True)
print('NOT installed,physical/numerical/Assistant qualified or a speed result;whole-kernel stack not certified',flush=True)
