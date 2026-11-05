#!/usr/bin/env python3
"""Frozen client qualification;optional private binary recheck.NO device access."""
import hashlib
import json
import pathlib
import re
import sys
assert __debug__ and len(sys.argv) in (1,2)
here=pathlib.Path(__file__).resolve().parent;root=here.parents[3]
m=json.loads((here/'manifest.json').read_text())
def sha(data):return hashlib.sha256(data).hexdigest()
assert m['version']==1 and len(m['files'])==44 and len(m['sources'])==27 and len(m['new_sources'])==11
for name,pin in m['files'].items():
 p=pathlib.PurePosixPath(name);assert not p.is_absolute() and '..' not in p.parts
 data=(here/p).read_bytes();assert len(data)==pin['bytes'] and sha(data)==pin['sha256'],name
 if name.endswith('.stderr'):assert not data
for name,pin in m['sources'].items():
 p=pathlib.PurePosixPath(name);assert not p.is_absolute() and '..' not in p.parts
 assert sha((root/p).read_bytes())==pin,name
assert all(name in m['sources'] for name in m['new_sources'])
labels=('sdk-client','parent-syntax','client','parent','parent-output','output','reference','source-compile','source')
for tree,session in (('main',25389),('clean',9075)):
 assert m['terminal'][tree]=={'session':session,'exit':0}
 assert (here/tree/'actual-child-exits.txt').read_text()==''.join(name+' 0\n' for name in labels)
 lines=(here/tree/'outer.log').read_text().splitlines();assert len(lines)==10
 for label,line in zip(labels,lines[:-1]):
  assert line==f'PASS_CLOSED {label} exit0 stdout_sha'+sha((here/tree/(label+'.log')).read_bytes())+' stderr0'
 assert lines[-1].startswith('PASS OFFBOARD native104 client/parent ')
for label in labels:
 left,right=[(here/tree/(label+'.log')).read_text() for tree in ('main','clean')]
 if label in ('client','parent'):
  pattern=r'/var/folders/[^\n]+/pioneer-q4-(?:client|parent)-test\.[^\n]+'
  assert len(re.findall(pattern,left))==len(re.findall(pattern,right))==1
  left,right=re.sub(pattern,'<FIXTURE_DIRECTORY>',left),re.sub(pattern,'<FIXTURE_DIRECTORY>',right)
 assert left==right,label
assert (here/'main/source-sha256.txt').read_bytes()==(here/'clean/source-sha256.txt').read_bytes()
assert m['sources']['tools/pioneer/pioneer_native_compact_q4_stream_physical.sh']==m['parent_sha256']
if len(sys.argv)==2:
 private=pathlib.Path(sys.argv[1]);assert private.is_absolute()
 for tree in ('main-b','clean-b'):
  data=(private/tree/'build/pioneer-native-compact-q4-stream-diagnostic').read_bytes()
  assert len(data)==m['client']['bytes'] and sha(data)==m['client']['sha256']
 print('PASS private1944800B SDK clients identical in BOTH trees;NO bytes printed')
print('PASS compact104 OFFBOARD client:44 exact receipts/27 identical current sources/11 new files/nine actual0 children per tree;NOT physical closure/full-model quality/tokens-sec proof')
