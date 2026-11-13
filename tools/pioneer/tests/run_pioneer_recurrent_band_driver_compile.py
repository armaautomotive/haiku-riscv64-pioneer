#!/usr/bin/env python3
"""Compile the distinct recurrent driver role with synthetic data only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2]!='--recurrent-out') or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABSOLUTE_RECURRENT_BAND_BUILD_DIRECTORY [--recurrent-out]')
build = Path(sys.argv[1]); here = Path(__file__).resolve().parent
out = len(sys.argv)==3
assert build.is_absolute() and build.is_dir() and not build.is_symlink()
lines = (build / 'build-argv.txt').read_text().splitlines()
matches = [line.split('\t') for line in lines if 'pioneer_native_model_band_driver.cpp' in line]
assert len(matches) == 1
args = matches[0]
tag = '-DPIONEER_DRIVER_BUILD_TAG=' + ('0x2026111200010148ULL' if out else '0x2026111200010147ULL')
role = '-D' + ('PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC=1' if out else 'PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC=1')
assert args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
assert args.count(tag) == args.count(role) == args.count('-o') == 1 and args.index('-o') == len(args)-2
private = [arg for arg in args if arg.startswith('-I/private/tmp/pioneer-recurrent-out-band.' if out else '-I/private/tmp/pioneer-recurrent-band.')]
assert len(private) == 1
args[args.index(private[0])] = '-I' + str(here/('fixtures/recurrent-out-source' if out else 'fixtures/qgwd-dense-source'))
with tempfile.TemporaryDirectory(prefix='recurrent-band-driver-',dir='/private/tmp') as tmp:
    args[-1] = str(Path(tmp)/'driver.o')
    subprocess.run(args,check=True)
    for label,flag in (('wrong-tag','-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010140ULL'),
                       ('work','-DTT_PIONEER_Q38_WORK_MODE=1'),('cpu','-DPIONEER_Q38_CPU_VALIDATION=1'),
                       ('reference','-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1'),('fast','-D__FAST_MATH__=1'),
                       ('dense','-DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=1'),
                       ('other-recurrent','-D'+('PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC' if out else 'PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC')+'=1')):
        candidate = args[:-2] + ['-fsyntax-only']
        if label == 'wrong-tag':candidate[candidate.index(tag)] = flag
        else:candidate.append(flag)
        result = subprocess.run(candidate,capture_output=True,text=True)
        assert result.returncode, (label,result.stderr)
print('PASS recurrent diagnostic SDK driver compile and seven role refusals; SYNTHETIC, NO DEVICE')
