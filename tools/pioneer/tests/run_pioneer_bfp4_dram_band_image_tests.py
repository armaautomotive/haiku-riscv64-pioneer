#!/usr/bin/env python3
"""Build/interpret fixed BFP4 GDDR images through shared L1; no device IO."""
import os
import argparse
import hashlib
from pathlib import Path
import re
import stat
import subprocess
import tempfile

here=Path(__file__).resolve().parent
sdk=Path('/Volumes/HaikuBuildLocal/generated.riscv64')
assert sdk.is_dir() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--private-samples',type=Path,help='Existing private directory of separately authenticated real-model samples')
parser.add_argument('--sha256',nargs=3,help='Three independently captured exporter sample SHA-256 values')
args=parser.parse_args()
assert bool(args.private_samples)==bool(args.sha256)
if args.private_samples:
    assert args.private_samples.is_absolute() and not args.private_samples.is_symlink()
    info=args.private_samples.stat()
    assert stat.S_ISDIR(info.st_mode) and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o700
    assert all(re.fullmatch(r'[0-9a-f]{64}',value) for value in args.sha256)
    for sample in range(3):
        path=args.private_samples/('band'+str(sample)+'.rbp4')
        info=path.lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_nlink==1 and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o600
        assert info.st_size==102544 and hashlib.sha256(path.read_bytes()).hexdigest()==args.sha256[sample]
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-dram-image-',dir='/private/tmp') as tmp:
    root=Path(tmp)
    binary=root/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',
                    str(here/'pioneer_bfp4_dram_band_image_test.cpp'),'-o',str(binary)],check=True)
    for width in (5120,6144,17408):
        output=root/str(width)
        subprocess.run(['python3',str(here.parent/'build_bfp4_dram_band.py'),str(sdk),str(output),str(width)],check=True)
        subprocess.run([str(binary),str(output),str(width)],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    fixture=root/'synthetic-fixture'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-fsanitize=address,undefined',
                    str(here/'pioneer_bfp4_synthetic_export_fixture.cpp'),'-o',str(fixture)],check=True)
    synthetic=root/'synthetic.rbp4'
    subprocess.run([str(fixture),str(synthetic)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    subprocess.run([str(binary),str(root/'5120'),'5120',str(synthetic)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    print('PASS synthetic private-sample transport and output comparison; NOT real model data')
    if args.private_samples:
        for sample in range(3):
            path=args.private_samples/('band'+str(sample)+'.rbp4')
            assert hashlib.sha256(path.read_bytes()).hexdigest()==args.sha256[sample]
            subprocess.run([str(binary),str(root/'5120'),'5120',str(path)],check=True,
                           env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
            assert hashlib.sha256(path.read_bytes()).hexdigest()==args.sha256[sample]
        print('PASS private real-model BFP4 bands: three pinned samples, actual RV32 GDDR/matrix observer, no device or model-quality/speed proof')
