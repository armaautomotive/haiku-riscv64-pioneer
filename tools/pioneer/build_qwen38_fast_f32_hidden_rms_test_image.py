#!/usr/bin/env python3
"""Build separate approximate and original RMS test images; no device IO."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv)==3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv
try:
    sys.argv=[str(here/'build_qwen38_f32_hidden_rms_test_image.py'),str(sdk),str(out/'original')]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
candidate=out/'candidate';candidate.mkdir(mode=0o700);tool=base['tool'];flags=base['flags']
def run(*args):subprocess.run([str(a) for a in args],check=True)
run(tool+'g++',*flags,'-DPIONEER_Q38_FAST_RMS_DIAGNOSTIC=1','-c',
    here/'blackhole_qwen38_fast_f32_hidden_rms_test.cpp','-o',candidate/'program.o')
run(tool+'as','-I'+str(here),'-march=rv32imf','-mabi=ilp32','-mno-relax',
    here/'blackhole_qwen38_learned_rms_start.S','-o',candidate/'start.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_scalar.ld',
    candidate/'start.o',candidate/'program.o','-Map',candidate/'program.map','-o',candidate/'program.elf')
for section in ('text','boot'):
    run(tool+'objcopy','-O','binary','-j','.'+section,candidate/'program.elf',candidate/('code.bin' if section=='text' else 'boot.bin'))
symbols=subprocess.check_output([tool+'nm','-n',str(candidate/'program.elf')],text=True)
dis=subprocess.check_output([tool+'objdump','-d',str(candidate/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(candidate/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
    m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
    if m:assert int(m[1],16)&127!=0x73
frames=[int(line.split('\t')[1]) for line in (candidate/'program.su').read_text().splitlines()]
assert frames and max(frames)<=1024
for name in ('code.bin','boot.bin','program.elf'):
    data=(candidate/name).read_bytes();print('FAST_RMS_IMAGE',name,len(data),hashlib.sha256(data).hexdigest(),flush=True)
assert 0<(candidate/'code.bin').stat().st_size<=32768 and (candidate/'boot.bin').stat().st_size==4
print('BUILD ONLY isolated approximate RMS numeric image; NO device, model or throughput claim')
