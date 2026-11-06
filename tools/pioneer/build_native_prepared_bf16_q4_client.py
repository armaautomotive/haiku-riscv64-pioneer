#!/usr/bin/env python3
"""Build distinct115 CPU-reference diagnostic,NOT a Work engine/device action."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
assert __debug__ and len(sys.argv)==6 and not os.environ.get('PYTHONOPTIMIZE')
build,private,prepared,out=map(Path,sys.argv[1:5]);generation=int(sys.argv[5]);assert 2<=generation<2**63
assert all(p.is_absolute() for p in (build,private,prepared,out)) and build.is_dir() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
pins={'PioneerNativeCompactQ4StreamData.h':'4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
      'PioneerNativeCompactQ4StreamMetadata.h':'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'}
for folder in (private,prepared):
 assert folder.is_dir() and not folder.is_symlink() and folder.stat().st_uid==os.geteuid() and stat.S_IMODE(folder.stat().st_mode)==0o700
for name,digest in pins.items():
 p=private/name;info=p.lstat()
 assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
 assert hashlib.sha256(p.read_bytes()).hexdigest()==digest
p=prepared/'PioneerNativePreparedBf16Q4Data.h';info=p.lstat()
assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
prepared_sha=hashlib.sha256(p.read_bytes()).hexdigest()
assert (prepared/'prepared-data-identity.txt').read_text()==prepared_sha+'  PioneerNativePreparedBf16Q4Data.h\n'
out.mkdir(mode=0o700);here=Path(__file__).resolve().parent;root=here.parents[1]
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';cc=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for d in (sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())):
 if d.is_dir():includes+=['-isystem',str(d)]
includes+=['-I'+str(prepared),'-I'+str(private),'-I'+str(here),'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot')]
flags=['-DPIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE=1','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1',
       '-DPIONEER_NATIVE_PREPARED_BF16_Q4_EXPECTED_GENERATION='+str(generation),'-DPIONEER_NATIVE_PREPARED_BF16_Q4_CLIENT_ROLE=115']
binary=out/'pioneer-native-prepared-bf16-q4-compare'
command=[cc,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-ffile-prefix-map='+str(root)+'=/pioneer-source',*flags,*includes,
 '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),
 *['-L'+str(d) for d in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))],
 str(here/'pioneer_native_prepared_bf16_q4_diagnostic.cpp'),str(here/'PioneerModelWeightCache.cpp'),
 str(release/'kits/shared/libshared.a'),'-lbe','-o',str(binary)]
subprocess.run(command,check=True,cwd=out)
cases=[]
for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_NATIVE_FACTORED_Q4_COMPARE','PIONEER_NATIVE_FACTORED_Q4_TRACE','PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST','PIONEER_NATIVE_FACTORED_Q4_CONFIGURED',
             'PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING'):
 for value in (0,1):cases.append(['-D'+role+'='+str(value)])
for role,invalid in (('PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE',2),('PIONEER_NATIVE_PREPARED_BF16_Q4_EXPECTED_GENERATION',1),
                     ('PIONEER_NATIVE_PREPARED_BF16_Q4_CLIENT_ROLE',114),('PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING',2)):
 for value in (None,0,invalid):cases.append(['-U'+role]+([] if value is None else ['-D'+role+'='+str(value)]))
cases.append(['-ffast-math'])
for extra in cases:
 r=subprocess.run([cc,'-std=c++17',*flags,*extra,*includes,'-fsyntax-only',str(here/'pioneer_native_prepared_bf16_q4_diagnostic.cpp')],
                  stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,cwd=out)
 assert r.returncode and b'error:' in r.stderr,extra
assert hashlib.sha256(p.read_bytes()).hexdigest()==prepared_sha
for name,digest in pins.items():assert hashlib.sha256((private/name).read_bytes()).hexdigest()==digest
(out/'role-refusals.txt').write_text(''.join('PASS '+' '.join(extra)+'\n' for extra in cases))
(out/'build-argv.txt').write_text('\t'.join(command)+'\n')
(out/'prepared-data-identity.txt').write_text(prepared_sha+'  PioneerNativePreparedBf16Q4Data.h\n')
print('PREPARED_BF16_CLIENT',generation,binary.stat().st_size,hashlib.sha256(binary.read_bytes()).hexdigest(),flush=True)
print('BUILD_ONLY_DIAGNOSTIC_CPU_REFERENCES_roleRefusals'+str(len(cases))+'_NOT_WORK_OR_NATIVE_HARDWARE_AUTHORITY')
