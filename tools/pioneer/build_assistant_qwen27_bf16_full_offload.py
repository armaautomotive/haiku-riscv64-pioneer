#!/usr/bin/env python3
"""Build vocabulary-only USER worker paired to BF16 tag105;never install/run."""
import hashlib
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys

args=sys.argv[1:];paired='--pair-issue' in args
if paired:assert args.count('--pair-issue')==1;args.remove('--pair-issue')
profile='--profile' in args
if profile:assert args.count('--profile')==1;args.remove('--profile')
profile=profile or paired
assert __debug__ and len(args)==3 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,libraries,out=map(Path,args);here=Path(__file__).resolve().parent;root=here.parents[1]
assert all(p.is_absolute() for p in (sdk,libraries,out)) and sdk.is_dir() and libraries.is_dir()
assert not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
 sys.argv=[str(here/'build_assistant_qwen27_full_offload.py'),str(sdk),str(libraries),str(out/'unchanged-client')]
 old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
source=here/('pioneer_assistant_qwen27_bf16_pair_issue_full_offload.cpp' if paired else 'pioneer_assistant_qwen27_bf16_profile_full_offload.cpp' if profile else 'pioneer_assistant_qwen27_bf16_full_offload.cpp')
obj=out/'worker.o';binary=out/('HaikuAssistantQwen27Bf16PairIssueFullOffload' if paired else 'HaikuAssistantQwen27Bf16ProfileFullOffload' if profile else 'HaikuAssistantQwen27Bf16FullOffload')
compile=old['compile'][:]
compile[compile.index(str(old['source']))]=str(source);compile[compile.index(str(old['obj']))]=str(obj)
compile.insert(1,'-DPIONEER_Q38_BF16_FULL_CLIENT=1')
dependencyText=subprocess.check_output([*compile[:compile.index('-c')],'-MM',str(source)],text=True)
dependencies={Path(p).resolve() for p in dependencyText.replace('\\\n','').split(':',1)[1].split() if Path(p).resolve().is_relative_to(root)}
dependencies.update((Path(__file__).resolve(),here/'build_assistant_qwen27_full_offload.py'))
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
commands=[]
def run(command):
 commands.append(list(map(str,command)));subprocess.run(list(map(str,command)),check=True)
run(compile)
link=old['link'][:];link[link.index(str(old['obj']))]=str(obj);link[link.index(str(old['binary']))]=str(binary);run(link)
refusals=[]
for label,extra in [('missing-bf16',['-UPIONEER_Q38_BF16_FULL_CLIENT']),
 ('zero-bf16',['-UPIONEER_Q38_BF16_FULL_CLIENT','-DPIONEER_Q38_BF16_FULL_CLIENT=0']),
 ('wrong-bf16',['-UPIONEER_Q38_BF16_FULL_CLIENT','-DPIONEER_Q38_BF16_FULL_CLIENT=2']),
 ('missing-parent',['-UPIONEER_Q38_FULL_OFFLOAD_CLIENT'])]+[
 (name+'-defined0',['-D'+name+'=0']) for name in ('PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_CPU_VALIDATION',
 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE','PIONEER_Q38_FULL_OFFLOAD_BATCHED',
 'PIONEER_Q38_FULL_OFFLOAD_METRICS','PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4','PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4')]:
 if profile and label=='PIONEER_Q38_FULL_OFFLOAD_METRICS-defined0':continue # source selects observers; direct role tests cover this refusal
 result=subprocess.run([*compile[:compile.index('-c')],*extra,'-fsyntax-only',str(source)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 assert result.returncode and 'BF16 full-card client requires' in result.stderr,label
 refusals.append(label)
imports=subprocess.check_output([old['tool']+'nm','-u',str(binary)],text=True)
llama={r.split()[-1] for r in imports.splitlines() if r.split() and r.split()[-1].startswith('llama_')}
assert llama==old['expected'],llama
for name in ('llama_decode','llama_init_from_model','llama_get_logits','ggml_backend','ggml_threadpool','PioneerQ38Ggml'):
 assert name not in imports,name
smoke=out/('pioneer-q38-bf16-pair-issue-rate-smoke' if paired else 'pioneer-q38-bf16-profile-rate-smoke' if profile else 'pioneer-q38-bf16-full-offload-smoke');smokeCommand=old['smoke_command'][:]
smokeCommand[smokeCommand.index(str(old['smoke']))]=str(smoke)
if profile:
 old_source=str(here/'pioneer_q38_full_offload_smoke.cpp')
 assert smokeCommand.count(old_source)==1
 smokeCommand[smokeCommand.index(old_source)]=str(here/'pioneer_q38_full_offload_rate_smoke.cpp')
worker='/boot/home/pioneer-q38-bf16-pair-issue-109-a/HaikuAssistantQwen27Bf16PairIssueFullOffload' if paired else '/boot/home/pioneer-q38-bf16-profile-106-a/HaikuAssistantQwen27Bf16ProfileFullOffload' if profile else '/boot/home/pioneer-q38-bf16-full-105-a/HaikuAssistantQwen27Bf16FullOffload'
smokeCommand.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="'+worker+'"');run(smokeCommand)
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'build-argv.json').write_text(json.dumps(commands,indent=2)+'\n');(out/'worker.imports').write_text(imports)
(out/'role-refusals.txt').write_text('\n'.join('PASS '+label for label in refusals)+'\n')
for path in (obj,binary,smoke):print(path.name,path.stat().st_size,hashlib.sha256(path.read_bytes()).hexdigest(),flush=True)
print('PASS BUILD_ONLY BF16 tag'+('109' if paired else '106' if profile else '105')+' USER worker/exact11 vocabulary imports/no CPU context,decode,logits or fallback/'
 +str(len(refusals))+' role refusals/unchanged tag88 USER pins;NOT installed/native execution/model-quality/TPS proof',flush=True)
