#!/usr/bin/env python3
"""Test actual tracked upload control with simulated OS/device; never install."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parents[2];compiler=os.environ.get('CXX','c++')
source=(root/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
def run(*args):subprocess.run([str(v) for v in args],check=True)
with tempfile.TemporaryDirectory(prefix='pioneer-hybrid-control-',dir='/private/tmp') as directory:
 out=Path(directory)
 for marker,name in (('PROBE_LOCK','pioneer_lock'),('Q38_RETAIN_MEMORY','pioneer_retain_memory'),
                     ('Q38_SPAN_CONTROL','pioneer_span_control'),('Q38_DISPATCH_COOKIE_RELEASE','pioneer_cookie_release')):
  begin='// PIONEER_'+marker+'_BEGIN';end='// PIONEER_'+marker+'_END'
  assert source.count(begin)==source.count(end)==1
  (out/(name+'.inc')).write_text(source.split(begin,1)[1].split(end,1)[0])
 image=out/'image'
 subprocess.run(['sh',str(here.parent/'build_qwen38_span_image.sh'),str(sdk),str(image)],
  check=True,env={**os.environ,'PIONEER_Q38_SPAN_FAST':'1'})
 binary=out/'test';run(compiler,'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS','-I'+str(out),
  here/'pioneer_q38_hybrid_upload_control_test.cpp','-o',binary)
 run(binary,image)
 close=out/'close-test';run(compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  '-I'+str(out),here/'pioneer_q38_hybrid_cookie_close_test.cpp','-o',close)
 run(close)
 # Compile the actual private whole-driver context with the experimental
 # route enabled. The context renames every conventional driver entry.
 kernel=root/'src/add-ons/kernel/drivers/misc';release=sdk/'objects/haiku/riscv64/release'
 headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
 includes=[]
 for path in ('kernel','kernel/arch/riscv64','kernel/arch/generic','','drivers','system','system/arch/riscv64','shared'):
  includes+=['-isystem',str(root/'headers/private'/path)]
 includes+=['-isystem',str(root/'build/config_headers'),'-I'+str(release/'system/kernel'),'-I'+str(headers),'-idirafter',str(headers/'posix')]
 for path in [headers/'os',*sorted((headers/'os').iterdir())]:
  if path.is_dir():includes+=['-isystem',str(path)]
 role=(kernel/'PioneerQ38TokenNativeRole.h').read_text()
 parents=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==21
 parents.update(('TT_PIONEER_EXPERIMENTAL_Q38_AFFINE','TT_PIONEER_EXPERIMENTAL_Q38_BLOCK',
  'TT_PIONEER_EXPERIMENTAL_Q38_ROWS','TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED',
  'TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD','PIONEER_Q38_HYBRID_TRACKED_UPLOAD'))
 tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';obj=out/'native-context.o'
 run(str(tool)+'-g++','-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
  '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
  '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic',
  *['-D'+name+'=1' for name in sorted(parents)],'-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL',
  *includes,'-c',here/'pioneer_q38_token_native_context_compile.cpp','-o',obj)
 symbols=subprocess.check_output([str(tool)+'-nm','-C','--defined-only',str(obj)],text=True)
 required=('pioneer_token_context_hybrid_control', 'pioneer_token_context_hybrid_span', 'pioneer_token_context_scoped_bind')
 missing=[name for name in required if name not in symbols];assert not missing,missing
 assert not any(line.split()[-1] in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
  for line in symbols.splitlines() if line.split())
 frames=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
 tracked=[int(row[1]) for row in frames if len(row)==3 and ('Q38HybridUpload' in row[0] or 'BindQ38TokenNative' in row[0])]
 assert tracked and max(tracked)<=2048
 print('PASS actual tracked native context RISC-V object; max tracked static frame',max(tracked),'; renamed ABI and no device')
 print('PASS tracked control/handler uses actual extracted driver body; no driver installed or hardware contacted')
