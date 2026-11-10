#!/usr/bin/env python3
"""Build the separate QGWD diagnostic driver; NEVER install or execute."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

if len(sys.argv)!=4 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: build_q38_qgwd_candidate141.py ABS_SDK ABS_PINNED_HEADER NEW_ABS_OUTPUT')
sdk,header,out=map(Path,sys.argv[1:])
if not all(p.is_absolute() for p in (sdk,header,out)) or not sdk.is_dir() or not header.is_file() or out.exists() or out.is_symlink():
    raise SystemExit('invalid build paths')
if hashlib.sha256(header.read_bytes()).hexdigest()!='84b7bda4b16924be3da350499a0376e2acb0ce1b47a73ee49fcc914829041c72':
    raise SystemExit('QGWD candidate program header is not pinned')
here=Path(__file__).resolve().parent;root=here.parents[1];driver=root/'src/add-ons/kernel/drivers/misc'
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';cxx=str(tool)+'-g++'
release=sdk/'objects/haiku/riscv64/release'
headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
includes=[]
for part in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/part)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(headers),
           '-idirafter',str(headers/'posix'),'-I'+str(here)]
for path in [headers/'os',*sorted((headers/'os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-fno-exceptions','-fno-rtti','-fstack-usage','-ffreestanding','-finline','-fno-semantic-interposition',
 '-fno-tree-vectorize','-fno-use-cxa-atexit','-fno-strict-aliasing','-fno-delete-null-pointer-checks',
 '-mcmodel=medany','-fpic','-D_KERNEL_MODE=1']
names=('PIONEER_Q38_QGWD_NATIVE_141','PIONEER_Q38_QGWD_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128',
 'PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_QGWD_PROGRAMS_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION','PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE',
 'PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION','PIONEER_Q38_QGWD_CONTROL_INSPECTION',
 'PIONEER_Q38_QGWD_SPAN_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION','PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION','PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION',
 'PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION','PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION')
flags=['-D'+name+'=1' for name in names]
flags+=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS',
 'COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')]
flags+=['-DTT_PIONEER_DRIVER_RELOAD=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010141ULL',
        '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
source=driver/'pioneer_q38_hybrid_full_driver.cpp'
dep=subprocess.check_output([*common,*flags,*includes,'-MM',str(source)],text=True)
deps={Path(p).resolve() for p in dep.replace('\\\n','').split(':',1)[1].split() if Path(p).resolve().is_relative_to(root)}
deps.update((source.resolve(),(driver/'PioneerPciProfile.cpp').resolve(),Path(__file__).resolve(),header.resolve()))
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in deps}
out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True,cwd=out)
run(*common,*includes,'-c',driver/'PioneerPciProfile.cpp','-o',out/'profile.o')
run(*common,*flags,*includes,'-c',source,'-o',out/'native.o')
def compiler_path(name):return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
binary=out/'tenstorrent_pci_probe-qgwd-141'
run(cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
 release/'system/glue/haiku_version_glue.o',out/'native.o',out/'profile.o',release/'system/kernel/kernel.so',
 compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',binary)
symbols=subprocess.check_output([str(tool)+'-nm','-C','--defined-only',str(binary)],text=True)
undefined=subprocess.check_output([str(tool)+'-nm','-u',str(binary)],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38QgwdUploadControlInspection(','Q38SpanControl(','Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped('):
    if name not in symbols:raise RuntimeError('candidate missing required symbol '+name)
for name in ('llama_','ggml_','Q38WorkControl','Q38HybridUploadControl(','ReferenceProducts','OriginalSource','SyntheticIO'):
    if name in symbols or name in undefined:raise RuntimeError('unexpected CPU/reference symbol '+name)
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z0-9]+)\[\]=',header.read_text())
if len(arrays)!=15:raise RuntimeError('wrong number of pinned QGWD sections')
raw=subprocess.check_output([str(tool)+'-nm','-S','--defined-only',str(out/'native.o')],text=True)
named={r[-1]:(int(r[0],16),int(r[1],16)) for r in map(str.split,raw.splitlines()) if len(r)==4}
table=subprocess.check_output([str(tool)+'-objdump','-t',str(out/'native.o')],text=True)
sections={r[-1]:r[-3] for r in map(str.split,table.splitlines()) if len(r)>=6 and r[-1] in named};extracted={}
for digest,label in arrays:
    found=[n for n in named if 'PioneerQ38HybridPrograms' in n and n.endswith(str(len(label))+label+'E')]
    if len(found)!=1:raise RuntimeError('missing section '+label)
    name=found[0];section=sections[name];first,size=named[name]
    if section not in extracted:
        target=out/('section-'+str(len(extracted)));run(str(tool)+'-objcopy','-O','binary','-j',section,out/'native.o',target)
        extracted[section]=target.read_bytes()
    if hashlib.sha256(extracted[section][first:first+size]).hexdigest()!=digest:raise RuntimeError('changed section '+label)
frames=[line.split('\t') for line in (out/'native.su').read_text().splitlines()]
if not frames or any(len(r)!=3 or r[2]!='static' or int(r[1])>4096 for r in frames):raise RuntimeError('driver stack limit')
selected=[int(r[1]) for r in frames if 'TokenNative' in r[0] or 'QgwdUpload' in r[0]]
if not selected or max(selected)>2048:raise RuntimeError('QGWD/token stack limit')
if any(hashlib.sha256(p.read_bytes()).hexdigest()!=digest for p,digest in frozen.items()):raise RuntimeError('source changed during build')
print('QGWD_CANDIDATE_141_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('EXPERIMENTAL BUILD ONLY: NOT INSTALLED, TESTED ON DEVICE, OR QUALIFIED')
