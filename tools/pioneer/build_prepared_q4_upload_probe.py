#!/usr/bin/env python3
"""Snapshot host upload probe,using the ORIGINAL writer's recipe identity."""
import hashlib,os,pathlib,re,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Refuse optimized Python;source provenance checks are required')
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;repository=here.parent.parent;out=pathlib.Path(sys.argv[1])
assert out.is_absolute() and not out.exists() and not out.is_symlink()
sources={}
def collect(path):
    assert path.is_file() and not path.is_symlink()
    relative=path.relative_to(repository).as_posix()
    if relative in sources:return
    data=path.read_bytes();assert data and len(data)<1024*1024;sources[relative]=data
    for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',data.decode(),re.M):
        candidate=path.parent/name;assert not candidate.is_symlink()
        candidate=candidate.resolve();candidate.relative_to(repository);collect(candidate)
def digest(records):
    result=hashlib.sha256()
    for name,data in sorted(records.items()):result.update(name.encode()+b'\0'+len(data).to_bytes(8,'little')+data)
    return result.hexdigest()
for name in ('pioneer_q38_native_q4_prepare.cpp','PioneerModelWeightCache.cpp'):collect(here/name)
writer=digest(sources)
assert writer=='70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6','original cache recipe changed;do not silently regenerate data'
collect(here/'pioneer_q38_prepared_q4_upload_probe.cpp')
out.mkdir(mode=0o700);snapshot=out/'source';snapshot.mkdir(mode=0o700)
for name,data in sources.items():
    target=snapshot/name;target.parent.mkdir(parents=True,exist_ok=True)
    with target.open('xb') as stream:stream.write(data)
    target.chmod(0o400)
command=[os.environ.get('CXX','c++'),'-std=c++17','-O3','-ffp-contract=off','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations',
 '-ffile-prefix-map='+str(snapshot)+'=/pioneer-prepared-q4-upload-source',
 '-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="'+writer+'"',str(snapshot/'tools/pioneer/pioneer_q38_prepared_q4_upload_probe.cpp'),
 str(snapshot/'tools/pioneer/PioneerModelWeightCache.cpp'),'-o',str(out/'probe')]
subprocess.run(command,check=True)
for name,data in sources.items():assert (snapshot/name).read_bytes()==data and (repository/name).read_bytes()==data
(out/'source-identities.txt').write_text(''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(sources.items())))
(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
print('BUILD_ONLY_PREPARED_Q4_UPLOAD_PROBE sourceFiles'+str(len(sources))+' writerRecipe'+writer+' probeSource'+digest(sources)+
 ' binarySHA256'+hashlib.sha256((out/'probe').read_bytes()).hexdigest()+';NO device/native/inference/TPS grant',flush=True)
