#!/usr/bin/env python3
"""Replay saved worker/component receipts,NOT current device admission."""
import hashlib
import json
from pathlib import Path
assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
for row in (here/'evidence-sha256.txt').read_text().splitlines():
 digest,name=row.split('  ',1);path=root/name
 assert path.is_relative_to(root) and hashlib.sha256(path.read_bytes()).hexdigest()==digest,name
expected={'':{'build','producer','cached-regression','fpu-compile','fpu'},
 'producer-tests/':{'bridge-compile','shape-compile','worker-compile','worker','fraction'},
 'cached-regression/':{'build','contract-compile','contract','image-compile','image','fault-compile','fault','warm-compile','warm','original-compile','original','fraction'}}
for side in ('main','clean'):
 directory=here/side
 for prefix,names in expected.items():
  exits=json.loads((directory/prefix/'actual-exits.json').read_text())
  assert set(exits)==names and set(exits.values())=={0}
  for name in names:assert not (directory/prefix/(name+'.stderr')).read_bytes()
 for leaf,count in (('source-sha256.txt',187),('cached-regression/source-sha256.txt',60)):
  rows=(directory/leaf).read_text().splitlines();assert len(rows)==count
  for row in rows:
   digest,name=row.split('  ',1);assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
 assert 'jobs108/packedWords13824/faults18/maxCombinedStack3712' in (directory/'producer-tests/worker.stdout').read_text()
 fpu=(directory/'fpu.stdout').read_text()
 assert fpu.count('PASS ACTUAL full QGW4->linked returning FPU')==5
 assert 'jobs5/outputs608/maxCombinedStack2752/canonical stripe metadata jobs22848' in fpu
 assert 'metadata192 matrices/jobs22848/refusals2304' in fpu
 assert 'original288 exact outputs' in (directory/'cached-regression/original.stdout').read_text()
 assert '6 cases/704 outputs' in (directory/'cached-regression/image.stdout').read_text()
 assert 'requests/416 exact outputs' in (directory/'cached-regression/warm.stdout').read_text()
 assert 'fault/refusal cases32' in (directory/'cached-regression/fault.stdout').read_text()
 assert len((directory/'worker/role-refusals.txt').read_text().splitlines())==60
 assert 'children5_stderr0_source187' in (directory/'suite.stdout').read_text()
for path in (here/'main').rglob('*'):
 if not path.is_file() or path.name=='test-argv.json':continue
 a=path.read_bytes();b=(here/'clean'/path.relative_to(here/'main')).read_bytes()
 if path.relative_to(here/'main')==Path('build.stdout'):
  # ONLY output-directory spelling differs in unchanged ancestor logs.
  a=a.replace(b'/private/tmp/pioneer-bf16-worker.HPoJVzEU/qual-stripe-main',b'$OUT')
  b=b.replace(b'/private/tmp/pioneer-bf16-worker.HPoJVzEU/qual-stripe-clean',b'$OUT')
 assert a==b,str(path)
assert len((here/'binary-sha256.txt').read_text().splitlines())==17
print('PASS SAVED QGW4 main+clean5 top/5 producer/12 cached children0,stderr0/source187/'+
 '108 actual producer jobs+5 full-worker FPU jobs608 outputs/22848 canonical METADATA jobs;'+
 'HISTORICAL OFFBOARD,NOT connected ROOT/native/hardware/full-model quality/TPS authority')
