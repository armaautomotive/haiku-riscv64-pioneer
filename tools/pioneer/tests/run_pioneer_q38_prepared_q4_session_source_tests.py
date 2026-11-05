#!/usr/bin/env python3
"""Snapshot production helper+test;reuse the exact original writer's cache."""
import hashlib,os,pathlib,re,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
assert len(sys.argv)==4 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[2];out,model,cache=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (out,model,cache)) and model.is_file() and cache.is_dir() and not out.exists()
sources={}
def collect(path):
    assert path.is_file() and not path.is_symlink();name=path.relative_to(root).as_posix()
    if name in sources:return
    data=path.read_bytes();sources[name]=data
    for included in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',data.decode(),re.M):
        candidate=path.parent/included;assert not candidate.is_symlink()
        candidate=candidate.resolve();candidate.relative_to(root);collect(candidate)
for name in ('pioneer_q38_native_q4_prepare.cpp','PioneerModelWeightCache.cpp'):collect(here.parent/name)
digest=hashlib.sha256()
for name,data in sorted(sources.items()):digest.update(name.encode()+b'\0'+len(data).to_bytes(8,'little')+data)
writer=digest.hexdigest();assert writer=='70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6'
collect(here/'pioneer_q38_prepared_q4_session_source_test.cpp')
snapshot=out/'source';snapshot.mkdir(parents=True,mode=0o700)
for name,data in sources.items():
    target=snapshot/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data);target.chmod(0o400)
command=[os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations',
 '-ffp-contract=off','-fsanitize=address,undefined','-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1','-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1',
 '-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1','-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="'+writer+'"',
 str(snapshot/'tools/pioneer/tests/pioneer_q38_prepared_q4_session_source_test.cpp'),str(snapshot/'tools/pioneer/PioneerModelWeightCache.cpp'),'-o',str(out/'test')]
subprocess.run(command,check=True)
subprocess.run([str(out/'test'),str(model),str(cache)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
for name,data in sources.items():assert (snapshot/name).read_bytes()==data and (root/name).read_bytes()==data
print('PASS production helper/source snapshot unchanged;original cache recipe '+writer,flush=True)
