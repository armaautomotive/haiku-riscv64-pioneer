#!/usr/bin/env python3
"""Build fixed diagnostic112 client for one fresh generation,NO device access."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
build,private,out=map(Path,sys.argv[1:4]);generation=int(sys.argv[4]);assert 2<=generation<2**63
assert all(p.is_absolute() for p in (build,private,out)) and build.is_dir() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
pins={'PioneerNativeCompactQ4StreamData.h':'4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
 'PioneerNativeCompactQ4StreamMetadata.h':'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'}
assert private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700 and not private.is_symlink()
for name,digest in pins.items():
 info=(private/name).lstat();assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
 assert hashlib.sha256((private/name).read_bytes()).hexdigest()==digest
out.mkdir(mode=0o700);here=Path(__file__).resolve().parent;root=here.parents[1]
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';cc=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for p in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
 if p.is_dir():includes+=['-isystem',str(p)]
includes+=['-I'+str(private),'-I'+str(here),'-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot')]
flags=['-DPIONEER_NATIVE_FACTORED_Q4_TRACE=1','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1',
 '-DPIONEER_NATIVE_FACTORED_Q4_TRACE_EXPECTED_GENERATION='+str(generation)]
binary=out/'pioneer-native-factored-q4-trace'
command=[cc,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-ffile-prefix-map='+str(root)+'=/pioneer-source',*flags,*includes,
 '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),
 *['-L'+str(p) for p in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))],
 str(here/'pioneer_native_factored_q4_trace_diagnostic.cpp'),str(here/'PioneerModelWeightCache.cpp'),
 str(release/'kits/shared/libshared.a'),'-lbe','-o',str(binary)]
subprocess.run(command,check=True)
cases=[]
for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
 for value in (0,1):cases.append(['-D'+role+'='+str(value)])
for role in ('PIONEER_NATIVE_FACTORED_Q4_TRACE','PIONEER_NATIVE_FACTORED_Q4_TRACE_EXPECTED_GENERATION'):
 for value in (None,0,1 if role.endswith('GENERATION') else 2):
  cases.append(['-U'+role]+([] if value is None else ['-D'+role+'='+str(value)]))
cases.append(['-ffast-math'])
for extra in cases:
 r=subprocess.run([cc,'-std=c++17',*flags,*extra,*includes,'-fsyntax-only',str(here/'pioneer_native_factored_q4_trace_diagnostic.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 assert r.returncode and b'error:' in r.stderr,extra
(out/'role-refusals.txt').write_text(''.join('PASS '+' '.join(extra)+'\n' for extra in cases))
(out/'build-argv.txt').write_text('\t'.join(command)+'\n')
for name,digest in pins.items():assert hashlib.sha256((private/name).read_bytes()).hexdigest()==digest
print('FACTORED_TRACE_CLIENT',generation,binary.stat().st_size,hashlib.sha256(binary.read_bytes()).hexdigest(),flush=True)
print('BUILD_ONLY_DIAGNOSTIC_CPU_REFERENCES_roleRefusals'+str(len(cases))+'_NOT_WORK_OR_NATIVE_HARDWARE_AUTHORITY')
