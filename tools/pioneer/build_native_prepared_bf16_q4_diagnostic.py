#!/usr/bin/env python3
"""Build distinct prepared115. Private weights/binaries are NEVER publishable."""
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys
assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
build,private,original,output=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (build,private,original,output)) and build.is_dir() and private.is_dir() and original.is_file()
assert not output.exists() and not output.is_symlink() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parent.parent
pins={'PioneerNativeCompactQ4StreamData.h':'4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
 'PioneerNativeCompactQ4StreamMetadata.h':'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'}
def private_unchanged():
    assert not private.is_symlink() and private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700
    for name,digest in pins.items():
        p=private/name;info=p.lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
        assert hashlib.sha256(p.read_bytes()).hexdigest()==digest
private_unchanged()
output.mkdir(mode=0o700);commands=[]
def run(command):
    commands.append(list(map(str,command)));subprocess.run(list(map(str,command)),check=True,cwd=output)
host=os.environ.get('CXX','c++')
run([host,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-I'+str(private),
     here/'pioneer_prepared_bf16_q4_export.cpp',here/'PioneerModelWeightCache.cpp','-o',output/'exporter'])
run([output/'exporter',original,output])
prepared=output/'PioneerNativePreparedBf16Q4Data.h'
prepared_sha=hashlib.sha256(prepared.read_bytes()).hexdigest()
def prepared_unchanged():
    info=prepared.lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
    assert hashlib.sha256(prepared.read_bytes()).hexdigest()==prepared_sha
prepared_unchanged()
image=output/'image5120'
run([sys.executable,here/'build_q38_prepared_bf16_q4_compare.py',build,image])
code=(image/'text.bin').read_bytes();boot=(image/'boot.bin').read_bytes()
assert len(code)==14788 and hashlib.sha256(code).hexdigest()=='7caadb532dc62ec3dfd8b136554b9d7e953e3ce1b0fa8e85eaf7750c2249acd9'
assert len(boot)==4 and hashlib.sha256(boot).hexdigest()=='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
role=115
header=('#include <stdint.h>\nstruct PioneerNativeCompactQ4StreamGeneratedImage {\n'
 ' static constexpr unsigned Samples=9,Rows=32,Phases=4,LayoutRole=115,Bytes=14788,MetadataFirst=128,MetadataRows=128,DstLast=255;\n'
 ' static constexpr unsigned PreparedBf16Comparison=1,NewPhases=2,Accumulator=80;\n'
 ' static constexpr uint32_t Boot=0x%08x;\n'%int.from_bytes(boot,'little')+
 ' static unsigned Width(unsigned s){return s<6?5120:s<9?17408:0;}\n'
 ' static uint32_t Word(unsigned s,unsigned i){\n static const uint32_t code[3697]={\n')
words=[int.from_bytes(code[i:i+4],'little') for i in range(0,len(code),4)]
header+=',\n'.join(','.join('0x%08x'%v for v in words[i:i+8]) for i in range(0,len(words),8))
header+='\n };return s>=9 || i>=3697?0:code[i];}\n};\n'
(output/'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header)
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx = tool + 'g++'
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here), '-I' + str(private)]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
common = [cxx, '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti', '-fstack-usage',
          '-ffreestanding', '-finline', '-fno-semantic-interposition', '-fno-tree-vectorize', '-fno-use-cxa-atexit',
          '-fno-strict-aliasing', '-fno-delete-null-pointer-checks', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE']
roles = ['BANKS', 'PANEL', 'AFFINE', 'BLOCK', 'STREAM', 'ROWS', 'COHORT', 'COHORT_SEALED', 'COHORT_PAYLOAD', 'DISPATCH', 'DISPATCH_VECTOR']
native = ['-DTT_PIONEER_EXPERIMENTAL_Q38_' + name for name in roles]
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010115ULL', '-DPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC=1', '-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1', '-DPIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE=1']
driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_compact_q4_stream_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
cases=[('missing-role',['-UPIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE']),
 ('zero-role',['-UPIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE','-DPIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE=0']),
 ('missing-native',['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC']),
 ('missing-reload',['-UTT_PIONEER_DRIVER_RELOAD']),
 ('missing-timing',['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING']),
 ('wrong-timing',['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=0']),
 ('old-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010114ULL']),
 ('fast-math',['-ffast-math'])]
for macro in ('PIONEER_NATIVE_FACTORED_Q4_COMPARE','PIONEER_NATIVE_FACTORED_Q4_TRACE','PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST',
              'PIONEER_NATIVE_FACTORED_Q4_CONFIGURED','PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING',
              'PIONEER_Q38_PREPARED_BF16_COMPARE_BUILD','PIONEER_Q38_PREPARED_BF16_Q4_BUILD','PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD',
              'PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC','PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC','PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC',
              'PIONEER_NATIVE_BFP_DIAGNOSTIC','PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC','PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC',
              'PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','TT_PIONEER_Q38_WORK_MODE',
              'PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
              'PIONEER_NATIVE_Q4_FPU_STREAM_DIAGNOSTIC','PIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC',
              'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT'):
    for value in (0,1):cases.append((macro+'='+str(value),['-D'+macro+'='+str(value)]))
for label,extra in cases:
    r=subprocess.run(common+native+extra+includes+['-E',str(driver/'pioneer_native_compact_q4_stream_driver.cpp')],
                     stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=output)
    assert r.returncode and any(s in r.stderr for s in ('Exact separate prepared BF16 Q4 comparison115 role required',
        'Exact prepared BF16 Q4 comparison115 kernel identity required','Exact diagnostic107 kernel identity required',
        'Exact separate timed compact Q4 diagnostic107 role required','Build only the exact diagnostic104',
        'Diagnostic104 cannot alias','Exact separate factored-Q4 comparison111 role required',
        'Exact comparison111 kernel identity required','Exact separate factored-Q4 trace112 role required',
        'Exact trace112 kernel identity required','Exact separate manual-Last comparison113 role required',
        'Exact separate configured comparison114 role required','Exact separate paired compact Q4 diagnostic108 role required',
        'Exact diagnostic108 kernel identity required')),label
(output/'driver-role-refusals.txt').write_text('\n'.join('PASS '+label for label,_ in cases)+'\n')
wrong=output/'wrong-image';wrong.mkdir(mode=0o700)
for old,new,message in (
 ('LayoutRole=115','LayoutRole=114','exact compact QCS1 two-width image and Dst scope only'),
 ('MetadataFirst=128','MetadataFirst=0','exact compact QCS1 two-width image and Dst scope only'),
 ('MetadataRows=128','MetadataRows=64','exact compact QCS1 two-width image and Dst scope only'),
 ('DstLast=255','DstLast=127','exact compact QCS1 two-width image and Dst scope only'),
 ('PreparedBf16Comparison=1','PreparedBf16Comparison=0','exact prepared BF16 comparison115 image only'),
 ('NewPhases=2','NewPhases=4','exact prepared BF16 comparison115 image only'),
 ('Accumulator=80','Accumulator=32','exact prepared BF16 comparison115 image only')):
    (wrong/'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header.replace(old,new))
    r=subprocess.run(common+native+['-I'+str(wrong)]+includes+['-fsyntax-only',str(driver/'pioneer_native_compact_q4_stream_driver.cpp')],
                     stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=output)
    assert r.returncode and message in r.stderr,(old,new)
wrong_data=output/'wrong-data';wrong_data.mkdir(mode=0o700)
(wrong_data/'PioneerNativePreparedBf16Q4Data.h').write_text('#include <stdint.h>\nstruct PioneerNativePreparedBf16Q4Data {static constexpr unsigned Version=1,Rows=32,Samples=9,Role=104,PreparedBytes=5632;static unsigned Width(unsigned s){return s<6?5120:17408;}static unsigned Words(unsigned s){return Width(s)/256*2688;}static uint32_t Word(unsigned,unsigned){return 0;}};\n')
r=subprocess.run(common+native+['-I'+str(wrong_data)]+includes+['-fsyntax-only',str(driver/'pioneer_native_compact_q4_stream_driver.cpp')],
                 stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=output)
assert r.returncode and 'exact compiled diagnostic115 prepared data only' in r.stderr
(output/'placement-compile-refusals.txt').write_text('PASS seven image/scope and one prepared-data-role refusals,actual full driver instantiation\n')
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
native_name='tenstorrent_pci_probe-native-q4-fpu-stream'+str(role)
for obj, name in [('native.o', native_name), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
binary = output / native_name
exports = subprocess.check_output([tool + 'nm', '-D', str(binary)], text=True)
(output / 'native-exports.txt').write_text(exports)
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= {line.split()[-1] for line in exports.splitlines() if line.split()}
legacy_name = 'PioneerNativeCompactQ4StreamLegacyFindDevice'
legacy = subprocess.check_output([tool + 'objdump', '-d', '--disassemble=_Z' + str(len(legacy_name)) + legacy_name + 'PKc', str(binary)], text=True)
(output / 'native-legacy-entry.disassembly').write_text(legacy)
instructions = []
for line in legacy.splitlines():
    match = re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$', line)
    if match:
        instructions.append((match[1], match[2].strip()))
assert instructions == [('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'), ('ld', 's0,8(sp)'), ('li', 'a0,0'), ('add', 'sp,sp,16'), ('ret', '')]
assert hashlib.sha256((output / 'tenstorrent_pci_probe-default43-control').read_bytes()).hexdigest() == '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
private_unchanged()
prepared_unchanged()
(output/'prepared-data-identity.txt').write_text(prepared_sha+'  PioneerNativePreparedBf16Q4Data.h\n')
for name in (native_name, 'tenstorrent_pci_probe-default43-control'):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print(f'PASS BUILD_ONLY actual native{role} driver/all methods LINKED,{len(cases)} role refusals,actual wrong-image/wrong-data refusals,legacy NULL/control unchanged;PRIVATE headers unchanged;NO stage/open/hardware/full-model/speed authority')
