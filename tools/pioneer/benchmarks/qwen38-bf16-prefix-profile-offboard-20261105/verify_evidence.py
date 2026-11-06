#!/usr/bin/env python3
"""Verify saved instruction attribution,never physical speed or admission."""
import hashlib
import json
from pathlib import Path
import re

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
exits=json.loads((here/'actual-exits.json').read_text())
assert exits=={'compile':0,'profile':0}
for name in ('compile','profile'):assert not (here/(name+'.stderr')).read_bytes()
rows=(here/'source-sha256.txt').read_text().splitlines();assert len(rows)==183
for row in rows:
 digest,name=row.split('  ',1)
 assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
assert hashlib.sha256((here/'profile-map.txt').read_bytes()).hexdigest()=='09ddd19d0394389f7a61ebb71797d54b8fef20bfd8982a319844e19382aed4cd'
text=(here/'profile.stdout').read_text()
assert 'total138797427/functions105/EXCLUSIVE actual PCs incl SIMULATED polling' in text
assert 'reservedStack6496/5Q8+Conv+GDN real SAME119' in text
counts=[int(n) for n in re.findall(r'^ROOT_INSTRUCTION_FUNCTION (\d+) ',text,re.M)]
assert len(counts)==20 and counts[:4]==[34477441,33751273,22906200,11027898]
assert sum(counts[:3])==91134914 and sum(counts)<=138797427
assert 'children2_stderr0_sources183' in (here/'suite.stdout').read_text()
print('PASS SAVED actual unchanged cold ROOT instruction profile/2children0/183sources/'
 '138797427instructions/6496B prefix stack;NOT BF16 matrix/full model/physical cycles or TPS proof')
