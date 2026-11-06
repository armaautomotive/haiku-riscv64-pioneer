#!/usr/bin/env python3
"""Frozen local qualification of diagnostic111;NO staging/device access."""
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import sys
assert __debug__ and len(sys.argv)==6 and not os.environ.get('PYTHONOPTIMIZE')
sdk,private,original,out=map(Path,sys.argv[1:5]);generation=int(sys.argv[5]);assert 2<=generation<2**63
assert all(p.is_absolute() for p in (sdk,private,original,out)) and sdk.is_dir() and private.is_dir() and original.is_file()
assert not out.exists() and not out.is_symlink() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parents[2]
assert not out.is_relative_to(root)
# Broad source snapshot catches edits during compilation;only consumed
# sources are archived below. SDK and private inputs are separate dependencies.
suffixes={'.h','.hpp','.cpp','.c','.inc','.py','.sh','.S','.ld'}
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for folder in
 (root/'tools/pioneer',root/'src/add-ons/kernel',root/'headers/private',root/'build/config_headers')
 for p in folder.rglob('*') if p.is_file() and p.suffix in suffixes}
private_pins={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in private.glob('PioneerNativeCompactQ4Stream*.h')}
out.mkdir(mode=0o700);exits={};commands=[]
def same():
 assert all(hashlib.sha256(p.read_bytes()).hexdigest()==v for p,v in frozen.items())
 assert all(hashlib.sha256((private/n).read_bytes()).hexdigest()==v for n,v in private_pins.items())
def run(name,args):
 same();commands.append([name,*map(str,args)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  r=subprocess.run(list(map(str,args)),stdout=stdout,stderr=stderr,env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
 exits[name]=r.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert r.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_COMPARE111_LOCAL',name,r.returncode,flush=True)
run('native-build',[sys.executable,here.parent/'build_native_compact_q4_stream_diagnostic.py',sdk,private,original,out/'native','--factored-compare'])
run('client-build',[sys.executable,here.parent/'build_native_factored_q4_compare_client.py',sdk,private,out/'client',generation])
parent=(here.parent/'pioneer_native_factored_q4_compare_physical.sh').read_text()
for field,path in (('candidate_hash',out/'native/tenstorrent_pci_probe-native-q4-fpu-stream111'),('client_hash',out/'client/pioneer-native-factored-q4-compare')):
 expected=re.search(r'^'+field+r'=([0-9a-f]{64})$',parent,re.M);assert expected
 assert hashlib.sha256(path.read_bytes()).hexdigest()==expected.group(1),field
run('native-endpoints',[sys.executable,here/'run_pioneer_native_compact_q4_stream_native_tests.py',out/'native',private,out/'endpoints','--factored-compare'])
compiler=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
units=['pioneer_q38_factored_q4_compare_test.cpp','pioneer_native_factored_q4_compare_lease_test.cpp','pioneer_native_factored_q4_private_test.cpp']
for i,unit in enumerate(units):
 binary=out/('test'+str(i));args=[*compiler]
 if i==2:args+=['-I'+str(private),'-I'+str(out/'native')]
 args+=[here/unit]
 if i==2:args+=[here.parent/'PioneerModelWeightCache.cpp']
 run('compile'+str(i),[*args,'-o',binary])
 run('test'+str(i),[binary,out/'native/image5120',*([original] if i==2 else [])])
run('parent-gate',[sys.executable,here/'pioneer_native_factored_q4_parent_test.py'])
run('old-clock-default',[sys.executable,here.parent/'build_q38_q4_kernel_timing.py',sdk,out/'old-clock',5120])
same();dependencies={Path(__file__).resolve(),here/'pioneer_native_factored_q4_parent_test.py',here.parent/'pioneer_native_factored_q4_compare_physical.sh',
 here.parent/'build_native_compact_q4_stream_diagnostic.py',here.parent/'build_native_factored_q4_compare_client.py',
 here.parent/'build_q38_factored_q4_compare.py',here/'run_pioneer_native_compact_q4_stream_native_tests.py'}
def collect(command):
 data=subprocess.check_output(command,text=True)
 for name in data.replace('\\\n','').split(':',1)[1].split():
  p=Path(name).resolve()
  if p.is_relative_to(root):dependencies.add(p)
for line in (out/'native/build-argv.txt').read_text().splitlines():
 argv=line.split('\t')
 if '-c' not in argv:continue
 j=argv.index('-o');del argv[j:j+2];argv[argv.index('-c')]='-M';collect(argv)
for i,unit in enumerate(units):
 collect([compiler[0],'-std=c++17','-I'+str(private),'-I'+str(out/'native'),'-M',str(here/unit)])
dependencies.update(here.parent/('blackhole_q38_factored_q4_compare'+s) for s in ('.cpp','_start.S','.ld'))
dependencies.add(here.parent/'blackhole_q38_q4_kernel_clock.cpp')
for p in dependencies:assert p in frozen,p
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(dependencies)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
(out/'private-source-pins.json').write_text(json.dumps(private_pins,sort_keys=True,indent=2)+'\n')
print('PASS_COMPARE111_COMPLETE_LOCAL children'+str(len(exits))+'_stderr0_consumedSources'+str(len(dependencies))+'_NO_NATIVE_HARDWARE_EXECUTION_OR_TPS',flush=True)
