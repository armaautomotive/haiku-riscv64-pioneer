#!/usr/bin/env python3
"""Test the kernel receipt and span adapter off-device; never install a driver."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
def run(*args,**kwargs):subprocess.run([str(v) for v in args],check=True,**kwargs)
with tempfile.TemporaryDirectory(prefix='pioneer-hybrid-receipt-',dir='/private/tmp') as directory:
 out=Path(directory);image=out/'image';receipt=out/'receipt';span=out/'span'
 run('sh',root/'build_qwen38_span_image.sh',sdk,image)
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
 run(compiler,*flags,'-Wno-unused-function','-Wno-unused-variable',here/'pioneer_q38_hybrid_upload_receipt_test.cpp','-o',receipt)
 for mode in ([],['--tamper'],['--extra'],['--abort'],['--late']):run(receipt,*mode)
 run(compiler,*flags,'-ffp-contract=off','-DTT_PIONEER_EXPERIMENTAL_Q38_BANKS',here/'pioneer_q38_hybrid_tracked_span_test.cpp','-o',span)
 run(span,image)
 for options in (['-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD=0','-DPIONEER_Q38_GDN_MUTABLE_STATE=1'],
                 ['-DPIONEER_Q38_HYBRID_TRACKED_UPLOAD=1']):
  result=subprocess.run([compiler,'-std=c++17',*options,'-E','-x','c++',str(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38SpanService.h')],
   capture_output=True,text=True)
  assert result.returncode and 'Hybrid tracked upload requires its exact distinct session' in result.stderr
 tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
 obj=out/'kernel.o'
 run(str(tool)+'-g++','-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
  '-fno-exceptions','-fno-rtti','-ffreestanding','-fstack-usage','-mcmodel=medany','-fpic','-D_KERNEL_MODE',
  '-I'+str(headers),'-idirafter',headers/'posix','-c',here/'pioneer_q38_hybrid_upload_receipt_kernel_compile.cpp','-o',obj)
 frames=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
 assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in frames)
 names=subprocess.check_output([str(tool)+'-nm','--defined-only',str(obj)],text=True)
 assert all('hybrid_receipt_'+name in names for name in ('begin','span','record','finish_span','seal','layout'))
 assert not any(line.split()[-1] in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device') for line in names.splitlines() if line.split())
 print('PASS Haiku RISC-V receipt compile, max static frame',max(int(row[1]) for row in frames),'; no linked driver or hardware')
