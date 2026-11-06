#!/usr/bin/env python3
"""Execute genuine reset -> first BF16 preparation, BEFORE matrix, OFFBOARD."""
import hashlib,json,os,pathlib,re,subprocess,sys
assert __debug__ and len(sys.argv)==6 and not os.environ.get('PYTHONOPTIMIZE')
image,control,old,worker,out=map(pathlib.Path,sys.argv[1:])
assert all(p.is_absolute() for p in (image,control,old,worker,out))
assert not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(here.parent))
from generate_qwen38_token_programs import authenticated,pins
from PioneerQ38Bf16ProfileProgramPins import ROOT_PINS
from PioneerQ38Bf16PairIssueProgramPins import WORKER_PINS
# Exact prototype BUILD identities: NOT a new native code package or grant.
candidate=json.loads((here.parent/'benchmarks/qwen38-bf16-deferred-prepare-offboard-20261106/clean-qualification.json').read_text())['candidate_artifacts']
authenticated(image,tuple((n,v['bytes'],v['sha256']) for n,v in candidate.items()))
authenticated(control,ROOT_PINS)
authenticated(worker,WORKER_PINS)
authenticated(old,pins(here/'run_pioneer_q38_mixed_worker_tests.py','PINS'))
out.mkdir(mode=0o700);exits={};commands=[]
cxx=os.environ.get('CXX','c++')
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
flags+=['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_TEST_BF16_CONNECTED_PREPARE=1']
source=here/'pioneer_q38_bf16_connected_prepare_test.cpp'
data=subprocess.check_output([cxx,'-std=c++17',*flags,'-MM',str(source)],text=True)
dependencies={pathlib.Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split()}
dependencies.update((pathlib.Path(__file__).resolve(),here/'run_pioneer_q38_ffn_persistent_image_tests.sh',
 here/'run_pioneer_q38_mixed_worker_tests.py',here.parent/'generate_qwen38_token_programs.py',
 here.parent/'PioneerQ38Bf16ProfileProgramPins.py',here.parent/'PioneerQ38Bf16PairIssueProgramPins.py',
 here.parent/'PioneerQ38Bf16ProgramPins.py',here.parent/'benchmarks/qwen38-bf16-deferred-prepare-offboard-20261106/clean-qualification.json'))
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
def run(name,argv):
 argv=list(map(str,argv));commands.append([name,*argv]);(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 with (out/(name+'.stdout')).open('xb') as a,(out/(name+'.stderr')).open('xb') as b:
  process=subprocess.Popen(argv,stdout=a,stderr=b,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
  (out/'live-child.json').write_text(json.dumps(dict(name=name,pid=process.pid,argv=argv))+'\n')
  result=process.wait()
 exits[name]=result;(out/'actual-exits.json').write_text(json.dumps(exits,indent=2)+'\n')
 assert result==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_COMPILED_CONNECTED_PREPARE',name,'actual_exit0_stderr0',flush=True)
run('compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
 '-fsanitize=address,undefined',*flags,source,'-o',out/'connected-test'])
run('control',[out/'connected-test',control,old,worker,'control'])
run('deferred',[out/'connected-test',image,old,worker,'deferred'])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
print('PASS_COMPILED_CONNECTED_PREPARATION children3_actual0_stderr0_sources'+str(len(frozen))+
 '_NO_BF16_MATRIX_FINAL119_CLOSURE_NATIVE_PHYSICAL_MODEL_QUALITY_OR_TPS',flush=True)
