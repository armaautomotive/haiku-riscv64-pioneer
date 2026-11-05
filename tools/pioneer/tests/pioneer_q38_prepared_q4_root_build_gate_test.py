#!/usr/bin/env python3
"""Compile-role refusal tests ONLY;not ROOT execution or native ownership."""
import hashlib,os,pathlib,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Refuse optimized Python;role checks are required')
assert len(sys.argv)==2
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
image=pathlib.Path(sys.argv[1]);assert image.is_absolute() and image.is_dir()
assert hashlib.sha256((image/'program.elf').read_bytes()).hexdigest()=='0306100598ee1aa4e20bf3399165f5fbc2714ab2a694021fa98d63f4f64bb8f1'
base=(image/'compile-recurrent-argv.txt').read_text().splitlines();assert base.count('-c')==base.count('-o')==1
source=base[base.index('-c')+1];at=base.index('-o');del base[at:at+2];base.remove('-c')
base.remove('-fstack-usage') # syntax-only role checks must not emit a default .su into the caller's cwd
at=base.index(source);base.insert(at,'-fsyntax-only')
positive=subprocess.run(base,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
assert positive.returncode==0,positive.stderr
cases=[('missing-selection',['-UPIONEER_Q38_PREPARED_Q4_ROOT_IMAGE']),
 ('zero-selection',['-UPIONEER_Q38_PREPARED_Q4_ROOT_IMAGE','-DPIONEER_Q38_PREPARED_Q4_ROOT_IMAGE=0']),
 ('missing-batched-parent',['-UPIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE']),
 ('kernel',['-D_KERNEL_MODE=1']),('driver-tag',['-DPIONEER_DRIVER_BUILD_TAG=1']),
 ('cpu-validation',['-DPIONEER_Q38_CPU_VALIDATION=1']),('cpu-reference',['-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1']),
 ('hybrid-work',['-DPIONEER_Q38_WORK_INFERENCE=1']),('fast-math',['-D__FAST_MATH__=1'])]
for name,flags in cases:
 command=base[:];at=command.index(source);command[at:at]=flags
 result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 assert result.returncode!=0 and ('Prepared Q4 requires the matching complete batched/pushed ROOT BUILD' in result.stderr
  or 'Prepared ROOT BUILD cannot alias native,math worker,CPU reference or Work authority' in result.stderr),(name,result.stderr)
 print('PASS_PREPARED_ROOT_ROLE_REFUSAL '+name,flush=True)
print('PASS_PREPARED_ROOT_BUILD_GATES refusals'+str(len(cases))+';BUILD_ONLY_NOT_EXECUTION_NATIVE_ROOT_OWNERSHIP_CARD_OR_TPS',flush=True)
