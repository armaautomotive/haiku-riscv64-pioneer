#!/usr/bin/env python3
"""Build/inspect the distinct full-offload candidate; NEVER install or run it."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,sys.argv[1:])
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
native+=['-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010088ULL']
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
for macro in ('PIONEER_Q38_TOKEN_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE',
              'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
              'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_NATIVE_Q8_LAYOUT',
              'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
              'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE','PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE'):
    refusals.append((macro+'-defined0',['-D'+macro+'=0']))
for label,extra in refusals:
    result=subprocess.run([*common,*native,*extra,*includes,'-E',str(driver/'pioneer_q38_full_offload_driver.cpp')],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    assert result.returncode and ('Full offload requires' in result.stderr or 'Full offload cannot alias' in result.stderr),label
(out/'role-refusals.txt').write_text('\n'.join('PASS '+label for label,_ in refusals)+'\n')
def compiler_path(name):
    return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
for name in ('native','default'):
    run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
         str(release/'system/glue/haiku_version_glue.o'),str(out/(name+'.o')),str(out/'profile.o'),
         str(release/'system/kernel/kernel.so'),compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',str(out/('tenstorrent_pci_probe-'+name))])
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
assert sha(out/'tenstorrent_pci_probe-default')=='2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(out/'tenstorrent_pci_probe-native')],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(out/'tenstorrent_pci_probe-native')],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):
    assert name in table,name
for name in ('llama_','ggml_','Q38WorkControl','ReferenceProducts','OriginalSource','SyntheticIO','NativeQ38PeerInstructionModel'):
    assert name not in table and name not in undefined,name
# The already-existing immutable G3 package must be present,not silently
# replaced by a CPU backend or a different diagnostic package.
raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(out/'native.o')],text=True)
symbols={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
rows=subprocess.check_output([tool+'objdump','-t',str(out/'native.o')],text=True)
sections={r[-1]:r[-3] for r in map(str.split,rows.splitlines()) if len(r)>=6 and r[-1] in symbols}
old_header=(here/'PioneerQ38TokenPushedPrograms.h').read_bytes()
assert hashlib.sha256(old_header).hexdigest()=='692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f'
old_arrays={label:digest for digest,label in re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',old_header.decode())}
header=(here/'PioneerQ38FullOffloadPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='fcff4279355cf5cfa1de048aa6bb9ed5fa4d7dca0f3dc9a7669cbd421be2a1cd'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode())
assert len(arrays)==13
for digest,label in arrays:
    if label.startswith('Peer'):assert digest==old_arrays[label]
    names=[n for n in symbols if 'PioneerQ38FullOffloadPrograms' in n and n.endswith(str(len(label))+label+'E')]
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
print('PASS BUILD_ONLY distinct full-card route,all13 full5-EOG firmware arrays exact/nine peers G3-unchanged,default43 BYTE-UNCHANGED,'+str(len(refusals))+' role refusals,no CPU model/reference/ggml Work imports;maxIndividualTokenFrame'+str(max(token_frames)),flush=True)
print('NOT installed,physical/numerical/Assistant qualified or a speed result;whole-kernel stack not certified',flush=True)
