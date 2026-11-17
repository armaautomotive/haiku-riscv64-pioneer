#!/usr/bin/env python3
"""Build the isolated Q8T4 Haiku timing driver. No install or device I/O."""
from hashlib import sha256
import os
from pathlib import Path
import re
import subprocess
import sys

assert __debug__ and len(sys.argv)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(Path,sys.argv[1:])
assert build.is_absolute() and build.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=Path(__file__).resolve().parent
root=here.parent.parent
driver=root/'src/add-ons/kernel/drivers/misc'
source=driver/'pioneer_native_q38_two_token_q4_driver.cpp'
tag='0x2026111700010215ULL'

def run(args,**options):
    return subprocess.run(args,cwd=out,check=True,**options)

image=out/'image'
run(['sh',here/'build_qwen38_block_image.sh',build,image],env=dict(os.environ,PIONEER_Q38_STREAM_IMAGE='15'))
boot,text=(image/'boot.bin').read_bytes(),(image/'text.bin').read_bytes()
assert int.from_bytes(boot,'little')==0x0006c06f and len(boot)==4
assert sha256(boot).hexdigest()=='e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
assert len(text)==7784 and sha256(text).hexdigest()=='98fd588698cf162634973a7945ff6d3b948c7469e4c09c2612fb3e535ed34a15'
words=[int.from_bytes(text[i:i+4],'little') for i in range(0,len(text),4)]
header='''#include <stdint.h>
struct PioneerQ38TwoTokenQ4GeneratedImage {
 static constexpr unsigned Bytes=7784, BootWord=0x0006c06f, Marker=0x34543851;
 static uint32_t Word(unsigned i){
  static const uint32_t data[Bytes/4]={
'''
header+=',\n'.join(','.join(f'0x{w:08x}' for w in words[i:i+8]) for i in range(0,len(words),8))
header+='\n  };\n  return i<Bytes/4?data[i]:0;\n }\n};\n'
(out/'PioneerQ38TwoTokenQ4GeneratedImage.h').write_text(header)

sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release'
tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx=tool+'g++'
includes=[]
for sub in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
    includes+=['-isystem',str(root/'headers/private'/sub)]
includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),
           '-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix'),'-I'+str(out)]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():
        includes+=['-isystem',str(path)]
common=[cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
        '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
        '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic','-D_KERNEL_MODE']
roles=['BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR']
native=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+role for role in roles]+[
    '-DTT_PIONEER_DRIVER_RELOAD','-DPIONEER_DRIVER_BUILD_TAG='+tag,'-DPIONEER_NATIVE_Q38_TWO_TOKEN_Q4=1']
run(common+includes+['-c',driver/'PioneerPciProfile.cpp','-o',out/'profile.o'])
run(common+native+includes+['-c',source,'-o',out/'native.o'])
refusals=[['-UPIONEER_NATIVE_Q38_TWO_TOKEN_Q4'],['-UTT_PIONEER_DRIVER_RELOAD'],
          ['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026111700010212ULL'],['-ffast-math']]
refusals += [['-D'+m+'=1'] for m in ('PIONEER_Q38_FULL_OFFLOAD_NATIVE','TT_PIONEER_Q38_WORK_MODE',
    'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_GDN_MUTABLE_STATE')]
for flags in refusals:
    failed=subprocess.run(common+native+flags+includes+['-E',source],cwd=out,
        stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    assert failed.returncode and ('exact Q8T4 tag215' in failed.stderr or 'tag215 cannot alias' in failed.stderr),flags

def toolfile(name):
    return run([cxx,'-print-file-name='+name],capture_output=True,text=True).stdout.strip()

binary=out/'tenstorrent_pci_probe-native-q8t4'
run([cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',toolfile('crtbeginS.o'),
     release/'system/glue/haiku_version_glue.o',out/'native.o',out/'profile.o',
     release/'system/kernel/kernel.so',toolfile('libgcc.a'),toolfile('crtendS.o'),'-o',binary])
exports=run([tool+'nm','-D',binary],capture_output=True,text=True).stdout
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'} <= {
    line.split()[-1] for line in exports.splitlines() if line.split()}
legacy='PioneerQ38PairLegacyFindDevice'
dis=run([tool+'objdump','-d',f'--disassemble=_Z{len(legacy)}{legacy}PKc',binary],capture_output=True,text=True).stdout
instructions=[m.groups() for m in re.finditer(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*([^\n]*)$',dis,re.M)]
assert [(op,args.strip()) for op,args in instructions]==[
    ('add','sp,sp,-16'),('sd','s0,8(sp)'),('add','s0,sp,16'),('ld','s0,8(sp)'),
    ('li','a0,0'),('add','sp,sp,16'),('ret','')]
(out/'legacy-entry.disassembly').write_text(dis)
frames=(out/'native.su').read_text().splitlines()
compare=[int(r.split('\t')[1]) for r in frames if re.search(r'PioneerQ38TwoTokenQ4Native::(?:Compare|Control)\(',r) and '\t' in r]
assert compare and max(compare)<=4096, compare
client=out/'pioneer-q38-two-token-q4-compare'
client_flags=['-DTT_PIONEER_EXPERIMENTAL_Q38_'+name for name in ('BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS')]
gcc_libraries=['-L'+str(p) for p in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
assert gcc_libraries
run([cxx,'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off',*client_flags,
     *includes,here/'pioneer_q38_two_token_q4_compare.cpp','-B'+str(release/'system/glue')+'/',
     '-L'+str(release/'system/libroot'),*gcc_libraries,'-o',client])
imports=run([tool+'nm','-u',client],capture_output=True,text=True).stdout
assert 'llama_' not in imports and 'ggml_' not in imports
print(f'{sha256(binary.read_bytes()).hexdigest()}  {binary}')
print(f'{sha256(client.read_bytes()).hexdigest()}  {client}')
print(f'PASS Q8T4 Haiku link; {len(refusals)} role refusals, disabled legacy entry, compare stack <=4096 bytes; BUILD ONLY')
