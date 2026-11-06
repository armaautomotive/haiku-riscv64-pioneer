#!/usr/bin/env python3
"""Saved prefix/USER evidence only;not live hardware admission."""
import hashlib
import json
from pathlib import Path

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
def sources(path,count):
 rows=path.read_text().splitlines();assert len(rows)==count
 for row in rows:
  digest,name=row.split('  ',1);assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
def children(directory,names):
 values=json.loads((directory/'actual-exits.json').read_text())
 assert set(values)==set(names) and set(values.values())=={0}
 for name in names:assert not (directory/(name+'.stderr')).read_bytes()
children(here/'prefix',('probe-compile','prefix-compile','prefix'))
sources(here/'prefix/source-sha256.txt',199)
prefix=(here/'prefix/prefix.stdout').read_text()
assert '5 Q8 projections363 jobs->119 Conv+96 GDN closures' in prefix
assert 'BEFORE submission;SAME119 real recipe-derived histories,stack6496' in prefix
for side in ('main','clean'):
 directory=here/side
 children(directory,('protocol-compile','protocol','session-compile','session','sdk-build'))
 sources(directory/'source-sha256.txt',59);sources(directory/'sdk/source-sha256.txt',55)
 assert '3072 valid/12338 refusals' in (directory/'protocol.stdout').read_text()
 assert '197 fail-closed COW fixtures' in (directory/'session.stdout').read_text()
 build=(directory/'sdk-build.stdout').read_text()
 assert '12 role refusals/unchanged tag88 USER pins' in build
 assert '77584 0190291350ca5eb56a235fcf6c2d6e3c000a7146f9f4b86fd7cd28a13051feec' in build
 assert '224952 aa9aac510e66d7bfba98b61019181ddc6662639badbb7ebc4edcdb2f91d822ef' in build
for name in ('source-sha256.txt','sdk/source-sha256.txt','protocol.stdout','session.stdout'):
 assert (here/'main'/name).read_bytes()==(here/'clean'/name).read_bytes(),name
for name in ('parent-main','parent-clean','parent-original','parent-batched','smoke-main'):
 assert not (here/(name+'.stderr')).read_bytes()
 assert (here/(name+'.stdout')).read_text().startswith('PASS actual full-card')
assert 'BF16-105 parent: 54' in (here/'parent-main.stdout').read_text()
assert (here/'parent-main.stdout').read_bytes()==(here/'parent-clean.stdout').read_bytes()
assert hashlib.sha256((root/'tools/pioneer/pioneer_q38_bf16_full_offload_physical.sh').read_bytes()).hexdigest()=='aa6baf6d22cd4bdca6521ffce66d46451b49abf9e748a5c7b978c527aaf007b8'
assert hashlib.sha256((root/'tools/pioneer/tests/pioneer_q38_full_offload_parent_test.py').read_bytes()).hexdigest()=='05eb597850ab07c5991924e4b53132b5dc1c0d388a2730f16721443addaf8507'
stack=json.loads((here/'stack-inspect.json').read_text())
assert stack['conservativeBytes']==7936 and stack['stackBytes']==8192 and stack['functions']==209 and stack['calls']==1019
print('PASS SAVED actual cold ROOT prefix/363Q8+119Conv+96GDN/199 frozen sources;USER main+clean5children0/'
 '59+55sources/197 transport faults/54 guarded-parent cases/7936B static stack bound;'
 'NOT BF16 matrix/full-model/native execution/physical/TPS proof')
