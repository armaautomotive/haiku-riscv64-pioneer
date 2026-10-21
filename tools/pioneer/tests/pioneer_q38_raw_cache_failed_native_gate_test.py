#!/usr/bin/env python3
"""Failed candidate must stop before SDK,filesystem,OS or device commands."""
import os,pathlib,subprocess,tempfile
directory=pathlib.Path(__file__).resolve().parent.parent
base={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cases=[]
for flag in ('PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION','PIONEER_Q38_WORK_RAW_UNPACK_CACHE'):
 for value in ('1','01','-1','invalid'):cases.append(('build_qwen38_banks_driver.sh',{flag:value}))
for script,flag in [('build_qwen38_banks_client.sh','PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT'),('build_qwen38_inference.sh','PIONEER_Q38_MODEL_THREAD_RAW_CACHE'),('build_qwen38_inference.sh','PIONEER_Q38_BENCHMARK_HOST_RAW_CACHE_QUALIFICATION')]:
 for value in ('1','01','-1','invalid'):cases.append((script,{flag:value}))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-raw-failed-gate-') as name:
 root=pathlib.Path(name);empty=root/'EMPTY_PATH';empty.mkdir()
 # Empty PATH proves the updater's rejection needs no uname/id/sha/mmio or
 # other external command;the failed path is refused by shell builtins only.
 r=subprocess.run(['/bin/sh',str(directory/'pioneer_p100a_hot_swap.sh'),'--apply-raw-unpack-cache-qualification'],env={**base,'PATH':str(empty)},capture_output=True,text=True)
 assert r.returncode==2 and 'STOP_RAW_CACHE_FAILED_NATIVE' in r.stderr and 'not found' not in r.stderr
 for script,flags in cases:
  sdk,libs,out=root/'MUST_NOT_OPEN_SDK',root/'MUST_NOT_OPEN_LIBRARIES',root/'MUST_NOT_CREATE_OUTPUT'
  args=[str(sdk),str(out)] if script!='build_qwen38_inference.sh' else [str(sdk),str(libs),str(out)]
  r=subprocess.run(['/bin/sh',str(directory/script),*args],env={**base,**flags,'PATH':str(empty)},capture_output=True,text=True)
  assert r.returncode==2 and 'STOP_RAW_CACHE_FAILED_NATIVE' in r.stderr and 'not found' not in r.stderr,(script,r.stderr)
  assert not sdk.exists() and not libs.exists() and not out.exists()
print(f'PASS failedtag22 native installer+{len(cases)}driver/debug/model gates beforeANY externalcommand/SDK/output;NO retry/reset/rollback or device access')
