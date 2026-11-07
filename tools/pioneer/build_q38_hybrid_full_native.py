#!/usr/bin/env python3
"""Build separate QGW5/QGW7 driver candidates; never install or run them."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

args=sys.argv[1:];fixed='--fixed-126' in args;matrix='--matrix-127' in args
if matrix:
 assert args.count('--matrix-127')==1 and not fixed;args.remove('--matrix-127')
if fixed:
 assert args.count('--fixed-126')==1;args.remove('--fixed-126')
if not __debug__ or len(args)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
 raise SystemExit('use --fixed-126|--matrix-127 ABSOLUTE_HAIKU_BUILD PINNED_HEADER NEW_ABSOLUTE_OUTPUT')
if not fixed and not matrix:
 raise SystemExit('tag125 is retired after its physical failure; select a distinct candidate')
sdk,header,out=map(Path,args);here=Path(__file__).resolve().parent;root=here.parent.parent
tag='0x2026110700010127ULL' if matrix else '0x2026110700010126ULL'
if not all(p.is_absolute() for p in (sdk,header,out)) or not sdk.is_dir() or not header.is_file() or out.exists() or out.is_symlink():
 raise SystemExit('invalid SDK, header or output')
headerDigest=hashlib.sha256(header.read_bytes()).hexdigest()
expected='03c7673557c60f37c9773c323de7857158f79ceb452ce2b18f4d7cbdae1ed3dc' if matrix else '2598f7a469ddd843ab1bbe732d0960dacaed819b4710b507430274b46c874f49'
if headerDigest!=expected:
 raise SystemExit('header does not match the selected independently pinned program package')
out.mkdir(mode=0o700);tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';cxx=str(tool)+'-g++'
release=sdk/'objects/haiku/riscv64/release'
headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
includes=[]
for path in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
 includes+=['-isystem',str(root/'headers/private'/path)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(headers),'-idirafter',str(headers/'posix'),'-I'+str(here)]
for path in [headers/'os',*sorted((headers/'os').iterdir())]:
 if path.is_dir():includes+=['-isystem',str(path)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
 '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
 '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic','-D_KERNEL_MODE=1']
flags=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in
 ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')]
candidateFlags=(['-DPIONEER_Q38_HYBRID_MATRIX_NATIVE_127=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
 '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1'] if matrix else ['-DPIONEER_Q38_HYBRID_FIXED_126=1'])
flags+=['-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG='+tag,
 '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"',*candidateFlags]
driver=root/'src/add-ons/kernel/drivers/misc';source=driver/'pioneer_q38_hybrid_full_driver.cpp';commands=[]
role=driver/'PioneerQ38HybridFullNativeRole.h'
declared=re.findall(r'^#define ([A-Z][A-Z0-9_]*) 1$',source.read_text(),re.M)
roleflags=['-D_KERNEL_MODE=1',*['-D'+name+'=1' for name in declared],
 *['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in
 ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')],
 '-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG='+tag,*candidateFlags]
def rolecheck(args,shouldPass):
 result=subprocess.run([cxx,'-std=gnu++17',*args,'-E','-x','c++',str(role)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
 assert (result.returncode==0)==shouldPass,result.stderr[:1000]
rolecheck(roleflags,True)
for flag in roleflags:
 if flag.startswith(('-DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE','-DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK',
  '-DTT_PIONEER_EXPERIMENTAL_Q38_ROWS','-DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED',
  '-DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD')):continue # lower-layer dependencies, checked by actual build
 rolecheck([v for v in roleflags if v!=flag],False)
for flag in ('PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
 'PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
 'PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_RESIDENT','PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0',
 'PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION','PIONEER_Q38_TOKEN_PUSHED_PROGRAMS',
 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT','PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC',
 'PIONEER_Q38_NATIVE_Q8_LAYOUT','PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE',
 'PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE'):
 rolecheck([*roleflags,'-D'+flag+'=1'],False)
rolecheck([v for v in roleflags if not v.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026102100010012ULL'],False)
rolecheck([v for v in roleflags if not v.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010125ULL'],False)
rolecheck([v for v in roleflags if not v.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=' + ('0x2026110700010126ULL' if matrix else '0x2026110700010127ULL')],False)
# Even a different driver including the shared reload protocol must not
# claim tag126. Removing either selector or changing its value must fail.
reloadHeader=driver/'PioneerDriverReloadProtocol.h'
selector='PIONEER_Q38_HYBRID_MATRIX_NATIVE_127' if matrix else 'PIONEER_Q38_HYBRID_FIXED_126'
fixedRole=['-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG='+tag,
           '-DPIONEER_Q38_HYBRID_FULL_NATIVE=1','-D'+selector+'=1']
for removed in (None,2,3):
 probe=[v for i,v in enumerate(fixedRole) if i!=removed]
 result=subprocess.run([cxx,'-std=gnu++17',*probe,*includes,'-E','-x','c++',str(reloadHeader)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
 assert (result.returncode==0)==(removed is None),result.stderr[:1000]
for index in (2,3):
 probe=list(fixedRole);probe[index]=probe[index].split('=')[0]+'=0'
 result=subprocess.run([cxx,'-std=gnu++17',*probe,*includes,'-E','-x','c++',str(reloadHeader)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
 assert result.returncode,'tag126 accepted a disabled selector'
for chosen in ('PIONEER_Q38_HYBRID_FULL_NATIVE',selector):
 late='#include "'+str(reloadHeader)+'"\n#undef '+chosen+'\n#include "'+str(reloadHeader)+'"\n'
 result=subprocess.run([cxx,'-std=gnu++17',*fixedRole,*includes,'-E','-x','c++','-'],input=late,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
 assert result.returncode and ('Tag127 is reserved' if matrix else 'Tag126 is reserved') in result.stderr,'late candidate role change accepted'
def run(*args):
 commands.append([str(v) for v in args]);subprocess.run(commands[-1],check=True,cwd=out)
protocolTest=out/'protocol-test'
run(os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
 '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_FULL_PROTOCOL_TEST=1',
 *candidateFlags,
 here/'tests/pioneer_q38_full_offload_protocol_test.cpp','-o',protocolTest)
run(protocolTest)
dep=subprocess.check_output([*common,*flags,*includes,'-MM',str(source)],text=True)
dependencies={Path(p).resolve() for p in dep.replace('\\\n','').split(':',1)[1].split() if Path(p).resolve().is_relative_to(root)}
dependencies.add(Path(__file__).resolve());frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
run(*common,*includes,'-c',driver/'PioneerPciProfile.cpp','-o',out/'profile.o')
run(*common,*flags,*includes,'-c',source,'-o',out/'native.o')
def compiler_path(name):return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
binary=out/('tenstorrent_pci_probe-matrix127' if matrix else 'tenstorrent_pci_probe-hybrid126')
run(cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
 release/'system/glue/haiku_version_glue.o',out/'native.o',out/'profile.o',release/'system/kernel/kernel.so',
 compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',binary)
symbols=subprocess.check_output([str(tool)+'-nm','-C','--defined-only',str(binary)],text=True)
undefined=subprocess.check_output([str(tool)+'-nm','-u',str(binary)],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38HybridUploadControl(','Q38SpanControl(','Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped('):
 if name not in symbols:raise RuntimeError('missing candidate symbol '+name)
for name in ('llama_','ggml_','Q38WorkControl','ReferenceProducts','OriginalSource','SyntheticIO'):
 if name in symbols or name in undefined:raise RuntimeError('unexpected CPU/reference symbol '+name)
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z0-9]+)\[\]=',header.read_text())
assert len(arrays)==15
raw=subprocess.check_output([str(tool)+'-nm','-S','--defined-only',str(out/'native.o')],text=True)
named={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
rows=subprocess.check_output([str(tool)+'-objdump','-t',str(out/'native.o')],text=True)
sections={row[-1]:row[-3] for row in map(str.split,rows.splitlines()) if len(row)>=6 and row[-1] in named};extracted={}
for digest,label in arrays:
 names=[name for name in named if 'PioneerQ38HybridPrograms' in name and name.endswith(str(len(label))+label+'E')]
 assert len(names)==1,(label,names)
 name=names[0];section=sections[name];first,size=named[name]
 if section not in extracted:
  target=out/('section-'+str(len(extracted)));run(str(tool)+'-objcopy','-O','binary','-j',section,out/'native.o',target);extracted[section]=target.read_bytes()
 assert hashlib.sha256(extracted[section][first:first+size]).hexdigest()==digest,label
frames=[line.split('\t') for line in (out/'native.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
selected=[int(row[1]) for row in frames if 'Token' in row[0] or 'HybridUpload' in row[0]];assert selected and max(selected)<=2048
assert headerDigest==hashlib.sha256(header.read_bytes()).hexdigest()
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
print(('MATRIX127_BINARY_SHA256' if matrix else 'HYBRID126_BINARY_SHA256'),hashlib.sha256(binary.read_bytes()).hexdigest())
print('PASS candidate build: exact '+('tag127/QGW7' if matrix else 'tag126/QGW5')+', 15 pinned sections, no CPU inference symbols; NOT installed, run, numerically qualified or speed-tested')
