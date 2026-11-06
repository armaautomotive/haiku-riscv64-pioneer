#!/usr/bin/env python3
"""New trace component only;local compiled/data tests,no Pioneer access."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
assert __debug__ and len(sys.argv)==4 and not os.environ.get('PYTHONOPTIMIZE')
sdk,old,out=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent;root=here.parents[2]
assert sdk.is_absolute() and old.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
assert hashlib.sha256((old/'text.bin').read_bytes()).hexdigest()=='1e2cc3d675161216bcf5593a79ca483e37c384dab815b34009161cbf36dce280'
assert hashlib.sha256((old/'boot.bin').read_bytes()).hexdigest()=='d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
cc=os.environ.get('CXX','c++');cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
units=['pioneer_q38_q4_factored_trace_test.cpp','pioneer_q38_factored_q4_trace_image_test.cpp','pioneer_q38_factored_q4_compare_test.cpp','pioneer_q38_q4_factored_trace_report_test.cpp']
sources={Path(__file__).resolve(),here/'pioneer_q38_q4_factored_trace_roles_test.py',here.parent/'build_q38_factored_q4_trace.py'}
def collect(compiler,unit,flags=()):
    result=subprocess.check_output([compiler,'-std=c++17',*flags,'-MM',str(unit)],text=True)
    for name in result.replace('\\\n','').split(':',1)[1].split():
        path=Path(name).resolve();assert path.is_relative_to(root);sources.add(path)
for unit in units:collect(cc,here/unit)
collect(cross,here.parent/'blackhole_q38_factored_q4_trace.cpp',['-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_FACTORED_Q4_TRACE_BUILD=1','-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1'])
sources.update(here.parent/('blackhole_q38_factored_q4_trace'+suffix) for suffix in ('_start.S','.ld'))
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
out.mkdir(mode=0o700);exits={};commands=[]
def run(name,args):
    assert all(hashlib.sha256(p.read_bytes()).hexdigest()==sha for p,sha in frozen.items())
    commands.append([name,*map(str,args)])
    with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
        result=subprocess.run(list(map(str,args)),stdout=stdout,stderr=stderr)
    exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,indent=2)+'\n')
    assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
    print('CLOSED_TRACE_LOCAL',name,result.returncode,flush=True)
os.environ.update(ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1')
run('build',[sys.executable,here.parent/'build_q38_factored_q4_trace.py',sdk,out/'image'])
run('roles',[sys.executable,here/'pioneer_q38_q4_factored_trace_roles_test.py',sdk])
for name,unit,extra in [('issue-correspondence',units[0],[]),('compiled-trace',units[1],[out/'image']),('old-compiled-regression',units[2],[old]),('data-report',units[3],[])]:
    binary=out/name
    run(name+'-compile',[cc,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',here/unit,'-o',binary])
    run(name,[binary,*extra])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==sha for p,sha in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_TRACE_LOCAL children'+str(len(exits))+'_stderr0_sources'+str(len(frozen))+'_NO_NATIVE_HARDWARE_OR_SPEED_AUTHORITY',flush=True)
