#!/usr/bin/env python3
"""Compile actual diagnostic driver with synthetic zero weights; never link/install."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABSOLUTE_EXISTING_MODEL_BAND_BUILD_DIRECTORY')
prior=Path(sys.argv[1]);here=Path(__file__).resolve().parent
assert prior.is_absolute() and prior.is_dir() and not prior.is_symlink()
lines=(prior/'build-argv.txt').read_text().splitlines()
matches=[line.split('\t') for line in lines if 'pioneer_native_model_band_driver.cpp' in line]
assert len(matches)==1
args=matches[0];assert args[0].endswith('/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
old='-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010080ULL';assert args.count(old)==1 and args.count('-o')==1
assert args.index('-o')==len(args)-2
private=[arg for arg in args if arg.startswith('-I') and arg.endswith('/private-export')];assert len(private)==1
args[args.index(private[0])]='-I'+str(here/'fixtures/qgwd-dense-source')
with tempfile.TemporaryDirectory(prefix='qgwd-dense-driver-',dir='/private/tmp') as tmp:
    args[-1]=str(Path(tmp)/'diagnostic.o')
    new=args.copy();new[new.index(old)]='-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010140ULL'
    new.insert(new.index('-c'),'-DPIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC=1')
    subprocess.run(new,check=True)
    for label,flags in [('wrong-tag',[old]),('work',['-DTT_PIONEER_Q38_WORK_MODE=1']),
                        ('cpu',['-DPIONEER_Q38_CPU_VALIDATION=1']),('reference',['-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1']),
                        ('fast-math',['-D__FAST_MATH__=1'])]:
        candidate=new[:-2]+flags+['-fsyntax-only']
        if label=='wrong-tag':candidate[candidate.index('-DPIONEER_DRIVER_BUILD_TAG=0x2026111000010140ULL')]=old;candidate.remove(old) if candidate.count(old)>1 else None
        result=subprocess.run(candidate,capture_output=True,text=True)
        assert result.returncode,(label,result.stderr)
print('PASS actual dense diagnostic kernel driver compile with synthetic data and five forbidden-role refusals; NO LINK/INSTALL/DEVICE')
