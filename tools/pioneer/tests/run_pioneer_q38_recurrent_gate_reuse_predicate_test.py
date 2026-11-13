#!/usr/bin/env python3
"""Link the inspection controller and run its actual RV32 predicate offline."""
import ast
import hashlib
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile

assert __debug__ and len(sys.argv) in (4,9) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root,recurrent=map(Path,sys.argv[1:4]);here=Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (sdk,root,recurrent)) and sdk.is_dir() and root.is_dir() and recurrent.is_file()
old=root/'qualified-components';tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
pins=((root/'start.o','20ec8f9bb5690b079ad0dbb99dc18a9c89b15d940eef09a6dcf271c535383b46'),
 (root/'program.o','c9bc9bfd38ccc7000bbb2f7482dd25148dd3013b6db8d435b5da7e0718242037'),
 (old/'dense.o','b1960b90ba1b01b2e43b5889aeaab8a69f5db2f48cd103a340f6480941dfb9f2'),
 (old/'unchanged-feedback/root_conv_sections.o','e770a9bde85baadaf9239b7e40985af6415b99650866fad745fe4691a3196395'),
 (old/'unchanged-feedback/feedback.o','8c2626d34501e53dc25368cb8f67c48c9de778fec5d880e21901034795c083f2'))
for path,digest in pins: assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest
roles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION',
 'PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_SIMULATION',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_QGWE_BATCH_DENSE_PLAN','PIONEER_Q38_QGWF_DUAL_BATCH_PLAN',
 'PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION')
with tempfile.TemporaryDirectory(prefix='q38-gate-reuse-',dir='/private/tmp') as directory:
 tmp=Path(directory);raw=tmp/'raw.o';combined=tmp/'combined.o';image=tmp/'program.elf';test=tmp/'test'
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
 flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
        '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined']
 subprocess.run([os.environ.get('CXX','c++'),*flags,*['-D'+r+'=1' for r in roles],
  str(here/'pioneer_q38_recurrent_gate_reuse_predicate_test.cpp'),'-o',str(test)],check=True)
 env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
 subprocess.run([str(test),str(tmp)],check=True,env=env)
 if len(sys.argv)==9:
  worker,leaf,private,gate,qkv=map(Path,sys.argv[4:]);assert all(p.is_absolute() for p in (worker,leaf,private,gate,qkv))
  for path,digest in ((worker/'program.elf','67a9bf22e0f9713b699bb058db3cc5702f4940fb786845e66a3e89a3bfe2175d'),
   (leaf/'program.elf','e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
   (private/'PioneerNativeBfpModelBandData.h','396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a'),
   (private/'PioneerNativeBfpModelBandMetadata.h','a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d'),
   (gate,'e3c1189211445020054aeb42ccd89e0d968fb39acd35d0be99ff64755fb1f02b'),
   (qkv,'6146121bb87b7a684d544b73e06a62f0a6b05513866715bfdbff3dfc840d514a')):
   assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
  tree=ast.parse((here/'run_pioneer_q38_recurrent_small_dispatch_test.py').read_text())
  workerRoles=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='roles' for t in n.targets))
  rootbin=tmp/'root';workerbin=tmp/'worker';chainbin=tmp/'chain'
  for target,source,selected in ((rootbin,'pioneer_q38_recurrent_root_job_bridge.cpp',roles),
   (workerbin,'pioneer_q38_recurrent_worker_job_bridge.cpp',workerRoles),
   (chainbin,'pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp',workerRoles)):
   subprocess.run([os.environ.get('CXX','c++'),*flags,*['-D'+r+'=1' for r in selected],'-I'+str(private),str(here/source),'-o',str(target)],check=True)
  boot=subprocess.check_output([str(workerbin),str(worker),str(leaf),'--boot'],env=env);assert len(boot)==119*256
  gateJobs=subprocess.run([str(rootbin),str(tmp),'5'],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout
  qkvJobs=bytearray(subprocess.run([str(rootbin),str(tmp),'6','--after-gate'],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout)
  assert len(gateJobs)==len(qkvJobs)==20*512+119*256
  for w in range(20):
   at=20*512+w*256
   for field in (1,27):
    offset=at+field*4;value=struct.unpack_from('<I',qkvJobs,offset)[0];assert value==4;struct.pack_into('<I',qkvJobs,offset,3)
  retained=subprocess.run([str(chainbin),str(worker),str(leaf),str(gate),str(qkv),'--reuse-gate-capture'],
   input=gateJobs+qkvJobs,stdout=subprocess.PIPE,check=True,env=env).stdout
  prefix=(6144+10240)*4;snapshotSize=119*4824;assert len(retained)==prefix+2*snapshotSize
  gateActual=subprocess.run([str(rootbin),str(tmp),'5','--full-prepare'],input=boot+retained[prefix+snapshotSize:],
   stdout=subprocess.PIPE,check=True,env=env).stdout
  assert gateActual==retained[:6144*4]
  actual=subprocess.run([str(rootbin),str(tmp),'6','--gate-reuse-prepare'],input=boot+retained[prefix:prefix+snapshotSize],
   stdout=subprocess.PIPE,check=True,env=env).stdout
  assert actual==retained[6144*4:prefix]
  chained=subprocess.run([str(rootbin),str(tmp),'--gate-reuse-chain'],
   input=boot+retained[prefix+snapshotSize:]+boot+retained[prefix:prefix+snapshotSize],
   stdout=subprocess.PIPE,check=True,env=env).stdout
  assert chained==retained[:prefix]
  for failure in ('--gate-chain-bad-input','--gate-chain-bad-history'):
   rejected=subprocess.run([str(rootbin),str(tmp),failure],
    input=boot+retained[prefix+snapshotSize:]+boot+retained[prefix:prefix+snapshotSize],
    stdout=subprocess.PIPE,check=True,env=env).stdout
   assert rejected==retained[:6144*4]+b'PASS damaged gate history or input refused before dispatch; NO DEVICE\n'
  for failure in ('--gate-reuse-bad-history','--gate-reuse-bad-input'):
   rejected=subprocess.run([str(rootbin),str(tmp),'6',failure],input=boot+retained[prefix:prefix+snapshotSize],
    stdout=subprocess.PIPE,check=True,env=env).stdout
   assert rejected==b'PASS damaged gate history or input refused before dispatch; NO DEVICE\n'
  print('PASS ROOT gate and QKV Runs with retained ROOT and simulated worker histories; NO DEVICE OR SPEED')
