#!/usr/bin/env python3
"""BUILD ONLY: distinct fixed-config USER and rate client; never install/launch."""
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
sys.argv=[str(here/'build_assistant_qwen27_queued_q4_full_offload.py'),str(build),str(libraries),str(out/'control102')]
parent=runpy.run_path(sys.argv[0])
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(out/'control102/HaikuAssistantQwen27FullOffload')=='549b34032aaaa29c41a7696052129dbf66743c52680f5e06fc1ae6b663ba54fe'
assert sha(out/'control102/pioneer-q38-full-offload-rate-smoke')=='c3b0224228592314c0f5e2948bf3bdcd1a23ae6b8f58945b9397ec46ee7d8e26'
source=here/'pioneer_assistant_qwen27_fixed_config_q4_full_offload.cpp'
obj=out/'worker.o';binary=out/'HaikuAssistantQwen27FullOffload'
compile=parent['compile'][:]
compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4=1']
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
    ('missing-fixed',['-UPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4']),
    ('zero-fixed',['-UPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4','-DPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4=0']),
    ('two-fixed',['-UPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4','-DPIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4=2']),
    ('old-queued-package',['-DPIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4_PROGRAMS_H=1']),
    ('offboard-fixed-image',['-DPIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE=1']),
    ('offboard-delta-image',['-DPIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE=1']),
    ('offboard-pair-image',['-DPIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE=1'])]
for label,extra in refusals:
    result=subprocess.run([*compile[:compile.index('-c')],*extra,'-fsyntax-only',str(source)],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=out)
    assert result.returncode and any(t in result.stderr for t in (
        'Full-offload client is separate','Full-card','Prepared full-card source requires')),label
(out/'role-refusals.txt').write_text(''.join('PASS '+label+'\n' for label,_ in refusals))
tool=parent['tool']
undefined=subprocess.check_output([tool+'nm','-u',str(binary)],text=True)
llama_imports={r.split()[-1] for r in undefined.splitlines() if r.split() and r.split()[-1].startswith('llama_')}
assert llama_imports==parent['llama_imports'],llama_imports
for name in ('llama_decode','llama_init_from_model','llama_get_logits','ggml_','PioneerQ38Ggml'):
    assert name not in undefined,name
rows=[r.split('\t') for r in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=32768 for r in rows)
old='-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-queued-q4-20261105-a/HaikuAssistantQwen27FullOffload-queued-q4"'
new='-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-fixed-config-q4-20261105-a/HaikuAssistantQwen27FullOffload-fixed-config-q4"'
for label in ('smoke','rate'):
    command=parent['commands'][2 if label=='smoke' else 3][:]
    assert command.count(old)==1
    command[command.index(old)]=new
    target=out/('pioneer-q38-full-offload-smoke' if label=='smoke' else 'pioneer-q38-full-offload-rate-smoke')
    command[command.index(str(parent['out']/target.name))]=str(target)
    run(command)
    imports=subprocess.check_output([tool+'nm','-u',str(target)],text=True)
    assert 'llama_' not in imports and 'ggml_' not in imports
    print(sha(target),target.name,flush=True)
(out/'llama-imports.txt').write_text('\n'.join(sorted(llama_imports))+'\n')
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
print(sha(binary),binary.name,flush=True)
print('PASS BUILD_ONLY fixed-config USER/tag103;exact11 vocabulary/container imports;tag102/tag101/tag100/tag99 USER/rate reproduced;'+
      str(len(refusals))+' role refusals;no CPU context/decode/logits/ggml inference;unchanged original cache writer',flush=True)
print('NOT installed or hardware-qualified;NO speed result',flush=True)
