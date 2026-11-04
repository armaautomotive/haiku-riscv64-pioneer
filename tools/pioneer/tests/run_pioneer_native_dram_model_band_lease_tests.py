#!/usr/bin/env python3
"""PRIVATE real learned bytes+actual RV32 owner algorithm;fake MMIO ONLY."""
import hashlib
import os
import pathlib
import stat
import subprocess
import sys
import tempfile
if not __debug__ or len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != '--alignment64-math'):
    raise SystemExit('Use non-optimized Python SDK PRIVATE_EXPORT_DIRECTORY')
here = pathlib.Path(__file__).resolve().parent
sdk, private = map(pathlib.Path, sys.argv[1:3])
aligned = len(sys.argv) == 4
options = ['--alignment64-math'] if aligned else []
flags = ['-DPIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64=1'] if aligned else []
assert sdk.is_absolute() and private.is_absolute() and not private.is_symlink()
identity = private.stat()
assert stat.S_ISDIR(identity.st_mode) and identity.st_uid == os.geteuid() and stat.S_IMODE(identity.st_mode) == 0o700
pins = {'PioneerNativeBfpModelBandData.h': 'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
        'PioneerNativeBfpModelBandMetadata.h': 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
for name, digest in pins.items():
    path = private / name
    info = path.lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-dram-learned-lease.'))
subprocess.run([sys.executable, str(here.parent / 'build_native_dram_band.py'), str(sdk), str(folder / 'image'), '5120', *options], check=True)
assert hashlib.sha256((folder / 'image/text.bin').read_bytes()).hexdigest() == ('7b3ca669be8d9e0714f544bd0d74c045329b0490d8f2132187181a1a92be55cd' if aligned else 'cd924896986dabcead8493c9dedca1d7b25fde323fbd972caf67ad93139a9309')
compiler = os.environ.get('CXX', 'c++')
common = [compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
          '-fsanitize=address,undefined', *flags, '-I' + str(private), '-I' + str(here.parent)]
subprocess.run([*common, str(here / 'pioneer_native_dram_model_band_lease_test.cpp'), '-o', str(folder / 'test')], check=True)
subprocess.run([str(folder / 'test'), str(folder / 'image')], check=True,
               env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
for selection in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
    source = '#define B_DEVICE_OP_CODES_END 10000\n#include "PioneerNativeDramModelBandLease.h"\n'
    result = subprocess.run([*common, '-D' + selection, '-x', 'c++', '-c', '-', '-o', str(folder / 'forbidden.o')], input=source, text=True, capture_output=True)
    assert result.returncode and 'GDDR learned-band lease requires distinct diagnostic81' in result.stderr
for name, digest in pins.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
print('PASS GDDR lease3 forbidden production roles;PRIVATE headers unchanged;OFFBOARD evidence:', folder)
