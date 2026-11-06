#!/usr/bin/env python3
"""Verify saved bounded offboard receipts,not live native admission."""
import hashlib
import json
from pathlib import Path
import subprocess

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
base='df603cfd4eee7330ed15e2a05e40744ee842d9d9'
different={'src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc',
 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeEnd.inc',
 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp','tools/pioneer/build_qwen38_full_offload_native.py'}
expected={
 '':{'root-and-worker','generate','native-build','roles','footprint-compile','footprint','journal-positive','journal-faults','journal-capsule'},
 'root-and-worker/':{'root-build','bf16-roles','old-precise-roles','worker-suite'},
 'root-and-worker/worker-suite/':{'build','producer','cached-regression','fpu-compile','fpu','connected-compile','connected'},
 'root-and-worker/worker-suite/producer-tests/':{'bridge-compile','shape-compile','worker-compile','worker','fraction'},
 'root-and-worker/worker-suite/cached-regression/':{'build','contract-compile','contract','image-compile','image','fault-compile','fault','warm-compile','warm','original-compile','original','fraction'},
 'bootstrap/':{'bootstrap'},
}
manifests={}
for side in ('main','clean'):
 directory=here/side
 for prefix,names in expected.items():
  exits=json.loads((directory/prefix/'actual-exits.json').read_text())
  assert set(exits)==names and set(exits.values())=={0}
  for name in names:assert not (directory/prefix/(name+'.stderr')).read_bytes()
 for prefix,count in (('',558),('native/',362),('root-and-worker/',274),('bootstrap/',178)):
  rows=(directory/prefix/'source-sha256.txt').read_text().splitlines();assert len(rows)==count
  current={}
  for row in rows:
   digest,name=row.split('  ',1);current[name]=digest
   data=subprocess.check_output(['git','show',base+':'+name],cwd=root) if side=='clean' and name in different else (root/name).read_bytes()
   assert hashlib.sha256(data).hexdigest()==digest,(side,name)
  if not prefix:manifests[side]=current
 assert 'children9_stderr0_sources558' in (directory/'suite.stdout').read_text()
 assert 'positive4/refusals472' in (directory/'roles.stdout').read_text()
 assert 'ALL15 section bytes/ALL120 exact code3262511 words/snapshot15444538 words=61778152bytes' in (directory/'footprint.stdout').read_text()
 assert 'saved15444538/installed3262511/restored15444538/secondOriginal30889076' in (directory/'journal-positive.stdout').read_text()
 assert 'faults52/inert preconditionRefusals33' in (directory/'journal-faults.stdout').read_text()
 assert 'HELD-journal faults27' in (directory/'journal-capsule.stdout').read_text()
 bootstrap=(directory/'bootstrap/bootstrap.stdout').read_text()
 assert '8 session/prompt faults' in bootstrap and '8 packet/counter/source/session/header/outer' in bootstrap
 assert 'SAME119 still virgin/no peer jobs/first returning entry reached/stack reserved2992 written2984' in bootstrap
 assert 'sources178' in (directory/'bootstrap/suite.stdout').read_text()
 assert 'maxFrame2560' in (directory/'native-build.stdout').read_text()
 assert 'producers68/packets272/17408 BF16 values' in (directory/'root-and-worker/worker-suite/connected.stdout').read_text()
 assert 'receiptRefusals10280' in (directory/'root-and-worker/worker-suite/fpu.stdout').read_text()
 assert '248320 vocabulary IDs+5 out-of-range' in (directory/'root-and-worker/root-build.stdout').read_text()
assert manifests['main'].keys()==manifests['clean'].keys()
assert {name for name in manifests['main'] if manifests['main'][name]!=manifests['clean'][name]}==different
for name in ('artifact-sha256.json','roles.stdout','footprint.stdout','journal-positive.stdout','journal-faults.stdout','journal-capsule.stdout',
 'bootstrap/bootstrap.stdout','bootstrap/source-sha256.txt','root-and-worker/artifact-sha256.json'):
 assert (here/'main'/name).read_bytes()==(here/'clean'/name).read_bytes(),name
artifacts=json.loads((here/'main/artifact-sha256.json').read_text())
assert artifacts['native/tenstorrent_pci_probe-bf16']==[2884184,'d8b1c561c9895841c6389576b53027d6e6998b3761dac12f47dbec961fb8bbb9']
assert artifacts['programs.h'][1]=='2df93172725190daa7f8b24f27a6af986850f22a2da6d8c235727643667c8d04'
print('PASS SAVED QTP4 main+clean9+4+7+5+12 children0/61778152B actual held journal/52+27 faults/472 role refusals/'
 'actual cold ROOT+SAME119 boot/16 emitted faults/178 frozen bootstrap sources;'
 'NOT whole-layer/token/installed native/running lifecycle/physical/model-quality/TPS proof')
