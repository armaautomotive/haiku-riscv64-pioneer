#!/usr/bin/env python3
"""Saved local result consistency ONLY; no execution or launch authority."""
import hashlib,json,pathlib
here=pathlib.Path(__file__).resolve().parent
m=json.loads((here/'manifest.json').read_text())
assert m['hardware_or_installation'] is False
assert len(m['scoped_sources'])==29
for name,data in m['saved_records'].items():
 p=here/name;assert p.parent==here and p.is_file() and not p.is_symlink()
 b=p.read_bytes();assert len(b)==data['bytes'] and hashlib.sha256(b).hexdigest()==data['sha256'],name
for key in ('actual_exits','final_actual_exits','main_actual_exits','unit_actual_exits'):
 assert m[key] and set(m[key].values())=={0},key
assert m['old109_header_reproduced'] and m['new_header_clean_regenerated']
assert m['main_clean_driver_USER_smoke_byte_identical']
assert m['initial_untimed_USER_superseded'] and m['corrected_worker_requires_startup_record']
assert '20tokens/sec' in m['not_proven'] and 'model quality/speedup' in m['not_proven']
unit=(here/'unit-execute.stdout').read_text()
assert 'positive36/refusals1125/singleBitFaults416' in unit and 'MOCK closure calls' in unit
for label in ('main','clean'):
 geometry=(here/('final-footprint-'+label+'.stdout')).read_text()
 assert 'code3258437 words/snapshot15444538 words=61778152bytes' in geometry
imports=(here/'client-sdk-worker.imports').read_text()
llama={line.split()[-1] for line in imports.splitlines() if line.split() and line.split()[-1].startswith('llama_')}
assert len(llama)==11 and 'llama_decode' not in imports and 'llama_init_from_model' not in imports and 'ggml_backend' not in imports
assert '6/refusals615/' in (here/'main-roles110.stdout').read_text()
print('PASS saved local110 BUILD/UNIT/transport/journal/geometry records;NOT card execution,cleanup authority,model quality or speed')
