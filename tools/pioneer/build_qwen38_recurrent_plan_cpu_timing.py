#!/usr/bin/env python3
"""Build and inspect CPU-only immutable-plan timing. Never run/install here."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv) not in (3,4) or (len(sys.argv)==4 and sys.argv[3]!='--prefix-mirror') or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
mirror=len(sys.argv)==4
build,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink();out.mkdir(mode=0o700)
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
 if path.is_dir():includes+=['-isystem',str(path)]
libraries=['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
obj=out/'cpu-timing.o';binary=out/('q38-prefix-exact-mirror-CPU-TIMING-ONLY' if mirror else 'q38-immutable-plan-CPU-TIMING-ONLY')
subprocess.run([tool+'g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
 '-fstack-usage','-ffile-prefix-map='+str(here.parents[1])+'/=/pioneer-source/',*includes,'-c',
 str(here/('tests/pioneer_q38_recurrent_prefix_mirror_cpu_timing.cpp' if mirror else 'tests/pioneer_q38_recurrent_plan_cpu_timing.cpp')),'-o',str(obj)],check=True)
subprocess.run([tool+'g++',*includes,*libraries,'-B'+str(release/'system/glue')+'/',
 '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)],check=True)
symbols=subprocess.check_output([tool+'nm','-C',str(binary)],text=True)
assert not re.search(r'\b(?:ioctl|open|fopen|mmap|map_physical_memory)\b',symbols)
for forbidden in ('PioneerSessionIO','PioneerStoreContext','PioneerQ38RecurrentBlockOwner','PioneerQ38RecurrentPrefixOwner','PioneerQ38RecurrentPrefixNativeState','Q38WorkControl'):
 assert forbidden not in symbols,forbidden
frames=[int(s.split('\t')[1]) for s in (out/'cpu-timing.su').read_text().splitlines() if len(s.split('\t'))==3]
assert frames and max(frames)<=32768
for path in (obj,binary):print(hashlib.sha256(path.read_bytes()).hexdigest()+' '+path.name)
print('PASS Haiku CPU-only immutable-plan timing compile/link/no device-open-or-ioctl/native-owner symbols/maxUSERframe'+str(max(frames))+';NOT executed/model/TPS proof')
if mirror:print('EXACT PREFIX MIRROR user-mode DATA helper ONLY;NO kernel/native-role/driver integration,model IO or physical-card timing')
