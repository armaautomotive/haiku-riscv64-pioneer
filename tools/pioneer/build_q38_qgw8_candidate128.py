#!/usr/bin/env python3
"""Build isolated tag128, tag129 or tag131 candidates; NEVER install or run."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

if not __debug__ or len(sys.argv) not in (4,5) or (len(sys.argv)==5 and sys.argv[4] not in ('--hidden-129','--trace-131','--dense-stage-132','--dense-input-133','--dense-value-134')) \
 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute Haiku build, pinned header, new absolute output, optional --hidden-129 or --trace-131 required')
hidden=len(sys.argv)==5 and sys.argv[4]=='--hidden-129'
stage=len(sys.argv)==5 and sys.argv[4]=='--dense-stage-132'
input_trace=len(sys.argv)==5 and sys.argv[4]=='--dense-input-133'
value_trace=len(sys.argv)==5 and sys.argv[4]=='--dense-value-134'
trace=stage or input_trace or value_trace or (len(sys.argv)==5 and sys.argv[4]=='--trace-131')
sdk,header,out=map(Path,sys.argv[1:4]);here=Path(__file__).resolve().parent;root=here.parent.parent
if not all(p.is_absolute() for p in (sdk,header,out)) or not sdk.is_dir() or not header.is_file() or out.exists():
    raise SystemExit('invalid paths')
expected_header='ed4f548f42d1ead00b9c2a6a6ba4afad3e3d8acebecf3ac437b8b2bb7a76cf92' if value_trace else '81cc1a2be97a4246d7ae9a9a7e9594e589f8af795d49912031f0d776c754e412' if input_trace else 'd8dede4df3cbfa0b88828fc42dde395aa4647a682827483d4954ba51ab479294' if stage else 'bcb6b4b86349b236c94e1871763b80f38ff72a3bf369e7467baa2ec83767af24' if trace else '8ddeb0cfa01857557be9416fa61c8029040570cf67bd412e6dd1f59fdc32eecb'
if hashlib.sha256(header.read_bytes()).hexdigest()!=expected_header:
    raise SystemExit('candidate program header is not pinned')
out.mkdir(mode=0o700)
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';cxx=str(tool)+'-g++'
release=sdk/'objects/haiku/riscv64/release'
headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
includes=[]
for path in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/path)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(headers),'-idirafter',str(headers/'posix'),'-I'+str(here)]
for path in [headers/'os',*sorted((headers/'os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 '-fno-exceptions','-fno-rtti','-fstack-usage','-fcallgraph-info=su','-ffreestanding','-finline','-fno-semantic-interposition',
 '-fno-tree-vectorize','-fno-use-cxa-atexit','-fno-strict-aliasing','-fno-delete-null-pointer-checks',
 '-mcmodel=medany','-fpic','-D_KERNEL_MODE=1']
names=('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')
flags=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name+'=1' for name in names]
names=('PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128','PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
 'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION',
 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
flags+=['-D'+name+'=1' for name in names]
tag='0x2026110700010134ULL' if value_trace else '0x2026110700010133ULL' if input_trace else '0x2026110700010132ULL' if stage else '0x2026110700010131ULL' if trace else '0x2026110700010129ULL' if hidden else '0x2026110700010128ULL'
flags+=['-DTT_PIONEER_DRIVER_RELOAD=1', '-DPIONEER_DRIVER_BUILD_TAG='+tag,
 '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"']
if hidden:flags+=['-DPIONEER_Q38_QGW8_HIDDEN_NATIVE_129=1']
if trace:flags+=['-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1']
if stage:flags+=['-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1']
if input_trace:flags+=['-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=1']
if value_trace:flags+=['-DPIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134=1']
driver=root/'src/add-ons/kernel/drivers/misc'
source=driver/'pioneer_q38_hybrid_full_driver.cpp'
declared=re.findall(r'^#define ([A-Z][A-Z0-9_]*) 1$',source.read_text(),re.M)
role=['-D_KERNEL_MODE=1',*['-D'+name+'=1' for name in declared],*flags]
role_header=here/'PioneerQ38Qgw8NativeInspectionRole.h'
def rolecheck(values,expected):
    result=subprocess.run([cxx,'-std=gnu++17',*values,'-E','-x','c++',str(role_header)],capture_output=True,text=True)
    if (result.returncode==0)!=expected:raise RuntimeError('candidate role mismatch: '+result.stderr[:1000])
rolecheck(role,True)
for name in ('PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128','PIONEER_Q38_QGW8_NATIVE_INSPECTION',
 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION','PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
 'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION',
 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
 *(['PIONEER_Q38_QGW8_HIDDEN_NATIVE_129'] if hidden else []), *(['PIONEER_Q38_QGW8_TRACE_NATIVE_131'] if trace else []),
 *(['PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132'] if stage else []),
 *(['PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133'] if input_trace else []),
 *(['PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134'] if value_trace else [])):
    missing=[v for v in role if v!='-D'+name+'=1']
    rolecheck(missing,False);rolecheck([*missing,'-D'+name+'=0'],False)
for name in ('PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128',
 'PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_NATIVE_127',
 'PIONEER_Q38_QGW8_HIDDEN_CLIENT_129','PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
 'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    rolecheck([*role,'-D'+name+'=1'],False)
rolecheck([v for v in role if not v.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010127ULL'],False)
reload_header=driver/'PioneerDriverReloadProtocol.h'
reload_flags=['-DTT_PIONEER_DRIVER_RELOAD=1', '-DPIONEER_DRIVER_BUILD_TAG='+tag,
 '-DPIONEER_Q38_HYBRID_FULL_NATIVE=1','-DPIONEER_Q38_QGW8_CANDIDATE_NATIVE_128=1']
if hidden:reload_flags+=['-DPIONEER_Q38_QGW8_HIDDEN_NATIVE_129=1']
if trace:reload_flags+=['-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1']
if stage:reload_flags+=['-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1']
if input_trace:reload_flags+=['-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=1']
if value_trace:reload_flags+=['-DPIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134=1']
def reloadcheck(values,expected):
    result=subprocess.run([cxx,'-std=gnu++17',*values,*includes,'-E','-x','c++',str(reload_header)],capture_output=True,text=True)
    if (result.returncode==0)!=expected:raise RuntimeError('candidate reload identity mismatch: '+result.stderr[:1000])
reloadcheck(reload_flags,True)
for index in ((2,3,4) if hidden or trace else (2,3)):
    reloadcheck([flag for i,flag in enumerate(reload_flags) if i!=index],False)
    altered=list(reload_flags);altered[index]=altered[index].split('=')[0]+'=0';reloadcheck(altered,False)
if stage:
    reloadcheck([f for f in reload_flags if f!='-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1'],False)
    reloadcheck([f for f in reload_flags if f!='-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=1']+['-DPIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132=0'],False)
if input_trace:
    reloadcheck([f for f in reload_flags if f!='-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=1'],False)
    reloadcheck([f for f in reload_flags if f!='-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=1']+['-DPIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133=0'],False)
if value_trace:
    reloadcheck([f for f in reload_flags if f!='-DPIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134=1'],False)
    reloadcheck([f for f in reload_flags if f!='-DPIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134=1']+['-DPIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134=0'],False)
def run(*args):subprocess.run([str(v) for v in args],check=True,cwd=out)
dep=subprocess.check_output([*common,*flags,*includes,'-MM',str(source)],text=True)
dependencies={Path(p).resolve() for p in dep.replace('\\\n','').split(':',1)[1].split() if Path(p).resolve().is_relative_to(root)}
dependencies.update((source.resolve(),(driver/'PioneerPciProfile.cpp').resolve(),Path(__file__).resolve(),header.resolve()))
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
run(*common,*includes,'-c',driver/'PioneerPciProfile.cpp','-o',out/'profile.o')
run(*common,*flags,*includes,'-c',source,'-o',out/'native.o')
def compiler_path(name):return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
binary=out/('tenstorrent_pci_probe-qgw8-trace-131' if trace else 'tenstorrent_pci_probe-qgw8-hidden-129' if hidden else 'tenstorrent_pci_probe-qgw8-128')
if stage:binary=out/'tenstorrent_pci_probe-qgw8-dense-stage-132'
if input_trace:binary=out/'tenstorrent_pci_probe-qgw8-dense-input-133'
if value_trace:binary=out/'tenstorrent_pci_probe-qgw8-dense-value-134'
run(cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
 release/'system/glue/haiku_version_glue.o',out/'native.o',out/'profile.o',release/'system/kernel/kernel.so',
 compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',binary)
symbols=subprocess.check_output([str(tool)+'-nm','-C','--defined-only',str(binary)],text=True)
undefined=subprocess.check_output([str(tool)+'-nm','-u',str(binary)],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38Qgw8UploadControlInspection(','Q38SpanControl(','Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(',
             *(['Q38Qgw8HiddenControl('] if hidden else []), *(['Q38Qgw8TraceControlInspection('] if trace else [])):
    if name not in symbols:raise RuntimeError('candidate missing required symbol '+name)
for name in ('llama_','ggml_','Q38WorkControl','Q38HybridUploadControl(','ReferenceProducts','OriginalSource','SyntheticIO'):
    if name in symbols or name in undefined:raise RuntimeError('unexpected CPU/reference symbol '+name)
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z0-9]+)\[\]=',header.read_text())
if len(arrays)!=15:raise RuntimeError('wrong number of pinned QGW8 sections')
raw=subprocess.check_output([str(tool)+'-nm','-S','--defined-only',str(out/'native.o')],text=True)
named={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
rows=subprocess.check_output([str(tool)+'-objdump','-t',str(out/'native.o')],text=True)
sections={row[-1]:row[-3] for row in map(str.split,rows.splitlines()) if len(row)>=6 and row[-1] in named};extracted={}
for digest,label in arrays:
    names=[name for name in named if 'PioneerQ38HybridPrograms' in name and name.endswith(str(len(label))+label+'E')]
    if len(names)!=1:raise RuntimeError('missing section '+label)
    name=names[0];section=sections[name];first,size=named[name]
    if section not in extracted:
        target=out/('section-'+str(len(extracted)));run(str(tool)+'-objcopy','-O','binary','-j',section,out/'native.o',target);extracted[section]=target.read_bytes()
    if hashlib.sha256(extracted[section][first:first+size]).hexdigest()!=digest:raise RuntimeError('changed section '+label)
frames=[line.split('\t') for line in (out/'native.su').read_text().splitlines()]
if not frames or any(len(row)!=3 or row[2]!='static' or int(row[1])>4096 for row in frames):raise RuntimeError('driver stack limit')
selected=[int(row[1]) for row in frames if 'TokenNative' in row[0] or 'Qgw8Upload' in row[0]]
if not selected or max(selected)>2048:raise RuntimeError('QGW8/token stack limit')
if any(hashlib.sha256(p.read_bytes()).hexdigest()!=digest for p,digest in frozen.items()):raise RuntimeError('source changed during candidate build')
print('QGW8_DENSE_VALUE_134_SHA256' if value_trace else 'QGW8_DENSE_INPUT_133_SHA256' if input_trace else 'QGW8_DENSE_STAGE_132_SHA256' if stage else 'QGW8_TRACE_131_SHA256' if trace else 'QGW8_HIDDEN_129_SHA256' if hidden else 'QGW8_CANDIDATE_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('EXPERIMENTAL BUILD ONLY: NOT INSTALLED, TESTED ON DEVICE, OR QUALIFIED')
