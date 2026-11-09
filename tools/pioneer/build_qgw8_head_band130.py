#!/usr/bin/env python3
"""Private build of isolated head-band driver130; never install or open GPU."""
import hashlib
import os
from pathlib import Path
import re
import stat
import struct
import subprocess
import sys

if not __debug__ or len(sys.argv)!=5 or any(not Path(x).is_absolute() for x in sys.argv[1:]):
    raise SystemExit('usage: ABS_HAIKU_BUILD ABS_PRIVATE_EXPORT ABS_PINNED_C64_IMAGE NEW_ABS_OUTPUT')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('Pioneer environment override refused')
build,private,image,out=map(Path,sys.argv[1:])
if not build.is_dir() or private.is_symlink() or not private.is_dir() or private.stat().st_uid!=os.geteuid() or stat.S_IMODE(private.stat().st_mode)!=0o700:
    raise SystemExit('build/private directory refused')
if out.exists() or out.is_symlink() or not out.parent.is_dir(): raise SystemExit('output must be new')
pins={'PioneerNativeBfpModelBandData.h':'d3ff8eeb5830fb33f908a00d1d9dc17860724d41aedc2f0e0bf0045dd8deaae3',
      'PioneerNativeBfpModelBandMetadata.h':'664591584043b713148464b16054ba25d3456bcd603ad34686dc7208535d0fd9'}
def verify():
    for name,pin in pins.items():
        path=private/name; info=path.lstat()
        if not stat.S_ISREG(info.st_mode) or info.st_uid!=os.geteuid() or info.st_nlink!=1 or stat.S_IMODE(info.st_mode)!=0o600:
            raise RuntimeError('unsafe private input: '+name)
        if hashlib.sha256(path.read_bytes()).hexdigest()!=pin: raise RuntimeError('private hash mismatch: '+name)
    for name,pin in (('boot.bin','d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'),
                     ('text.bin','7b3ca669be8d9e0714f544bd0d74c045329b0490d8f2132187181a1a92be55cd')):
        if hashlib.sha256((image/name).read_bytes()).hexdigest()!=pin: raise RuntimeError('C64 image mismatch: '+name)
verify()
here=Path(__file__).resolve().parent; root=here.parent.parent
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release'
tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-'); cxx=tool+'g++'
if not Path(cxx).is_file() or not sdk.is_dir() or not (release/'system/kernel/kernel.so').is_file(): raise SystemExit('SDK refused')
out.mkdir(mode=0o700)
text=(image/'text.bin').read_bytes(); boot=(image/'boot.bin').read_bytes()
if len(boot)!=4 or len(text)%4: raise RuntimeError('bad image shape')
words=struct.unpack('<'+'I'*(len(text)//4),text)
lines=['// Generated code only, no learned data.', 'struct PioneerNativeDramModelBandGeneratedImage {',
       'static constexpr unsigned Width=5120,Phases=4,LayoutRole=130,Boot=0x%08x;'%struct.unpack('<I',boot)[0],
       'static unsigned Word(unsigned i){static const unsigned words[]={']
for first in range(0,len(words),8): lines.append(','.join('0x%08x'%w for w in words[first:first+8])+',')
lines+=['};return i<sizeof(words)/4?words[i]:0;}','};']
(out/'PioneerNativeDramModelBandGeneratedImage.h').write_text('\n'.join(lines)+'\n')
includes=[]
for p in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/p)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(sdk/'headers'),
           '-idirafter',str(sdk/'headers/posix'),'-I'+str(out),'-I'+str(here),'-I'+str(private)]
for p in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if p.is_dir():includes+=['-isystem',str(p)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
        '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
        '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic','-D_KERNEL_MODE']
roles=['BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR']
native=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+x for x in roles]+['-DTT_PIONEER_DRIVER_RELOAD',
        '-DPIONEER_DRIVER_BUILD_TAG=0x2026110800010130ULL','-DPIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC=1',
        '-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1','-DPIONEER_QGW8_HEAD_BAND_130=1']
driver=root/'src/add-ons/kernel/drivers/misc'
def run(args): subprocess.run(args,cwd=out,check=True)
run(common+includes+['-c',str(driver/'PioneerPciProfile.cpp'),'-o',str(out/'profile.o')])
run(common+native+includes+['-c',str(driver/'pioneer_native_dram_model_band_driver.cpp'),'-o',str(out/'native.o')])
run(common+includes+['-c',str(driver/'tenstorrent_pci_probe.cpp'),'-o',str(out/'control.o')])
for label,extra in (
    ('missing-head-selector',['-UPIONEER_QGW8_HEAD_BAND_130']),
    ('zero-head-selector',['-UPIONEER_QGW8_HEAD_BAND_130','-DPIONEER_QGW8_HEAD_BAND_130=0']),
    ('old-driver-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010084ULL']),
    ('missing-reload',['-UTT_PIONEER_DRIVER_RELOAD']),
    ('missing-parent',['-UPIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC']),
    ('work-role',['-DTT_PIONEER_Q38_WORK_MODE=1']),
    ('cpu-reference',['-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1']),
    ('load-only',['-DPIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC=1'])):
    # Preprocessed output contains PRIVATE model weights: never retain it.
    result=subprocess.run(common+native+extra+includes+['-E',str(driver/'pioneer_native_dram_model_band_driver.cpp')],
                          cwd=out,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    if result.returncode==0 or not any(x in result.stderr for x in
       ('Build only the exact diagnostic','QGW8 head-band role requires','Diagnostic81 cannot alias')):
        raise RuntimeError('driver role refusal not established: '+label)
def compiler_file(name): return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
for obj,name in (('native.o','tenstorrent_pci_probe-head130'),('control.o','default43-control')):
    run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_file('crtbeginS.o'),
         str(release/'system/glue/haiku_version_glue.o'),str(out/obj),str(out/'profile.o'),
         str(release/'system/kernel/kernel.so'),compiler_file('libgcc.a'),compiler_file('crtendS.o'),'-o',str(out/name)])
if hashlib.sha256((out/'default43-control').read_bytes()).hexdigest()!='2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc':
    raise RuntimeError('default control differs from pinned baseline; DO NOT USE DRIVER')
exports=subprocess.check_output([tool+'nm','-D',str(out/'tenstorrent_pci_probe-head130')],text=True)
names={line.split()[-1] for line in exports.splitlines() if line.split()}
if not {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names:
    raise RuntimeError('required driver exports missing')
legacy='PioneerNativeDramBandLegacyFindDevice'
disassembly=subprocess.check_output([tool+'objdump','-d','--disassemble=_Z'+str(len(legacy))+legacy+'PKc',
                                    str(out/'tenstorrent_pci_probe-head130')],text=True)
instructions=[]
for line in disassembly.splitlines():
    match=re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$',line)
    if match: instructions.append((match[1],match[2].strip()))
if instructions != [('add','sp,sp,-16'),('sd','s0,8(sp)'),('add','s0,sp,16'),('ld','s0,8(sp)'),
                    ('li','a0,0'),('add','sp,sp,16'),('ret','')]:
    raise RuntimeError('legacy endpoint is not the pinned null stub')
user_includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for p in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if p.is_dir(): user_includes+=['-isystem',str(p)]
user_includes+=['-I'+str(private),'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot')]
client_flags=['-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
              '-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1','-DPIONEER_QGW8_HEAD_BAND_130=1']
client=str(here/'pioneer_qgw8_head_band130_diagnostic.cpp')
for label,extra in (('missing-head',['-UPIONEER_QGW8_HEAD_BAND_130']),('zero-head',['-UPIONEER_QGW8_HEAD_BAND_130','-DPIONEER_QGW8_HEAD_BAND_130=0']),
                    ('missing-c64',['-UPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64']),('work',['-DTT_PIONEER_Q38_WORK_MODE=1']),
                    ('reference',['-DPIONEER_Q38_CPU_VALIDATION=1']),('kernel',['-D_KERNEL_MODE=1']),('fast-math',['-ffast-math'])):
    result=subprocess.run([cxx,*client_flags,*extra,*user_includes,'-E',client],cwd=out,
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    if result.returncode==0 or 'Exact isolated QGW8 head-band diagnostic required' not in result.stderr:
        raise RuntimeError('client role refusal not established: '+label)
libs=['-L'+str(p) for p in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
run([cxx,*client_flags,*user_includes,'-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),
     '-L'+str(release/'kits'),*libs,client,str(here/'PioneerModelWeightCache.cpp'),
     str(release/'kits/shared/libshared.a'),'-lbe','-o',str(out/'pioneer-qgw8-head130')])
verify()
print('HEAD130_DRIVER_BUILD_SHA256',hashlib.sha256((out/'tenstorrent_pci_probe-head130').read_bytes()).hexdigest())
print('HEAD130_CLIENT_BUILD_SHA256',hashlib.sha256((out/'pioneer-qgw8-head130').read_bytes()).hexdigest())
print('PASS isolated driver/client build, driver/client role refusals and unchanged default control; PRIVATE BINARIES, NO INSTALL OR DEVICE')
