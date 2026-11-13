#!/usr/bin/env python3
"""Compile the actual receipt with a fake, non-loadable kernel profile."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root = Path(__file__).resolve().parents[1]
role = ((root / 'PioneerQ38QgwdNativeInspectionRole.h').read_text()
        + (root / 'PioneerQ38HybridKernelCompileOnly.h').read_text().split('#ifdef PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY', 1)[1])
required = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
required.update(('PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY',
                 'PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION',
                 'PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION',
                 'PIONEER_Q38_QGWD_NATIVE_INSPECTION',
                 'PIONEER_Q38_QGW8_LAYOUT_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION',
                 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION',
                 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
                 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
                 'PIONEER_Q38_GDN_MUTABLE_STATE', '_KERNEL_MODE'))
flags = ['-D' + name + '=1' for name in sorted(required)]
flags += ['-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL']
flags += ['-D' + name + '=q38_private_' + name for name in
          ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')]
source = '''#include "PioneerQ38SmallRecurrentUploadReceiptInspection.h"
using R=PioneerQ38SmallRecurrentUploadReceiptInspection;
int size(){return sizeof(R);}
bool revalidate(R& r,const void* c,const void* f,const uint32_t* h,const uint8_t* l){
 return r.Revalidate(c,f,h,l,17,42);
}
bool workspace(R& r,const void* c,const void* f,const uint32_t* h,const uint8_t* l,uint32_t* o){
 return r.BuildWorkspace(c,f,h,l,17,42,o,96*32);
}
'''
cmd = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fno-exceptions', '-fno-rtti', '-ffreestanding',
       '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', '-I' + str(root), '-x', 'c++', '-fsyntax-only', '-']
if len(sys.argv) == 2:
    sdk = Path(sys.argv[1])
    assert sdk.is_absolute() and sdk.is_dir()
    headers = sdk / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
    assert headers.is_dir()
    cmd += ['-I' + str(headers), '-idirafter', str(headers / 'posix')]
    for path in [headers / 'os', *(headers / 'os').iterdir()]:
        if path.is_dir():
            cmd += ['-isystem', str(path)]
else:
    assert len(sys.argv) == 1
result = subprocess.run(cmd + flags, input=source, text=True, capture_output=True)
if result.returncode:
    raise SystemExit(result.stderr)
with tempfile.TemporaryDirectory(prefix='q38-recurrent-kernel-', dir='/private/tmp') as tmp:
    object_cmd = [arg for arg in cmd if arg != '-fsyntax-only']
    result = subprocess.run(object_cmd + flags + ['-O2', '-c', '-o', str(Path(tmp) / 'receipt.o')],
                            input=source, text=True, capture_output=True)
    if result.returncode:
        raise SystemExit(result.stderr)
for bad in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
            'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_CPU_VALIDATION'):
    check = subprocess.run(cmd + flags + ['-D' + bad + '=1'], input=source, text=True, capture_output=True)
    assert check.returncode and 'Recurrent kernel compile requires the fake-tag' in check.stderr, bad
for missing in ('_KERNEL_MODE', 'PIONEER_Q38_QGWD_NATIVE_INSPECTION',
                'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY', 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY'):
    check = subprocess.run(cmd + [f for f in flags if f != '-D' + missing + '=1'],
                           input=source, text=True, capture_output=True)
    assert check.returncode and 'Recurrent kernel compile requires the fake-tag' in check.stderr, missing
tag = '-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL'
check = subprocess.run(cmd + [f for f in flags if f != tag] + ['-DPIONEER_DRIVER_BUILD_TAG=0x2026111100010146ULL'],
                       input=source, text=True, capture_output=True)
assert check.returncode and 'Recurrent kernel compile requires the fake-tag' in check.stderr
print('PASS actual recurrent receipt fake-tag freestanding compile and role refusals; NO DRIVER OR DEVICE')
