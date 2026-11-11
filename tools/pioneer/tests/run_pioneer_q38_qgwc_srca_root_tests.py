#!/usr/bin/env python3
"""Inspect the distinct QGWC ROOT/worker integration, never launch hardware."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

args=sys.argv[1:];qgwe='--qgwe' in args
if qgwe:
 assert args.count('--qgwe')==1;args.remove('--qgwe');args.append('--qgwd')
qgwd='--qgwd' in args
seal_loop='--qgwd-seal-loop' in args
if seal_loop:
 assert args.count('--qgwd-seal-loop')==1 and qgwd;args.remove('--qgwd-seal-loop')
if qgwd:
 assert args.count('--qgwd')==1;args.remove('--qgwd')
assert __debug__ and len(args)==5 and not os.environ.get('PYTHONOPTIMIZE')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,first,second,worker,leaf=map(Path,args)
assert all(p.is_absolute() and p.is_dir() for p in (sdk,first,second,worker,leaf))
here=Path(__file__).resolve().parent
pins={'boot.bin':'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca',
 'text.bin':'6f715be833854c950067b45431c7c4a0e8679b57801facb524440d8858526cf9',
 'terminal.bin':'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5',
 'conv.bin':'ddb5d683a42d12168d4fa48f3580ef018d8f6c96cee616aafa3c216466522ad4',
 'bf16.bin':'148a1d0897d07b69f0cd03d553a4f424e0da96f5cba4038ae8e0d991ecaf5f50',
 'program.elf':'d44e18940bcd37cf35bc2f124b314843cfd99314e8aaebd5d4983eb074918eb0'}
if qgwd:
 pins.update({'text.bin':'0299104019b7d72e9ec17421658be68851b38115d9f64bb9055810e3d6253e7c',
  'conv.bin':'009d1b82b67e5534c2a067826062734f8fa1dd736ab7f44a00a055b6ce72c6b1',
  'bf16.bin':'c9d80f4eae7934d282a09056ccc2d82ecf1ab73beaa066c2fafc5e2d53215a8b',
  'program.elf':'5cf33c2c4eaf010879acd03a99cdd166c931e0a8400cc082322d407fe841aa21'})
if seal_loop:
 pins.update({'text.bin':'ebbe7c9f13615671f48042b833920a22a173dc09b3cc1d5a0548d7bb72af474b',
  'bf16.bin':'61f4cf7f3ad3cbb1dcdd8292fd068195fa272c36e9b868c170f2471489f2c558',
  'program.elf':'2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95'})
if qgwe:
 pins.update({'text.bin':'1ebf298c1b7749bf7cef151f64c8b5d9deb81806a199b98970467c6bc35c3fbc',
  'conv.bin':'05781d446e5fcd2dd1ae191a0a2492a2557db0f147b9d79d9e5e1a1afd802daf',
  'bf16.bin':'f7306e10ccb7038944bac695e605b134ac32bc7887d972fdd31459cc6de2723b',
  'program.elf':'a10e4e5c0971d94b3a60e1a730a5d58f3aeccea67ceacd4c076903a9d554505e'})
for folder in (first,second):
 for name,digest in pins.items():assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,(folder,name)
for path,digest in ((worker/'program.elf','d3614997705d473f6a2a0c0f23dabcbf1418f587bf7686c3862bcd55c81aaa42' if qgwe else 'b9ab4753f2e99734cdbeb16b4fc0077688784c8b841a7e7aba128981a610b6f8' if qgwd else '3450e5b2d46b27e9ac3f81cb62de79d53b0613d85af0acc03a73a149e812e09e'),
                    (leaf/'text.bin','77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0' if qgwe else '4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')):
 assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
compiler=os.environ.get('CXX','c++');tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
if qgwd:
 dis=subprocess.check_output([tool+'objdump','-d','-j','.globalworker',str(worker/'program.elf')],text=True)
 assert 'b9042703' in dis and 'lw\ta4,-1136(s0)' in dis,'synthetic history offset must match pinned worker'
common=[compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
        '-ffp-contract=off','-fsanitize=address,undefined']
sim=['-DPIONEER_Q38_QGWD_PLAN_SIMULATION=1' if qgwd else '-DPIONEER_Q38_QGWC_PLAN_SIMULATION=1','-DPIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION=1',
     '-DPIONEER_Q38_HEAD_MATRIX_SIMULATION=1','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
     '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
if qgwe:sim+=['-DPIONEER_Q38_QGWE_BATCH_DENSE_PLAN=1']
worker_flags=['-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE=1','-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1',
 '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1','-DPIONEER_Q38_BF16_WORKER_IMAGE=1',
 '-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1','-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
if qgwd:worker_flags+=['-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1','-DPIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER=1']
if qgwe:worker_flags+=['-DPIONEER_Q38_QGWE_BATCH_DENSE_WORKER=1','-DPIONEER_Q38_QGW8_BFP8_BATCH_WORKER=1','-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1']
role=here.parent/('PioneerQ38QgwdRootImageRole.h' if qgwd else 'PioneerQ38QgwcSrcaRootImageRole.h')
root_role='PIONEER_Q38_QGWD_ROOT_IMAGE' if qgwd else 'PIONEER_Q38_QGWC_SRCA_ROOT_IMAGE'
names=[root_role,'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION',
 'PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP','PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE']
flags=['-D'+name+'=1' for name in names]
if qgwe:flags+=['-DPIONEER_Q38_QGWE_BATCH_DENSE_ROOT=1']
def rolecheck(options,okay,body=None,role_compiler=compiler):
 source=body if body is not None else '#include "'+str(role)+'"\n'
 result=subprocess.run([role_compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
 assert (result.returncode==0)==okay,result.stderr[:1000]
rolecheck(flags,True);rolecheck([f for f in flags if f!='-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1']+['-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1'],True)
rolecheck(['-D'+root_role+'=0',*flags[1:]],False)
for name in names[1:]:
 reduced=[flag for flag in flags if flag!='-D'+name+'=1'];rolecheck(reduced,False);rolecheck(reduced+['-D'+name+'=0'],False)
for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE','PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE',
 'PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE','PIONEER_Q38_QGW8_COARSE_PROFILE_IMAGE'):
 rolecheck(flags+['-D'+forbidden+'=1'],False)
if qgwd:
 for forbidden in ('PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION'):
  rolecheck(flags+['-D'+forbidden+'=1'],False)
include='#include "'+str(role)+'"\n'
rolecheck(flags,False,include+'#undef '+root_role+'\n'+include)
if qgwd:
 # The real ROOT compilation role must be able to construct the QGWD dense
 # job grammar without pretending to be a worker image or enabling native IO.
 dense_header=here.parent/'PioneerQ38DenseQ8MatrixRootFrame.h'
 dense_flags=flags+['-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1',
                    '-DPIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION=1','-DPIONEER_ATTENTION_FREESTANDING=1',
                    '-DPIONEER_DRAM_SWEEP_FREESTANDING=1']
 dense_body='#include "'+str(dense_header)+'"\nstatic_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic=='+('0x45574751' if qgwe else '0x44574751')+');\n'
 rolecheck(dense_flags,True,dense_body,tool+'g++')
 rolecheck([f for f in dense_flags if f!='-D'+root_role+'=1'],False,dense_body,tool+'g++')
 for forbidden in ('_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE'):
  rolecheck(dense_flags+['-D'+forbidden+'=1'],False,dense_body,tool+'g++')
with tempfile.TemporaryDirectory(prefix='qgwc-srca-root-',dir='/private/tmp') as directory:
 tmp=Path(directory);image=tmp/'image';image.mkdir()
 root_option='--qgwd-dense-q8-root' if qgwd else '--qgwc-srca-root'
 for extra in ([root_option],[root_option,'--qgwa-batch-root'],[root_option,root_option]):
  target=tmp/'rejected'
  result=subprocess.run([sys.executable,str(here.parent/'build_qwen38_token_root.py'),str(sdk),str(target),*extra],capture_output=True)
  assert result.returncode and not target.exists()
 for name in pins:shutil.copyfile(first/name,image/name)
 (image/'program.demangled').write_bytes(subprocess.check_output([tool+'nm','-nC',str(image/'program.elf')]))
 env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
 binaries={}
 for name,source,extra in (('plan','pioneer_q38_hybrid_head_plan_test.cpp',[sim[0]]+(['-DPIONEER_Q38_QGWE_BATCH_DENSE_PLAN=1'] if qgwe else [])),
                           ('actual','pioneer_q38_hybrid_head_actual_root_test.cpp',sim),
                           ('helpers','pioneer_q38_hybrid_head_root_helpers_test.cpp',sim),
                           ('peer','pioneer_q38_hybrid_head_worker_image_test.cpp',worker_flags)):
  binary=tmp/name;binaries[name]=binary
  subprocess.run([*common,*extra,str(here/source),'-o',str(binary)],check=True)
 job=tmp/'job';manifest=tmp/'manifest';metadata=tmp/'metadata';proof=tmp/'proof'
 for args in ([binaries['plan'],job,manifest,metadata], [binaries['peer'],worker,leaf,job,proof],
              [binaries['actual'],image,manifest], [binaries['helpers'],image,metadata,job,proof]):
  subprocess.run([str(x) for x in args],check=True,env=env)
 if qgwd:
  dense=tmp/'dense-producer-root'
  subprocess.run([*common,*sim,'-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1',
                  str(here/'pioneer_q38_qgwd_dense_producer_root_test.cpp'),'-o',str(dense)],check=True)
  subprocess.run([str(dense),str(image)],check=True,env=env)
print('PASS '+('QGWE' if qgwe else 'QGWD_SEAL_LOOP' if seal_loop else 'QGWD' if qgwd else 'QGWC')+' two pinned ROOT images and actual ROOT/worker jobs in simulation; NO full-model or hardware claim')
