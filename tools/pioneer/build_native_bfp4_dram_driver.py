#!/usr/bin/env python3
"""Build ONLY the private fixed-sample BFP4 GDDR diagnostic driver."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import stat
import struct
import subprocess
import sys

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build',type=Path)
parser.add_argument('private',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--matched-q4-input',action='store_true',help='Build separate matched-input diagnostic 121')
args=parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.private.is_absolute() and not args.private.is_symlink()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
info=args.private.stat()
assert stat.S_ISDIR(info.st_mode) and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o700
legacy_pins={'PioneerNativeBfp4DramModelData.h':'51bd25cb2a624a4f979eb93b1b2eca2dcd72b5076bb354ad6480595ac9b74298',
      'PioneerNativeBfp4DramModelMetadata.h':'9842393b3be3329d272cd9d9d358d3194b4d5c31aeb39f43728241a323a8ffa2'}
matched_pins={'PioneerNativeBfp4DramModelData.h':'44a04a036aa297a162435fa6b32f0debdf7a0444fe0c9ab5ab731acefbb44592',
      'PioneerNativeBfp4DramModelMetadata.h':'b627b009aca25ac65c53d66a36164aabdf2cbd8ab8291372b7dc6f56340bc986'}
pins=matched_pins if args.matched_q4_input else legacy_pins
def unchanged():
    for name,digest in pins.items():
        path=args.private/name
        entry=path.lstat()
        assert stat.S_ISREG(entry.st_mode) and entry.st_uid==os.geteuid() and entry.st_nlink==1 and stat.S_IMODE(entry.st_mode)==0o600
        assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
unchanged()
here=Path(__file__).resolve().parent
root=here.parent.parent
build=args.haiku_build
args.output.mkdir(mode=0o700)
image=args.output/'image'
subprocess.run([sys.executable,str(here/'build_bfp4_dram_band.py'),str(build),str(image),'5120'],check=True)
boot=(image/'boot.bin').read_bytes()
code=(image/'text.bin').read_bytes()
assert hashlib.sha256(boot).hexdigest()=='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
assert hashlib.sha256(code).hexdigest()=='78bbf79aa2fa3353dc4f8958acda3b5613449fbd07b313e56d463d1d02792ade'
symbols={}
for line in (image/'program.symbols').read_text().splitlines():
    value,_,name=line.split();symbols[name]=int(value,16)
for name,value in {'native_dram_band_entry':0x70000,'native_dram_band_done':0x7000c,
                   'native_dram_band_park':0x70038,'native_dram_band_fault':0x70040,'native_dram_band_fault_park':0x70058}.items():
    assert symbols[name]==value
assert code[0x38:0x40]==struct.pack('<II',0x6f,0xffdff06f)
words=struct.unpack('<'+'I'*(len(code)//4),code)
header=['/* Exact-pinned fixed OFFBOARD BFP4 GDDR image. */',
        '#ifndef PIONEER_NATIVE_BFP4_DRAM_MODEL_GENERATED_IMAGE_H','#define PIONEER_NATIVE_BFP4_DRAM_MODEL_GENERATED_IMAGE_H',
        'struct PioneerNativeBfp4DramModelGeneratedImage {',
        ' static constexpr unsigned Width=5120,Phases=2,LayoutRole=120,Boot=0x%08x;'%struct.unpack('<I',boot)[0],
        ' static unsigned Word(unsigned i){','  static const unsigned code[]={']
for i in range(0,len(words),8):header.append('   '+','.join('0x%08x'%word for word in words[i:i+8])+',')
header+=['  };return i<sizeof(code)/4?code[i]:0;',' }','};','#endif']
(args.output/'PioneerNativeBfp4DramModelGeneratedImage.h').write_text('\n'.join(header)+'\n')
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release'
tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx=tool+'g++'
includes=[]
for path in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/path)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(sdk/'headers'),
           '-idirafter',str(sdk/'headers/posix'),'-I'+str(args.output),'-I'+str(here),'-I'+str(args.private)]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
        '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
        '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic','-D_KERNEL_MODE']
roles=['BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR']
native=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name for name in roles]
tag='0x2026110700010121ULL' if args.matched_q4_input else '0x2026110600010120ULL'
matched_role=['-DPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC=1'] if args.matched_q4_input else []
native+=['-DTT_PIONEER_DRIVER_RELOAD','-DPIONEER_DRIVER_BUILD_TAG='+tag,'-DPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC=1',*matched_role]
driver=root/'src/add-ons/kernel/drivers/misc'
commands=[]
def run(command):
    commands.append(command)
    subprocess.run(command,check=True,cwd=args.output)
run(common+includes+['-c',str(driver/'PioneerPciProfile.cpp'),'-o',str(args.output/'profile.o')])
run(common+native+includes+['-c',str(driver/'pioneer_native_bfp4_dram_model_driver.cpp'),'-o',str(args.output/'native.o')])
run(common+includes+['-c',str(driver/'tenstorrent_pci_probe.cpp'),'-o',str(args.output/'control.o')])
def compiler_path(name):return subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip()
driver_name='tenstorrent_pci_probe-native-bfp4-matched121' if args.matched_q4_input else 'tenstorrent_pci_probe-native-bfp4-dram120'
for obj,name in (('native.o',driver_name),('control.o','tenstorrent_pci_probe-default43-control')):
    run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',compiler_path('crtbeginS.o'),
         str(release/'system/glue/haiku_version_glue.o'),str(args.output/obj),str(args.output/'profile.o'),
         str(release/'system/kernel/kernel.so'),compiler_path('libgcc.a'),compiler_path('crtendS.o'),'-o',str(args.output/name)])
binary=args.output/driver_name
exports=subprocess.check_output([tool+'nm','-D',str(binary)],text=True)
names={line.split()[-1] for line in exports.splitlines() if line.split()}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
legacy_name='PioneerNativeDramBandLegacyFindDevice'
legacy=subprocess.check_output([tool+'objdump','-d','--disassemble=_Z'+str(len(legacy_name))+legacy_name+'PKc',str(binary)],text=True)
instructions=[]
for line in legacy.splitlines():
    m=re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$',line)
    if m:instructions.append((m[1],m[2].strip()))
assert instructions==[('add','sp,sp,-16'),('sd','s0,8(sp)'),('add','s0,sp,16'),('ld','s0,8(sp)'),('li','a0,0'),('add','sp,sp,16'),('ret','')]
cases=[['-UPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC'],['-UPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC','-DPIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC=0'],
       ['-UTT_PIONEER_DRIVER_RELOAD'],['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010084ULL']]
for macro in ('PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC','PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64',
              'PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC','PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC','PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC',
              'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    cases.append(['-D'+macro+'=1'])
cases.append(['-ffast-math'])
if args.matched_q4_input:
    cases.extend([['-UPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC'],
                  ['-UPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC','-DPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC=0'],
                  ['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010120ULL']])
else:
    cases.append(['-DPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC=1'])
for extra in cases:
    # Preprocessed output contains private model bytes; never persist it.
    result=subprocess.run([*common,*native,*extra,*includes,'-E',str(driver/'pioneer_native_bfp4_dram_model_driver.cpp')],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=args.output)
    assert result.returncode and ('Build only the exact BFP4 GDDR' in result.stderr or 'Diagnostic120 cannot alias' in result.stderr),(extra,result.stderr)
assert hashlib.sha256((args.output/'tenstorrent_pci_probe-default43-control').read_bytes()).hexdigest()=='2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
user_includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():user_includes+=['-isystem',str(path)]
libs=['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
client=args.output/'pioneer-native-bfp4-dram-model-diagnostic'
source_check=args.output/'pioneer-native-bfp4-dram-source-check'
for source,binary_path in (('pioneer_native_bfp4_dram_model_diagnostic.cpp',client),('pioneer_native_bfp4_dram_source_check.cpp',source_check)):
    run([cxx,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',*matched_role,*user_includes,'-I'+str(args.private),
         '-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
         '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),*libs,
         str(here/source),str(here/'PioneerModelWeightCache.cpp'),
         str(release/'kits/shared/libshared.a'),'-lbe','-o',str(binary_path)])
unchanged()
(args.output/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print(hashlib.sha256(binary.read_bytes()).hexdigest(),binary.name)
print(hashlib.sha256(client.read_bytes()).hexdigest(),client.name)
print(hashlib.sha256(source_check.read_bytes()).hexdigest(),source_check.name)
print('PASS BUILD_ONLY private BFP4 GDDR driver/client',('MATCHED121' if args.matched_q4_input else 'LEGACY120'),len(cases),'role refusals; NO install or device authorization')
