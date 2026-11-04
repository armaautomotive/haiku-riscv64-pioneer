#!/usr/bin/env python3
"""Diagnostic81 owner role/ABI gates ONLY,not a native binding or card test."""
import hashlib
import os
import pathlib
import stat
import subprocess
import sys
import tempfile
if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('Use PRIVATE_EXPORT_DIRECTORY')
here = pathlib.Path(__file__).resolve().parent
private = pathlib.Path(sys.argv[1])
assert private.is_absolute() and not private.is_symlink()
identity = private.stat()
assert stat.S_ISDIR(identity.st_mode) and identity.st_uid == os.geteuid() and stat.S_IMODE(identity.st_mode) == 0o700
data = private / 'PioneerNativeBfpModelBandData.h'
info = data.lstat()
assert stat.S_ISREG(info.st_mode) and info.st_uid == os.geteuid() and info.st_nlink == 1 and stat.S_IMODE(info.st_mode) == 0o600
pin = 'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5'
assert hashlib.sha256(data.read_bytes()).hexdigest() == pin
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-dram-learned-role.'))
source = '#include "' + str(here.parent / 'PioneerNativeDramModelBandLease.h') + '"\n'
correct = ['-D_KERNEL_MODE', '-DPIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC=1', '-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010081ULL']
cases = [('correct', correct, True), ('unselected', ['-D_KERNEL_MODE'], False),
         ('wrong-role', [flag for flag in correct if 'DIAGNOSTIC' not in flag] + ['-DPIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC=0'], False),
         ('wrong-tag', [flag for flag in correct if 'BUILD_TAG' not in flag] + ['-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010080ULL'], False),
         ('no-reload', [flag for flag in correct if 'DRIVER_RELOAD' not in flag], False),
         ('no-kernel', correct[1:], False), ('work', correct + ['-DPIONEER_Q38_WORK_INFERENCE'], False),
         ('validation', correct + ['-DPIONEER_Q38_CPU_VALIDATION'], False),
         ('work-mode', correct + ['-DTT_PIONEER_Q38_WORK_MODE'], False),
         ('full-reference', correct + ['-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE'], False)]
compiler = os.environ.get('CXX', 'c++')
for name, flags, accepted in cases:
    result = subprocess.run([compiler, '-std=c++17', '-DB_DEVICE_OP_CODES_END=10000', '-I' + str(private),
                             *flags, '-x', 'c++', '-c', '-', '-o', str(folder / (name + '.o'))], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == accepted, (name, result.stderr)
    if not accepted:
        assert 'GDDR learned-band lease requires distinct diagnostic81' in result.stderr or 'Exact kernel diagnostic81 reload role required' in result.stderr
abi = source + r'''
#include <cassert>
int main(){
 PioneerNativeDramModelBandRequest r={};r.version=1;r.sample=PioneerNativeDramModelBandNone;
 r.expected_tag=PioneerNativeDramModelBandTag;r.expected_generation=1;
 assert(PioneerNativeDramModelBandRequestValid(r));
 for(unsigned i=0;i<sizeof(r.result);i++){
  auto* bytes=reinterpret_cast<unsigned char*>(&r.result);bytes[i]=1;
  assert(!PioneerNativeDramModelBandRequestValid(r));bytes[i]=0;
 }
 r.expected_tag=0x2026110400010080ULL;assert(!PioneerNativeDramModelBandRequestValid(r));
}
'''
subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-DB_DEVICE_OP_CODES_END=10000', '-I' + str(private),
                '-x', 'c++', '-', '-o', str(folder / 'abi')], input=abi, text=True, check=True)
subprocess.run([str(folder / 'abi')], check=True)
assert hashlib.sha256(data.read_bytes()).hexdigest() == pin
print('PASS GDDR learned-band roles10 cases,protocol352 dirty-result-byte refusals+old-tag refusal,exact392B ABI;PRIVATE data unchanged;NO native IO/physical authority')
