#!/usr/bin/env python3
"""Source-frozen main/clean ROOT timing qualification, NO device/native IO."""
import ast
import hashlib
import os
from pathlib import Path
import json
import shutil
import subprocess
import sys
assert __debug__ and len(sys.argv)==5
sdk,control,discovery,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert all(p.is_absolute() for p in (sdk,control,discovery,out))
assert not out.exists() and not out.is_symlink() and not out.is_relative_to(root)
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);host=os.environ.get('CXX','c++')
public={Path(__file__).resolve()}
for file in discovery.glob('*.d'):
 data=file.read_text().replace('\\\n','')
 assert data.count(':')==1
 for item in data.split(':',1)[1].split():
  p=Path(item).resolve()
  if p.is_relative_to(root):public.add(p)
for name in ('pioneer_q38_fast_decode_profile_data_test.cpp','pioneer_q38_fast_decode_profile_image_test.cpp'):
 data=subprocess.check_output([host,'-std=c++17','-MM',str(here/name)],text=True)
 for rule in data.replace('\\\n','').splitlines():
  if not rule.strip():continue
  assert ':' in rule
  for item in rule.split(':',1)[1].split():
   p=Path(item).resolve()
   if p.is_relative_to(root):public.add(p)
for name in ('build_qwen38_fast_decode_profile_root.py','build_qwen38_full_offload_batched_root.py',
             'generate_qwen38_token_programs.py','blackhole_qwen38_global_chain.ld','PioneerQ38FullOffloadVectorQ4Programs.h'):
 public.add(here.parent/name)
public.add(here/'pioneer_q38_fast_decode_profile_role_test.py')
# The generic refusal reader loads literal pin sets from these scripts;
# it must reject artifact identity, not fail because a clean input is absent.
for node in ast.walk(ast.parse((here.parent/'generate_qwen38_token_programs.py').read_text())):
 if isinstance(node,ast.Constant) and isinstance(node.value,str) and node.value.endswith('.py') \
  and node.value.startswith(('build_qwen38_','tests/run_pioneer_q38_')):
  public.add(here.parent/node.value)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in public}
clean=out/'clean-source';clean_pins={}
preexisting={'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadMetricsRole.h',
 'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h','src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h',
 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc','src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeEnd.inc',
 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp','tools/pioneer/NATIVE_BUILD_STATUS.md','tools/pioneer/build_qwen38_full_offload_native.py'}
for p in public:
 relative=str(p.relative_to(root));target=clean/relative;target.parent.mkdir(parents=True,exist_ok=True)
 if relative in preexisting:target.write_bytes(subprocess.check_output(['git','show','HEAD:'+relative],cwd=root))
 else:shutil.copyfile(p,target)
 clean_pins[relative]=hashlib.sha256(target.read_bytes()).hexdigest()
# Frozen reusable accepted control inputs: builders still relink/check the
# ENTIRE old ELF and recompile all three original TUs before every new build.
saved=(control/'build-argv.txt').read_text().splitlines();link=saved[1].split('\t')
entry=Path(link[link.index('-T')+2]);feedback=next(Path(p) for p in link if Path(p).name=='feedback.o')
base=control/'unchanged-output-closure'
inputs={control/name for name in ('build-argv.txt','program.elf','boot.bin','text.bin','terminal.bin','conv.bin','token.o')}
inputs.update(base/name for name in ('compile-recurrent-argv.txt','compile-dense-argv.txt','output-closure-recurrent.o',
                                    'output-closure-dense.o','components-NOBOOT.o'))
inputs.update((entry,feedback,feedback.with_name('root_conv_sections.o')))
input_pins={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
def same():
 assert all(hashlib.sha256(p.read_bytes()).hexdigest()==v for p,v in frozen.items())
 assert all(hashlib.sha256((clean/p).read_bytes()).hexdigest()==v for p,v in clean_pins.items())
 assert all(hashlib.sha256(p.read_bytes()).hexdigest()==v for p,v in input_pins.items())
exits={};commands=[]
def run(name,command):
 same();commands.append([name,*map(str,command)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  result=subprocess.run(list(map(str,command)),stdout=stdout,stderr=stderr,cwd=out,
   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1',PYTHONDONTWRITEBYTECODE='1'))
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert not result.returncode and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_FAST_DECODE_LOCAL',name,result.returncode,flush=True)
for label,source in (('main',root),('clean',clean)):
 t=source/'tools/pioneer';tests=t/'tests';image=out/(label+'-image')
 run(label+'-build',[sys.executable,t/'build_qwen38_fast_decode_profile_root.py',sdk,control,image])
 for kind in ('data','image'):
  binary=out/(label+'-'+kind+'-test')
  run(label+'-'+kind+'-compile',[host,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
   tests/('pioneer_q38_fast_decode_profile_'+kind+'_test.cpp'),'-o',binary])
  run(label+'-'+kind,[binary,*([image] if kind=='image' else [])])
 run(label+'-roles',[sys.executable,tests/'pioneer_q38_fast_decode_profile_role_test.py',image])
 # Existing program families must reject this complete new ROOT before
 # opening a worker or producing any output. Never use their old owner.
 options=('--cancel-boundary','--no-cancel','--precise-activation','--pushed-completion','--fault-marker',
 '--precise-beta','--job-boot-fold','--token-frontier','--retained-conv-closure','--commit-boundary-seals',
 '--cohort-local-sources','--packet-identity-boundary','--tid-boundary','--counter-gather','--empty-receipt-poll',
 '--flat-matrix-grammar','--tile-prefetch','--batched-prefetch','--twin-batch-prefetch','--batched-output-closure','--wide-gdn')
 for number,option in enumerate(options):
  target=out/(label+'-REFUSED-'+str(number)+'.h');same()
  result=subprocess.run([sys.executable,str(t/'generate_qwen38_token_programs.py'),str(image),str(out/'MUST_NOT_OPEN_WORKER'),
   str(target),option],capture_output=True,text=True,env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
  assert result.returncode and result.stderr.startswith('PROGRAM_DATA_REFUSAL artifact ') and not target.exists()
  assert 'FileNotFoundError' not in result.stderr
 print('CLOSED_FAST_DECODE_OLD_PROGRAM_REFUSALS',label,len(options),flush=True)
same()
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','fast_decode.bin','program.elf'):
 assert (out/'main-image'/name).read_bytes()==(out/'clean-image'/name).read_bytes(),name
for kind in ('data','image','roles'):
 assert (out/('main-'+kind+'.stdout')).read_bytes()==(out/('clean-'+kind+'.stdout')).read_bytes()
assert hashlib.sha256((root/'tools/pioneer/PioneerQ38FullOffloadVectorQ4Programs.h').read_bytes()).hexdigest()=='3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389'
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'clean-source-sha256.txt').write_text(''.join(clean_pins[p]+'  '+p+'\n' for p in sorted(clean_pins)))
(out/'control-input-sha256.txt').write_text(''.join(input_pins[p]+'  '+str(p)+'\n' for p in sorted(input_pins)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_FAST_DECODE_ROOT_QUALIFICATION children'+str(len(exits))+'_stderr0_publicSources'+str(len(frozen))+
 '_reusableControlInputs'+str(len(input_pins))+'_mainCleanAll6ImagesExact_legacyVectorHeaderUnchanged;'
 'DIRECT_TIMING_HELPERS_ONLY_NOT_FULL_ROOT_MODEL_NATIVE_HARDWARE_OR_SPEED',flush=True)
