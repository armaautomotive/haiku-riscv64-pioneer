#!/usr/bin/env python3
"""Compile the ACTUAL native adapter against the Haiku kernel SDK; no link/run.

Compile preserved ordinary branches before the new distinct NODRV body.
Refuse inherited PIONEER environment selectors. No card/network/system writes.
"""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

if len(sys.argv) not in (3,4) or any(k.startswith('PIONEER_') and v != '0' for k,v in os.environ.items()):
    raise SystemExit(2)
build, output = map(pathlib.Path, sys.argv[1:3])
assert build.is_absolute() and output.is_absolute() and not output.exists()
assert not output.is_symlink()
project = pathlib.Path(__file__).resolve().parents[3]
reference = pathlib.Path(sys.argv[3]) if len(sys.argv)==4 else project
assert reference.is_absolute() and (reference/'headers/private/kernel').is_dir()
misc = project/'src/add-ons/kernel/drivers/misc'
sdk = build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
binpath = build/'cross-tools-riscv64/bin'
cxx = binpath/'riscv64-unknown-haiku-g++'
args = [str(cxx),'-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
        '-fstack-usage','-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize',
        '-fno-use-cxa-atexit','-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic','-D_KERNEL_MODE']
for inc in ('headers/private/kernel','headers/private/kernel/arch/riscv64','headers/private/kernel/arch/generic',
            'headers/private','headers/private/drivers','headers/private/system','headers/private/system/arch/riscv64',
            'build/config_headers','headers/private/shared'):
    args += ['-isystem',str(reference/inc)]
args += ['-I'+str(build/'objects/haiku/riscv64/release/system/kernel'),
         '-I'+str(build/'objects/haiku/riscv64/debug/system/kernel'),'-I'+str(sdk/'headers'),
         '-idirafter',str(sdk/'headers/posix'),'-iquote',str(misc)]
for inc in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if inc.is_dir(): args += ['-isystem',str(inc)]
features = ['-DTT_PIONEER_EXPERIMENTAL_Q38_'+x for x in (
    'BANKS','PANEL','AFFINE','BLOCK','STREAM','ROWS','COHORT','COHORT_SEALED','COHORT_PAYLOAD','DISPATCH','DISPATCH_VECTOR')]
features += ['-DTT_PIONEER_DRIVER_RELOAD']
output.mkdir(mode=0o700)
source = (misc/'tenstorrent_pci_probe.cpp').read_bytes()
old = subprocess.check_output(['git','show','623bdaad810e868aa6413ed366f11c459beb86e1:src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'],cwd=reference)
assert hashlib.sha256(old).hexdigest() == 'ad85ad21c14d36ea07e2d2a80949fe40770ee58df4b6cb61b0f99f538709e57d'

def compile_input(name, data, flags):
    target = output/(name+'.o')
    command = args+flags+['-x','c++','-c','-','-o',str(target)]
    subprocess.run(command,input=data,check=True)
    return target

for name, flags in [('default',[]),('base55',features+['-DPIONEER_DRIVER_BUILD_TAG=0x2026102100010012ULL'])]:
    before=compile_input(name+'-prior',old,flags)
    after=compile_input(name+'-current',source,flags)
    assert before.read_bytes()==after.read_bytes(), name+' preserved object must remain byte-exact'
    print(name,'prior/current complete objects equal',hashlib.sha256(after.read_bytes()).hexdigest(),flush=True)

obj=output/'native-DRSNODRV.o'
subprocess.run(args+features+['-DPIONEER_DRIVER_BUILD_TAG=0x4452534e4f445256ULL','-c',
    str(project/'tools/pioneer/tests/pioneer_dram_read_sweep_native_compile.cpp'),'-o',str(obj)],check=True)
nm=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-S','--defined-only',str(obj)],text=True)
demangled=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-nm'),'-C','-S','--defined-only',str(obj)],text=True)
normal=('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
defined={row[-1] for row in map(str.split,nm.splitlines()) if len(row)==4}
assert not set(normal)&defined
assert all('pioneer_dram_context_'+name in defined for name in normal)
for name in ('private_kernel_initialize','private_open','private_run','private_control'):
    assert 'pioneer_dram_context_'+name in defined
for name in ('PioneerDramSweepNativeContext::Scope','PioneerDramSweepNativeContext::ClosePci',
             'PioneerDramSweepNativeIO::SweepNativeScope','PioneerDramSweepClockIO::Check',
             'PioneerDramReadSweepOwnerType<BlackholeDramReadSweepImage',
             '::Execute<PioneerDramSweepNativeIO>'):
    assert name in demangled,(name,'actual native body must be instantiated')
for name,minimum in [('PioneerDramSweepNativeContext::Scope',300),
                     ('PioneerDramSweepNativeContext::ViewsPresent',500),
                     ('PioneerDramSweepNativeContext::ClosePci',384),
                     ('PioneerDramSweepNativeIO::SweepNativeScope',200),
                     ('PioneerDramSweepClockIO::Check',64),
                     ('PioneerDramReadSweepOwnerType<BlackholeDramReadSweepImage',2048)]:
    rows=[line.split() for line in demangled.splitlines() if name in line]
    assert rows and max(int(row[1],16) for row in rows)>=minimum,(name,'not a constant stub')
assembly=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-dr',str(obj)],text=True)
(output/'native-symbols.txt').write_text(demangled)
(output/'native-assembly.txt').write_text(assembly)
# Audit actual renamed ABI machine code against independent inert controls.
# Symbols live in section-specific address spaces; entries MUST be .text.
symbols={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,nm.splitlines()) if len(row)==4}
table=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-t',str(obj)],text=True)
sections={row[-1]:row[-3] for row in map(str.split,table.splitlines()) if len(row)>=6 and row[-1] in symbols}
text=subprocess.check_output([str(binpath/'riscv64-unknown-haiku-objdump'),'-dr','-j','.text',str(obj)],text=True)
instructions={int(m[1],16):m[2] for line in text.splitlines()
              for m in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+',line)] if m}
relocations=[(int(m[1],16),m[2]) for line in text.splitlines()
             for m in [re.match(r'^\s*([0-9a-f]+):\s+R_\S+\s+(\S+)',line)] if m]
def code(name):
    start,size=symbols['pioneer_dram_context_'+name]
    assert sections['pioneer_dram_context_'+name]=='.text'
    assert not any(start<=address<start+size for address,_ in relocations)
    words=[word for address,word in sorted(instructions.items()) if start<=address<start+size]
    assert sum(len(word)//2 for word in words)==size
    return words
for name in ('init_hardware','init_driver'):
    assert code(name)==code('expected_refusal')
for name in ('publish_devices','find_device'):
    assert code(name)==code('expected_null')
assert code('uninit_driver')==code('expected_inert')
start,size=symbols['pioneer_dram_context_private_control']
assert {name for address,name in relocations if start<=address<start+size}<= {'geteuid','*ABS*'}
assert 'get_thread_info' in assembly and 'get_sem_count' in assembly and 'get_area_info' in assembly
frames=[]
for line in obj.with_suffix('.su').read_text().splitlines():
    row=line.split('\t');assert len(row)==3 and row[2]=='static',line
    if any(word in row[0] for word in ('DramSweep','DramReadSweep','pioneer_dram_context')):
        frames.append((int(row[1]),row[0]))
assert frames and max(size for size,_ in frames)<=2048,sorted(frames,reverse=True)[:5]
print('native actual adapter object',hashlib.sha256(obj.read_bytes()).hexdigest(),
      'native frame max',max(size for size,_ in frames),'bytes; NO LINK/INSTALL/HARDWARE',flush=True)
role=['-DPIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT=1','-DPIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY=1',
      '-DTT_PIONEER_DRIVER_RELOAD','-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH',
      '-DPIONEER_DRIVER_BUILD_TAG=0x4452534e4f445256ULL']
role_input=('#include "'+str(misc/'PioneerDramReadSweepNativeRole.h')+'"\n').encode()
cases=[(role,True)]
cases += [(role[:i]+role[i+1:],False) for i in range(len(role))]
cases += [(role+['-D'+name+'=1'],False) for name in ('PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
 'PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT','PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT',
 'PIONEER_Q38_RESIDENT_OUTPUT_OWNER','PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT',
 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE')]
for flags,expected in cases:
    result=subprocess.run([str(cxx),'-x','c++','-E','-']+flags,input=role_input,capture_output=True)
    assert (result.returncode==0)==expected,(flags,result.stderr)
print('role perimeter',len(cases),'actual preprocessor cases; five renamed entry bodies inert',flush=True)
