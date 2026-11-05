#!/usr/bin/env python3
"""PARTIAL unqualified actual queued worker experiment; NO hardware/audit grant."""
import ast,hashlib,os,pathlib,re,subprocess,sys,tempfile
from compare_pioneer_q38_observed_trace import require,stable_bytes
require(__debug__ and not os.environ.get('PYTHONOPTIMIZE'),'checks required')
require(len(sys.argv)==4 and sys.argv[3] in ('--pilot','--group-pilot','--all-shapes'),'REPLAY CANDIDATE --pilot|--group-pilot|--all-shapes ONLY')
require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'no role override')
here=pathlib.Path(__file__).resolve().parent
image,candidate=map(pathlib.Path,sys.argv[1:3])
control=image/'unchanged-prepared/unchanged-original'
fixture=control/'unchanged-vector/unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
def pins(path,key='PINS'):
 return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==key for t in n.targets))
def snapshot(directory,records):
 require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink() and bool(records),'real immutable inputs')
 result=[]
 for name,size,digest in records:
  data=stable_bytes(directory/name,size);require(hashlib.sha256(data).hexdigest()==digest,'exact whole '+name);result.append((name,data))
 return result
snapshot(image,pins(here.parent/'build_qwen38_two_product_q4_worker.py','REPLAY_PINS'))
images=[snapshot(fixture,pins(here/'run_pioneer_q38_mixed_worker_tests.py')),
 snapshot(control,pins(here.parent/'build_qwen38_exact_q4_work_worker.py')),
 snapshot(candidate,pins(here.parent/'build_qwen38_queued_q4_block_worker.py','INSPECT_PINS'))]
legacy=[(name+'.bin',size,digest) for name,size,digest in pins(here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py')]
legacy.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
old=snapshot(fixture/'unchanged-worker',legacy)
with tempfile.TemporaryDirectory(prefix='pioneer-queued-q4-inspect-',dir='/private/tmp') as name:
 root=pathlib.Path(name);directories=[]
 for side,items in enumerate(images):
  target=root/('EXACT_UNQUALIFIED_ROM_'+str(side));target.mkdir(mode=0o700);directories.append(target)
  for filename,data in items:
   path=target/filename
   with path.open('xb') as stream:stream.write(data)
   path.chmod(0o400)
  (target/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(target/'program.elf')]))
  rows=[r.split() for r in subprocess.check_output([tool+'readelf','-sW',str(target/'program.elf')],text=True).splitlines()]
  functions=[r for r in rows if len(r)==8 and r[3]=='FUNC' and r[6].isdigit() and r[2].isdigit() and int(r[2])]
  require(bool(functions),'actual ELF FUNC extents')
  (target/'program.functions').write_text(''.join(r[7]+' FUNC '+r[1]+' '+r[2]+'\n' for r in functions))
 old_directory=directories[0]/'unchanged-worker';old_directory.mkdir(mode=0o700)
 for filename,data in old:
  path=old_directory/filename
  with path.open('xb') as stream:stream.write(data)
  path.chmod(0o400)
 flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
 flags+=['-DPIONEER_Q38_TWO_PRODUCT_WORKER_TEST=1','-DPIONEER_Q38_TWO_PRODUCT_REPLAY_WORKER_TEST=1',
         '-DPIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_TEST=1','-DPIONEER_Q38_QUEUED_Q4_BLOCK_WORKER_TEST=1']
 binary=root/'pilot'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_prepared_exact_q4_worker_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*map(str,directories),*([] if sys.argv[3]=='--all-shapes' else [sys.argv[3]])],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PARTIAL_UNQUALIFIED actual queued block worker;NO whole relocation/body audit,full/fault/history/native/model/hardware/TPS qualification',flush=True)
