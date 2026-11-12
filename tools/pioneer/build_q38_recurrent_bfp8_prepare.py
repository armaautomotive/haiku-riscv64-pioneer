#!/usr/bin/env python3
"""Build the isolated recurrent BFP8 writer; never read a model or use a card."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

if not __debug__ or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('assertions and unmodified Pioneer environment required')
args=sys.argv[1:]
if len(args) not in (1,3) or (len(args)==3 and args[1]!='--haiku-build'):
    raise SystemExit('use NEW_ABSOLUTE_OUTPUT [--haiku-build ABSOLUTE_BUILD]')
out=Path(args[0]);here=Path(__file__).resolve().parent
if not out.is_absolute() or out.exists() or out.is_symlink():raise SystemExit('output must be a new absolute path')
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_q38_hybrid_bfp8_prepare.py'),*args]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
if base['fingerprint']!='7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0':
    raise RuntimeError('unexpected frozen writer fingerprint')
frozen_dir=here/'frozen_bfp8';watched={}
identity=hashlib.sha256(b'all-bfp8\0')
for name in base['inputs']:
    path=frozen_dir/name if name in ('PioneerModelWeightCache.h','PioneerModelWeightCache.cpp') else here/name
    data=path.read_bytes();watched[path]=hashlib.sha256(data).digest()
    identity.update(name.encode()+b'\0'+data+b'\0')
if identity.hexdigest()!=base['fingerprint']:raise RuntimeError('frozen writer inputs changed before recurrent build')
for path in (Path(__file__).resolve(),here/'PioneerQ38RecurrentBfp8Selection.h',
             here/'pioneer_q38_recurrent_bfp8_prepare.cpp',frozen_dir/'pioneer_q38_recurrent_bfp8_frozen.cpp'):
    watched[path]=hashlib.sha256(path.read_bytes()).digest()
command=list(map(str,base['command']));old=str(here/'frozen_bfp8/pioneer_q38_hybrid_bfp8_frozen.cpp')
replacement=str(here/'frozen_bfp8/pioneer_q38_recurrent_bfp8_frozen.cpp')
oldflag='-DPIONEER_Q38_HYBRID_BFP8_PREPARER=1';newflag='-DPIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER=1'
binary=out/'pioneer-q38-recurrent-bfp8-prepare'
if command.count(old)!=1 or command.count(oldflag)!=1 or command.count(str(base['binary']))!=1:
    raise RuntimeError('unexpected base writer build command')
command[command.index(old)]=replacement;command[command.index(oldflag)]=newflag
command[command.index(str(base['binary']))]=str(binary)
# GCC's DWARF5 line-table directory strings retained the temporary build
# directory even with prefix maps. DWARF4 keeps debug data with mapped paths.
command.insert(1,'-ffile-prefix-map='+str(out)+'=NativeWeightCacheBuild')
command.insert(command.index(replacement),'-gdwarf-4')
subprocess.run(command,check=True,cwd=out)
preprocess=subprocess.run([*command[:command.index('-o')],'-E'],cwd=out,stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,text=True,check=True)
if len(re.findall(r'using\s+Tiles\s*=\s*PioneerQ38RecurrentBfp8Tiles\s*;',preprocess.stdout))!=1:
    raise RuntimeError('frozen writer did not bind exactly one recurrent tile adapter')
if 'OpenExistingDirectory' in preprocess.stdout:raise RuntimeError('unexpected newer cache ABI in frozen writer')
refusals=[['-U'+name] for name in ('PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER','PIONEER_Q38_ALL_BFP8_CACHE')]
refusals += [['-U'+name,'-D'+name+'=0'] for name in ('PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER','PIONEER_Q38_ALL_BFP8_CACHE')]
refusals += [['-D'+name+'=1'] for name in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
 'PIONEER_Q38_HYBRID_BFP8_PREPARER','PIONEER_Q38_DENSE_Q8_BFP8_PREPARER','PIONEER_Q38_HYBRID_BFP8_WINDOW11',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__')]
for flags in refusals:
    result=subprocess.run([*command[:command.index('-o')],*flags,'-fsyntax-only'],cwd=out,
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
    if result.returncode==0:raise RuntimeError('forbidden role accepted: '+str(flags))
if any(hashlib.sha256(path.read_bytes()).digest()!=digest for path,digest in watched.items()):
    raise RuntimeError('recurrent writer source changed during build')
print('RECURRENT_BFP8_WRITER_SHA256',hashlib.sha256(binary.read_bytes()).hexdigest())
print('BUILD ONLY: exact 144-tensor CPU preparer; NO model, cache, device, quality or speed claim')
