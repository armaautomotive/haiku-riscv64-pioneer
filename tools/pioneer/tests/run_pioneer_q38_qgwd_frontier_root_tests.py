#!/usr/bin/env python3
"""QGWD frontier ROOT build/leaf tests; never access a device."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==5 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,old,new_a,new_b=map(Path,sys.argv[1:])
assert all(p.is_absolute() and p.is_dir() for p in (sdk,old,new_a,new_b))
here=Path(__file__).resolve().parent;root=here.parent
pins={'boot.bin':'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca',
      'text.bin':'09e3ea3aa599f10cef7f452cfce34d164dc655a65856be21e15d364a0ce82b0b',
      'terminal.bin':'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5',
      'conv.bin':'5395ab6f90c53c93e95d5d0b5e280a2d0bf74f76eb4cdd69ccb69daea9f218ba',
      'bf16.bin':'09d75790f0dd7506eef07ff3bca5900d50fe6717168362ad1a2ddeed4e3cfca0',
      'program.elf':'a569498cca9175e4dbd13cf2f1f3dccb6596cf115b2d9b765a016cf9677a51f2'}
assert hashlib.sha256((old/'program.elf').read_bytes()).hexdigest()=='2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95'
for image in (new_a,new_b):
    for name,digest in pins.items():assert hashlib.sha256((image/name).read_bytes()).hexdigest()==digest,(image,name)
    assert len((image/'text.bin').read_bytes())==131064 # 8 bytes remain in the reserved 128 KiB extent
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
symbols=subprocess.check_output([tool+'nm','-nC',str(new_a/'program.elf')],text=True)
entries=re.findall(r'^([0-9a-f]+) [tTW] GlobalChain::PublishFrontier\(unsigned int\)$',symbols,re.M)
assert len(entries)==1
token_entries=re.findall(r'^([0-9a-f]+) [tTW] GlobalChain::PublishToken\(unsigned int const\*, unsigned int, unsigned int, bool\)$',symbols,re.M)
assert len(token_entries)==1
disassembly=subprocess.check_output([tool+'objdump','-dC',str(new_a/'program.elf')],text=True)
blocks=re.split(r'(?=^[0-9a-f]+ <[^\n]+>:\n)',disassembly,flags=re.M)
token_blocks=[b for b in blocks if b.startswith(token_entries[0]+' <GlobalChain::PublishToken(')]
assert len(token_blocks)==1 and token_blocks[0].count('<GlobalChain::PublishFrontier(unsigned int)>')==1
assert 'PublishFrontier(' not in subprocess.check_output([tool+'nm','-nC',str(old/'program.elf')],text=True)
role=root/'PioneerQ38TokenFrontierImageRole.h'
required=('PIONEER_Q38_QGWD_FRONTIER_IMAGE','__riscv','PIONEER_Q38_QGWD_ROOT_IMAGE',
          'PIONEER_Q38_TOKEN_FRONTIER_IMAGE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
          'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK','PIONEER_Q38_TOKEN_CANCEL_BOUNDARY',
          'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
          'PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP')
base=['-D'+name+'=1' for name in required];compiler=os.environ.get('CXX','c++')
def rolecheck(flags,okay,tail=''):
    source='#include "'+str(role)+'"\n'+tail
    result=subprocess.run([compiler,'-std=c++17',*flags,'-x','c++','-fsyntax-only','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,result.stderr[:1500]
rolecheck(base,True)
for name in required:
    selected='-D'+name+'=1'
    rolecheck([f for f in base if f!=selected],False)
    rolecheck([f for f in base if f!=selected]+['-D'+name+'=0'],False)
    rolecheck([f for f in base if f!=selected]+['-D'+name+'=2'],False)
for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD',
             'PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
             'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
             'PIONEER_Q38_HYBRID_FULL_CLIENT','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
             'PIONEER_Q38_BF16_FULL_NATIVE','PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_GLOBAL_WORKER_IMAGE',
             'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
             'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION','__FAST_MATH__'):
    for value in (0,1):rolecheck(base+['-D'+name+'='+str(value)],False)
rolecheck(base,False,'#undef PIONEER_Q38_QGWD_FRONTIER_IMAGE\n#include "'+str(role)+'"\n')
rolecheck([],False,'#define PIONEER_Q38_QGWD_FRONTIER_IMAGE 1\n#include "'+str(role)+'"\n')
with tempfile.TemporaryDirectory(prefix='qgwd-frontier-',dir='/private/tmp') as directory:
    tmp=Path(directory);test=tmp/'leaf'
    flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    *flags,str(here/'pioneer_q38_token_frontier_instruction_test.cpp'),'-o',str(test)],check=True)
    subprocess.run([str(test),str(new_a),entries[0]],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    for options in (['--qgwd-frontier-root'],['--qgwd-frontier-root']*2,
                    ['--qgwd-dense-q8-root','--qgwd-frontier-root','--qgwd-frontier-root']):
        out=tmp/'must-not-exist'
        result=subprocess.run([sys.executable,str(root/'build_qwen38_token_root.py'),str(tmp/'no-sdk'),str(out),*options],capture_output=True)
        assert result.returncode and not out.exists()
print('PASS QGWD frontier pinned ROOT build/roles and emitted leaf; NO DEVICE, NATIVE OR TPS')
