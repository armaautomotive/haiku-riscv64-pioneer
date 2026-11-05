#!/usr/bin/env python3
"""Fingerprint+snapshot the complete local preparation source;host BUILD ONLY."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Refuse optimized Python;preparation provenance checks are required')
args=sys.argv[1:];haiku=None
if args and args[0]=='--haiku-sdk':
    assert len(args)==3;haiku=pathlib.Path(args[1]);args=args[2:]
    assert haiku.is_absolute() and haiku.is_dir()
assert len(args)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;repository=here.parent.parent
out=pathlib.Path(args[0])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
sources={}


def collect(path):
    assert path.is_file() and not path.is_symlink()
    relative=path.relative_to(repository).as_posix()
    if relative in sources:
        return
    data=path.read_bytes();assert data and len(data)<1024*1024
    sources[relative]=data
    for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',data.decode(),re.M):
        candidate=path.parent/name
        assert not candidate.is_symlink()
        candidate=candidate.resolve();candidate.relative_to(repository)
        collect(candidate)


for name in ('pioneer_q38_native_q4_prepare.cpp','PioneerModelWeightCache.cpp'):
    collect(here/name)
digest=hashlib.sha256()
for name,data in sorted(sources.items()):
    digest.update(name.encode()+b'\0'+len(data).to_bytes(8,'little')+data)
fingerprint=digest.hexdigest()
out.mkdir(mode=0o700);snapshot=out/'source';snapshot.mkdir(mode=0o700)
for name,data in sources.items():
    target=snapshot/name;target.parent.mkdir(parents=True,exist_ok=True)
    with target.open('xb') as stream:stream.write(data)
    target.chmod(0o400)
compiler=os.environ.get('CXX','c++') if haiku is None else str(haiku/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
extra=[]
if haiku:
    sdk=haiku/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release=haiku/'objects/haiku/riscv64/release'
    extra=['-Wno-misleading-indentation','-I'+str(repository/'headers/private/libroot'),'-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
    for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
        if path.is_dir():extra+=['-isystem',str(path)]
    extra+=['-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot')]
    extra+=['-L'+str(path) for path in sorted((haiku/'build_packages').glob('gcc_syslibs-*/lib'))]
command=[compiler,'-std=c++17','-O3','-ffp-contract=off',
         '-Wall','-Wextra','-Werror','-Wno-deprecated-declarations',
         '-ffile-prefix-map='+str(snapshot)+'=/pioneer-native-q4-preparation-source',
         '-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="'+fingerprint+'"',*extra,
         str(snapshot/'tools/pioneer/pioneer_q38_native_q4_prepare.cpp'),
         str(snapshot/'tools/pioneer/PioneerModelWeightCache.cpp'),'-o',str(out/'prepare')]
subprocess.run(command,check=True)
if haiku:
    assert fingerprint=='70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6'
    undefined=subprocess.check_output([compiler[:-3]+'nm','-u',str(out/'prepare')],text=True)
    for forbidden in ('llama_','ggml_','ioctl','mmap','pthread_create'):
        assert forbidden not in undefined,forbidden
for name,data in sources.items():
    assert (snapshot/name).read_bytes()==data and (repository/name).read_bytes()==data
(out/'source-identities.txt').write_text(''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(sources.items())))
(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
binary=(out/'prepare').read_bytes()
(out/'binary-identities.txt').write_text(hashlib.sha256(binary).hexdigest()+'  prepare\n')
print('BUILD_ONLY_NATIVE_Q4_PREPARATION sourceFiles'+str(len(sources))+' recipeFingerprint'+fingerprint+
      ' binarySHA256'+hashlib.sha256(binary).hexdigest()+';NO test/device/native/inference/speed grant',flush=True)
