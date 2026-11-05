#!/usr/bin/env python3
"""BUILD ONLY: distinct replay13 USER and rate client; never install/launch."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Whole USER checks required')
assert len(sys.argv)==4
build,libraries,out=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (build,libraries,out)) and build.is_dir() and libraries.is_dir()
assert not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
out.mkdir(mode=0o700)
sys.argv=[str(here/'build_assistant_qwen27_full_offload.py'),str(build),str(libraries),str(out/'control99'),
          '--batched','--metrics','--prepared-q4','--counter-unroll','--phase-profile']
parent=runpy.run_path(sys.argv[0])
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(out/'control99/HaikuAssistantQwen27FullOffload')=='f47b04f4f0e21211520d0d8a2285353c0a40807fc0f016daa7f3e2234870ef8d'
assert sha(out/'control99/pioneer-q38-full-offload-rate-smoke')=='f810d61b543f07751ad1a3667f250afe3f4dc72454c8b6e9acbb1e7f60c0a5e0'
source=here/'pioneer_assistant_qwen27_two_product_replay_q4_full_offload.cpp'
obj=out/'worker.o';binary=out/'HaikuAssistantQwen27FullOffload'
compile=parent['compile'][:]
compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY=1']
compile[compile.index(str(parent['source']))]=str(source)
compile[compile.index(str(parent['obj']))]=str(obj)
link=parent['link'][:]
link[link.index(str(parent['obj']))]=str(obj)
link[link.index(str(parent['binary']))]=str(binary)
commands=[]
def run(command):
    commands.append(command)
    subprocess.run(command,check=True,cwd=out)
run(compile);run(link)
refusals=parent['refusals']+[
    ('missing-replay',['-UPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY']),
    ('zero-replay',['-UPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY','-DPIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY=0']),
    ('old-profile-package',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H=1']),
    ('native-tag-in-USER',['-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010100ULL'])]
for label,extra in refusals:
    result=subprocess.run([*compile[:compile.index('-c')],*extra,'-fsyntax-only',str(source)],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=out)
    assert result.returncode and any(t in result.stderr for t in (
        'Full-offload client is separate','Full-card','Prepared full-card source requires')),label
(out/'role-refusals.txt').write_text(''.join('PASS '+label+'\n' for label,_ in refusals))
tool=parent['tool']
undefined=subprocess.check_output([tool+'nm','-u',str(binary)],text=True)
llama_imports={r.split()[-1] for r in undefined.splitlines() if r.split() and r.split()[-1].startswith('llama_')}
assert llama_imports==parent['expected'],llama_imports
for name in ('llama_decode','llama_init_from_model','llama_get_logits','ggml_','PioneerQ38Ggml'):
    assert name not in undefined,name
rows=[r.split('\t') for r in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=32768 for r in rows)
old='-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-profile-q4"'
new='-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-two-product-replay-q4-20261105-a/HaikuAssistantQwen27FullOffload-two-product-replay-q4"'
for label in ('smoke','rate'):
    command=parent[label+'_command'][:]
    assert command.count(old)==1
    command[command.index(old)]=new
    target=out/parent[label].name
    command[command.index(str(parent[label]))]=str(target)
    run(command)
    imports=subprocess.check_output([tool+'nm','-u',str(target)],text=True)
    assert 'llama_' not in imports and 'ggml_' not in imports
    print(sha(target),target.name,flush=True)
(out/'llama-imports.txt').write_text('\n'.join(sorted(llama_imports))+'\n')
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print(sha(binary),binary.name,flush=True)
print('PASS BUILD_ONLY replay13 USER/tag100;exact11 vocabulary/container imports;tag99 USER/rate reproduced;'+
      str(len(refusals))+' role refusals;no CPU context/decode/logits/ggml inference;unchanged original cache writer',flush=True)
print('NOT installed or hardware-qualified;NO speed result',flush=True)
