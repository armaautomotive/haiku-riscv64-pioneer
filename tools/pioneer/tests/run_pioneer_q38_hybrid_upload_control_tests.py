#!/usr/bin/env python3
"""Test actual tracked upload control with simulated OS/device; never install."""
import os
from pathlib import Path
import hashlib
import re
import subprocess
import sys
import tempfile

args=sys.argv[1:];header=None;qgw8='--qgw8-header' in args
selector='--qgw8-header' if qgw8 else '--qgw5-header'
if selector in args:
 index=args.index(selector);assert args.count(selector)==1 and index+1<len(args)
 header=Path(args[index+1]);del args[index:index+2];assert header.is_absolute() and header.is_file()
if qgw8:
 assert hashlib.sha256(header.read_bytes()).hexdigest()=='72ec049c6905e37531b4fc21854991a8ddf513fc0944e8acde2ed73fc59a8588'
assert __debug__ and len(args)==1
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(args[0]);assert sdk.is_absolute() and sdk.is_dir()
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
 if header:
  parents.update(('PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY','PIONEER_Q38_TOKEN_BF16_PROGRAMS',
   'PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC','PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC'))
 if qgw8:
  parents.update(('PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
   'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION','PIONEER_Q38_QGW8_CONTROL_INSPECTION',
   'PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION',
   'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
   'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_SPAN_FAST'))
  protocol=kernel/'PioneerQ38HybridFullProtocol.h'
  source_probe='#include "'+str(protocol)+'"\nstatic_assert(PioneerQ38FullOffload::BuildTag==0x51544b4e4f445256ULL);\n'
  flags=['-D'+name+'=1' for name in sorted(parents)] + ['-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL',
   '-DB_DEVICE_OP_CODES_END=1000'] + ['-D'+name+'=qgw8_fake_'+name for name in
    ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')]
  def probe(values):
   return subprocess.run([compiler,'-std=c++17',*values,'-fsyntax-only','-x','c++','-'],
    input=source_probe,capture_output=True,text=True)
  assert probe(flags).returncode==0
  for name in ('PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION','PIONEER_Q38_QGW8_NATIVE_INSPECTION',
               'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION'):
   assert probe([flag for flag in flags if flag!='-D'+name+'=1']).returncode,name
  for name in ('PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_HYBRID_FULL_NATIVE',
               'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION'):
   assert probe(flags+['-D'+name+'=1']).returncode,name
  wrong=[flag for flag in flags if not flag.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010127ULL']
  assert probe(wrong).returncode,'real candidate tag accepted by fake native role'
 tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku';obj=out/'native-context.o'
 run(str(tool)+'-g++','-std=gnu++17','-O2','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fstack-usage',
  *(['-Wno-misleading-indentation'] if qgw8 else []),
  '-ffreestanding','-finline','-fno-semantic-interposition','-fno-tree-vectorize','-fno-use-cxa-atexit',
  '-fno-strict-aliasing','-fno-delete-null-pointer-checks','-mcmodel=medany','-fpic',
  *['-D'+name+'=1' for name in sorted(parents)],'-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL',
  *(['-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"','-I'+str(here.parent)] if header else []),
  *includes,'-c',here/'pioneer_q38_token_native_context_compile.cpp','-o',obj)
 symbols=subprocess.check_output([str(tool)+'-nm','-C','--defined-only',str(obj)],text=True)
 required=('pioneer_token_context_hybrid_control', 'pioneer_token_context_hybrid_span', 'pioneer_token_context_scoped_bind')
 if qgw8:required+=('pioneer_token_context_qgw8_control','pioneer_token_context_qgw8_full_support','pioneer_token_context_qgw8_full_control')
 missing=[name for name in required if name not in symbols];assert not missing,missing
 assert not any(line.split()[-1] in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
  for line in symbols.splitlines() if line.split())
 if header:
  arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z0-9]+)\[\]=',header.read_text())
  assert len(arrays)==15
  raw=subprocess.check_output([str(tool)+'-nm','-S','--defined-only',str(obj)],text=True)
  named={row[-1]:(int(row[0],16),int(row[1],16)) for row in map(str.split,raw.splitlines()) if len(row)==4}
  rows=subprocess.check_output([str(tool)+'-objdump','-t',str(obj)],text=True)
  sections={row[-1]:row[-3] for row in map(str.split,rows.splitlines()) if len(row)>=6 and row[-1] in named}
  extracted={}
  for digest,label in arrays:
   names=[name for name in named if 'PioneerQ38HybridPrograms' in name and name.endswith(str(len(label))+label+'E')]
   assert len(names)==1,(label,names)
   name=names[0];section=sections[name];first,size=named[name]
   if section not in extracted:
    target=out/('section-'+str(len(extracted)))
    run(str(tool)+'-objcopy','-O','binary','-j',section,obj,target);extracted[section]=target.read_bytes()
   assert hashlib.sha256(extracted[section][first:first+size]).hexdigest()==digest,label
  print('PASS all 15 emitted '+('QGW8' if qgw8 else 'QGW5')+' code arrays match the pinned linked images')
 frames=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
 tracked=[int(row[1]) for row in frames if len(row)==3 and
  ('Q38HybridUpload' in row[0] or 'Q38Qgw8Upload' in row[0] or 'BindQ38TokenNative' in row[0]
   or (qgw8 and ('Q38FullOffloadSupport' in row[0] or 'Q38TokenNativeControl' in row[0])))]
 assert tracked and max(tracked)<=2048
 if qgw8:assert any(len(row)==3 and 'Q38Qgw8Upload' in row[0] and row[2]=='static' for row in frames)
 print('PASS actual tracked '+('QGW8 ' if qgw8 else 'QGW5 ' if header else '')+'native context RISC-V object; max tracked static frame',max(tracked),'; renamed ABI and no device')
 print('PASS tracked control/handler uses actual extracted driver body; no driver installed or hardware contacted')
