#!/usr/bin/env python3
"""Link ROOT and run it against retained worker instruction simulators."""
import ast
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert __debug__ and (len(sys.argv) in (9,14) or (len(sys.argv) in (10,15) and sys.argv[9]=='--reuse')) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root,recurrent,worker,leaf,private,alpha,beta=map(Path,sys.argv[1:9]);here=Path(__file__).resolve().parent
reuse=len(sys.argv) in (10,15);followup=tuple(map(Path,sys.argv[10:] if reuse else sys.argv[9:])) if len(sys.argv) in (14,15) else ()
assert all(path.is_absolute() for path in (sdk,root,recurrent,worker,leaf,private,alpha,beta))
assert all(path.is_dir() and not path.is_symlink() for path in (sdk,root,worker,leaf,private))
pins=((worker/'program.elf','67a9bf22e0f9713b699bb058db3cc5702f4940fb786845e66a3e89a3bfe2175d'),
 (leaf/'program.elf','e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
 (private/'PioneerNativeBfpModelBandData.h','396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a'),
 (private/'PioneerNativeBfpModelBandMetadata.h','a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d'),
 (alpha,'92f7287e9626c4dae6ff4c1118d6d2c64f194cf1c239ac3f7b1b60e477ed4fbb'),
 (beta,'f6615b16825dbe365aad7d01729b2b6cbd19c2f0d51545890d1fe74933bcb753'))
for path,digest in pins:
 assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for path,_ in pins[2:]:
 info=path.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
rootPins=((recurrent,'bed9ed4c8598a50b8448d08fa8e5499580d1b350f2a9c7dc39e307058f04d22a' if reuse else '7a93aebd77d49a8e2eb51a75ffb75f2c25bca67d5036b3fdb4dcfd2ae332049c'),
 (root/'start.o','20ec8f9bb5690b079ad0dbb99dc18a9c89b15d940eef09a6dcf271c535383b46'),
 (root/'program.o','c9bc9bfd38ccc7000bbb2f7482dd25148dd3013b6db8d435b5da7e0718242037'),
 (root/'qualified-components/dense.o','b1960b90ba1b01b2e43b5889aeaab8a69f5db2f48cd103a340f6480941dfb9f2'),
 (root/'qualified-components/unchanged-feedback/root_conv_sections.o','e770a9bde85baadaf9239b7e40985af6415b99650866fad745fe4691a3196395'),
 (root/'qualified-components/unchanged-feedback/feedback.o','8c2626d34501e53dc25368cb8f67c48c9de778fec5d880e21901034795c083f2'))
for path,digest in rootPins:
 assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
tree=ast.parse((here/'run_pioneer_q38_recurrent_small_dispatch_test.py').read_text())
roles=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
           and any(isinstance(t,ast.Name) and t.id=='roles' for t in n.targets))
rootRoles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION',
 'PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_SIMULATION',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_QGWE_BATCH_DENSE_PLAN','PIONEER_Q38_QGWF_DUAL_BATCH_PLAN',
 'PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION')
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');old=root/'qualified-components'
with tempfile.TemporaryDirectory(prefix='q38-small-root-worker-',dir='/private/tmp') as directory:
 tmp=Path(directory);raw=tmp/'raw.o';combined=tmp/'combined.o';image=tmp/'program.elf';rootBinary=tmp/'root-test';workerBinary=tmp/'worker-test'
 assert subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')])==(worker/'program.symbols').read_bytes()
 for folder,sections in ((worker,('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn')),
                         (leaf,('boot','text'))):
  for section in sections:
   extracted=tmp/'section.bin';subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(extracted)],check=True)
   assert extracted.read_bytes()==(folder/(section+'.bin')).read_bytes()
 subprocess.run([tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
  '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer','-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
  str(recurrent),str(old/'dense.o'),str(old/'unchanged-feedback/root_conv_sections.o'),'-o',str(raw)],check=True)
 subprocess.run([tool+'objcopy',*['--strip-symbol='+n for n in ('mixed_recurrent_block_wait','mixed_recurrent_block_stopped',
  'mixed_live_block_wait','mixed_live_block_stopped')],str(raw),str(combined)],check=True)
 subprocess.run([tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'pioneer_q38_recurrent_controller.ld'),
  str(root/'start.o'),str(root/'program.o'),str(combined),str(old/'unchanged-feedback/feedback.o'),'-o',str(image)],check=True)
 assert not subprocess.check_output([tool+'nm','-u',str(image)]).strip()
 for section in ('bf16','text','terminal','conv'):
  subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(image),str(tmp/(section+'.bin'))],check=True)
 (tmp/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(image)]))
 common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined']
 subprocess.run([*common,*['-D'+r+'=1' for r in rootRoles],str(here/'pioneer_q38_small_controller_bridge_test.cpp'),'-o',str(rootBinary)],check=True)
 subprocess.run([*common,*['-D'+r+'=1' for r in roles],'-I'+str(private),str(here/'pioneer_q38_small_worker_bridge_test.cpp'),'-o',str(workerBinary)],check=True)
 env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
 workerArgs=[str(workerBinary),str(worker),str(leaf)]
 boots=subprocess.check_output([*workerArgs,'--boot',str(alpha),str(beta)],env=env)
 assert len(boots)==119*64*4
 jobs=subprocess.run([str(rootBinary),str(tmp),'--reuse-jobs' if reuse else '--jobs'],input=boots,stdout=subprocess.PIPE,check=True,env=env).stdout
 assert len(jobs)==((20*2+2)+2 if reuse else 2*(20*2+2))*64*4
 results=subprocess.run([*workerArgs,'--reuse-run' if reuse else '--run',str(alpha),str(beta)],input=jobs,stdout=subprocess.PIPE,check=True,env=env).stdout
 assert len(results)==2*119*4824
 if reuse:
  for mode in ('--reuse-chain','--reuse-bad-input','--reuse-bad-native','--reuse-bad-prior'):
   subprocess.run([str(rootBinary),str(tmp),mode],input=boots+results,check=True,env=env)
 else:
  for mode in ('--publish','--corrupt','--corrupt-producer','--full','--full-corrupt-prior','--chain','--chain-corrupt-prior'):
   subprocess.run([str(rootBinary),str(tmp),mode],input=boots+results,check=True,env=env)
 if followup:
  gate,qkv,learned,output,conv=followup
  if not reuse:
   controllerValues=subprocess.run([str(rootBinary),str(tmp),'--chain-export'],input=boots+results,
                                  stdout=subprocess.PIPE,check=True,env=env).stdout
   assert len(controllerValues)==96*4
   subprocess.run([sys.executable,str(here/'run_pioneer_q38_full_recurrent_bfp_lifecycle_test.py'),
                  *map(str,(sdk,worker,leaf,private,gate,qkv,alpha,beta,learned,output,conv)),
                  '--controller-input'],input=controllerValues,check=True,env=env)
  warmJobs=subprocess.run([str(rootBinary),str(tmp),'--warm-reuse-jobs' if reuse else '--warm-jobs'],input=boots,stdout=subprocess.PIPE,check=True,env=env).stdout
  assert len(warmJobs)==2*42*64*4
  warmSnapshots=subprocess.run([sys.executable,str(here/'run_pioneer_q38_full_recurrent_bfp_lifecycle_test.py'),
                  *map(str,(sdk,worker,leaf,private,gate,qkv,alpha,beta,learned,output,conv)),
                  '--capture-controller-reuse' if reuse else '--capture-controller'],input=warmJobs,stdout=subprocess.PIPE,check=True,env=env).stdout
  assert len(warmSnapshots)==2*119*4824
  for mode in (('--reuse-chain','--reuse-bad-input','--reuse-bad-native','--reuse-bad-prior') if reuse else ('--chain','--chain-corrupt-prior')):
   subprocess.run([str(rootBinary),str(tmp),mode],input=boots+warmSnapshots,check=True,env=env)
for path,digest in pins+rootPins:assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
