#!/usr/bin/env python3
"""OFFBOARD full-K compact stream qualification;no device/cache writes."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
assert len(sys.argv)==4
build, original, out=map(Path,sys.argv[1:])
assert build.is_absolute() and build.is_dir() and original.is_absolute() and original.is_file() and not original.is_symlink()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parents[2];out.mkdir(mode=0o700)
exits={};commands=[];dependencies={Path(__file__).resolve()}
def run(name,argv):
    commands.append([name,*map(str,argv)])
    with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
        result=subprocess.run(list(map(str,argv)),stdout=stdout,stderr=stderr,check=False)
    exits[name]=result.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
    assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
    print('CLOSED_COMPACT_STREAM_OFFBOARD',name,result.returncode,flush=True)
def collect(compiler,unit,flags=()):
    data=subprocess.check_output([str(compiler),'-std=c++17',*flags,'-MM',str(unit)],text=True)
    dependencies.update(Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split())

# The new loader is the exact old checked algorithm under distinct names,
# a compact layout and its descriptive comment. No counter/credit/failure
# rule is relaxed for the new request count.
old=(here.parent/'PioneerQ38Q4FpuStreamLoader.h').read_text()
expected=old.replace('PIONEER_Q38_Q4_FPU_STREAM_LOADER_H','PIONEER_Q38_Q4_FPU_COMPACT_STREAM_LOADER_H')
expected=expected.replace('PioneerQ38Q4FpuStreamLayout','PioneerQ38Q4FpuCompactStreamLayout').replace('PioneerQ38Q4FpuStreamLoader','PioneerQ38Q4FpuCompactStreamLoader')
expected=expected.replace('Four14-request slices can overlap four groups','Two13-request slices can overlap two groups')
assert expected.rstrip('\n')==(here.parent/'PioneerQ38Q4FpuCompactStreamLoader.h').read_text().rstrip('\n')
dependencies.add(here.parent/'PioneerQ38Q4FpuStreamLoader.h')
for width in (5120,17408):
    run('build'+str(width),[sys.executable,here.parent/'build_q38_q4_fpu_compact_stream.py',build,out/('image'+str(width)),width,'--inspect-candidate'])
compiler=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
for name in ('layout','loader','pipeline','image','fault','model'):
    unit=here/('pioneer_q38_q4_fpu_compact_stream_'+name+'_test.cpp');binary=out/(name+'-test')
    extra=[here.parent/'PioneerModelWeightCache.cpp'] if name=='model' else []
    run(name+'-compile',[*compiler,unit,*extra,'-o',binary])
    for source in (unit,*extra):collect(compiler[0],source)
    if name=='image':
        for width in (5120,17408):run('image'+str(width),[binary,out/('image'+str(width)),width])
    else:
        arguments=[out/'image5120'] if name=='fault' else [out/'image5120',out/'image17408',original] if name=='model' else []
        run(name,[binary,*arguments])
run('fraction',[sys.executable,here/'pioneer_q38_q4_fpu_compact_stream_fraction_test.py',out/'image5120.stdout',out/'image17408.stdout'])
dependencies.update(here/name for name in ('pioneer_q38_q4_fpu_compact_stream_fraction_test.py','pioneer_native_bfp_arithmetic_test.py'))
run('block-regression',[sys.executable,here/'run_pioneer_q38_q4_fpu_compact_tests.py',build,original,out/'block-regression'])
for line in (out/'block-regression/source-sha256.txt').read_text().splitlines():
    digest,name=line.split('  ',1);path=root/name
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
    dependencies.add(path)

tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');unit=here.parent/'blackhole_q38_q4_fpu_compact_stream.cpp'
forbidden=('_KERNEL_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
 'PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','PIONEER_Q4_FPU_STREAM_K','PIONEER_Q38_COMPACT_Q4_FPU_IMAGE','__FAST_MATH__')
role='-DPIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE=1';width='-DPIONEER_Q4_FPU_COMPACT_STREAM_K=5120'
negative=[('missing',[width]),('zero',[width,role[:-1]+'0']),('two',[width,role[:-1]+'2'])]
negative += [(name+str(v),[width,role,'-D'+name+'='+str(v)]) for name in forbidden for v in (0,1)]
for label,flags in negative:
    result=subprocess.run([tool+'g++','-march=rv32i','-mabi=ilp32',*flags,'-E',str(unit)],capture_output=True,text=True)
    assert result.returncode and 'Distinct compact full-K OFFBOARD image only' in result.stderr,label
refused=len(negative)
for k in (0,5121,6144,17409):
    result=subprocess.run([tool+'g++','-std=c++17','-march=rv32i','-mabi=ilp32',role,'-DPIONEER_Q4_FPU_COMPACT_STREAM_K='+str(k),'-fsyntax-only',str(unit)],capture_output=True,text=True)
    assert result.returncode and 'exact Q4 model widths only' in result.stderr
    refused+=1
(out/'role-refusals.txt').write_text('PASS_COMPACT_STREAM_IMAGE_REFUSALS '+str(refused)+' definedZeroAndWrongWidths_NO_NATIVE_OR_WORK\n')
collect(tool+'g++',unit,['-march=rv32i','-mabi=ilp32',role,width])
for name in ('build_q38_q4_fpu_compact_stream.py','build_q38_q4_fpu_stream.py','blackhole_q38_q4_fpu_compact_stream_start.S',
             'blackhole_q38_q4_fpu_compact_stream.ld','blackhole_q38_q4_fpu_stream.cpp','blackhole_q38_q4_fpu_stream_start.S','blackhole_q38_q4_fpu_stream.ld'):
    dependencies.add(here.parent/name)
collect(tool+'g++',here.parent/'blackhole_q38_q4_fpu_stream.cpp',['-march=rv32i','-mabi=ilp32','-DPIONEER_Q4_FPU_STREAM_K=5120'])
records=[]
for path in sorted(dependencies,key=lambda p:str(p.relative_to(root))):
    assert path.is_relative_to(root) and path.is_file()
    records.append(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+str(path.relative_to(root)))
(out/'source-sha256.txt').write_text('\n'.join(records)+'\n')
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_COMPACT_FULL_K_COMPLETE_OFFBOARD children'+str(len(exits))+'_strictStderr0_roles'+str(refused)+'_source'+str(len(records))
      +'_old7824And4948Reproduced_NO_NATIVE_COHORT_MODEL_QUALITY_OR_TPS',flush=True)
