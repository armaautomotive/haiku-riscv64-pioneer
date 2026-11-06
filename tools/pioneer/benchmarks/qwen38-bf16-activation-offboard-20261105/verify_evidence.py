#!/usr/bin/env python3
"""Saved LOCAL producer evidence, not native/hardware/inference admission."""
import hashlib
import json
from pathlib import Path
assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
for row in (here/'evidence-sha256.txt').read_text().splitlines():
 digest,name=row.split('  ',1);path=root/name
 assert path.is_relative_to(root) and hashlib.sha256(path.read_bytes()).hexdigest()==digest,name
for folder in ('main','clean'):
 directory=here/folder;exits=json.loads((directory/'actual-exits.json').read_text())
 assert set(exits)=={'build','compile','producer','fraction'} and set(exits.values())=={0}
 for name in exits:assert not (directory/(name+'.stderr')).read_bytes()
 rows=(directory/'source-sha256.txt').read_text().splitlines();assert len(rows)==61
 for row in rows:
  digest,name=row.split('  ',1);assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
 assert 'accepted22/refused64/values5632/maxCombinedStack2304' in (directory/'producer.stdout').read_text()
 assert '128 actual RV32' in (directory/'fraction.stdout').read_text()
 assert 'children4_stderr0_frozenSources61' in (directory/'suite.stdout').read_text()
 assert len((directory/'image/role-refusals.txt').read_text().splitlines())==21
 symbols=(directory/'image/program.symbols').read_text()
 assert ' T q38_bf16_activation_producer' in symbols and 'Quantize' not in symbols and 'ProjectionQuantize' not in symbols
for path in (here/'main').rglob('*'):
 if path.is_file() and path.name!='test-argv.json':assert path.read_bytes()==(here/'clean'/path.relative_to(here/'main')).read_bytes(),path.name
print('PASS SAVED local BF16 producer main+clean4 actual children0/stderr0/source61/'+
 '5632 values/64 refusals/independent BF16;HISTORICAL OFFBOARD ONLY,NOT live transport/worker/ROOT/native/model-quality/hardware/TPS authority')
