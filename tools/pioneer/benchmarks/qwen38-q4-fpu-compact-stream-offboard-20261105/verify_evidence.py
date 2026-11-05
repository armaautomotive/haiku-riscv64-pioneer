#!/usr/bin/env python3
"""Saved offline DATA/source checks only,never native cleanup authority."""
import hashlib
import json
import os
from pathlib import Path
import re
import sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
assert len(sys.argv)==1
here=Path(__file__).resolve().parent;root=here.parents[3]
main,clean=here/'main',here/'clean'
for name in ('parent-main.stderr','parent-clean.stderr'):
    assert not (here/name).read_bytes()
assert (here/'parent-main.stdout').read_bytes()==(here/'parent-clean.stdout').read_bytes()
def same_children(a,b,count):
    exits=json.loads((a/'actual-exits.json').read_text())
    assert len(exits)==count and set(exits.values())=={0}
    assert (a/'actual-exits.json').read_bytes()==(b/'actual-exits.json').read_bytes()
    for name in exits:
        for suffix in ('.stdout','.stderr'):
            data=(a/(name+suffix)).read_bytes()
            assert data==(b/(name+suffix)).read_bytes(),name+suffix
            if suffix=='.stderr':assert not data
same_children(main,clean,17)
same_children(main/'block-regression',clean/'block-regression',15)
assert (main/'source-sha256.txt').read_bytes()==(clean/'source-sha256.txt').read_bytes()
seen=set()
for row in (main/'source-sha256.txt').read_text().splitlines():
    digest,name=row.split('  ',1);path=root/name
    assert re.fullmatch('[0-9a-f]{64}',digest) and name not in seen
    assert path.resolve().is_relative_to(root) and path.is_file() and not path.is_symlink()
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,name
    seen.add(name)
assert len(seen)==76
for width in (5120,17408):
    text=(main/('image'+str(width)+'.stdout')).read_text()
    assert len(re.findall(r'^Q4_COMPACT_STREAM_REFERENCE ',text,re.M))==64
    assert 'ONE clear/job,SP432/touched420;' in text
assert (main/'role-refusals.txt').read_text()==(clean/'role-refusals.txt').read_text()=='PASS_COMPACT_STREAM_IMAGE_REFUSALS 31 definedZeroAndWrongWidths_NO_NATIVE_OR_WORK\n'
assert '32 unknown/frame/NIU/config/completion/producer failures' in (main/'fault.stdout').read_text()
assert '9 numeric controls(missing metadata stores/four phases/zero input)' in (main/'fault.stdout').read_text()
assert (main/'model.stdout').read_text().endswith('PASS_PRIVATE_COMPACT_FULL_K_STREAM cases9_jobs18_words576_originalWeights2654208_sameOpenFullSHA_epochNanoIdentityStable_NO_NATIVE_MODEL_QUALITY_OR_SPEED\n')
print('PASS_ARCHIVED_COMPACT_STREAM_EVIDENCE children17_nested15_currentSource76_streamsAndExitsExact;NOT_NATIVE_MODEL_QUALITY_OR_SPEED_AUTHORITY')
