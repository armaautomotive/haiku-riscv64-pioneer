#!/usr/bin/env python3
"""Replay bounded saved offboard receipts;never live device admission."""
import hashlib
import json
from pathlib import Path

assert __debug__
here = Path(__file__).resolve().parent
root = here.parents[3]
expected = {
    '': {'root-build', 'bf16-roles', 'old-precise-roles', 'worker-suite'},
    'worker-suite/': {'build', 'producer', 'cached-regression', 'fpu-compile', 'fpu', 'connected-compile', 'connected'},
    'worker-suite/producer-tests/': {'bridge-compile', 'shape-compile', 'worker-compile', 'worker', 'fraction'},
    'worker-suite/cached-regression/': {'build', 'contract-compile', 'contract', 'image-compile', 'image',
                                     'fault-compile', 'fault', 'warm-compile', 'warm', 'original-compile', 'original', 'fraction'},
}
for side in ('main', 'clean'):
    directory = here/side
    for prefix, names in expected.items():
        exits = json.loads((directory/prefix/'actual-exits.json').read_text())
        assert set(exits) == names and set(exits.values()) == {0}
        for name in names:
            assert not (directory/prefix/(name+'.stderr')).read_bytes()
    for prefix, count in (('', 273), ('worker-suite/', 192), ('worker-suite/cached-regression/', 61)):
        rows = (directory/prefix/'source-sha256.txt').read_text().splitlines()
        assert len(rows) == count
        for row in rows:
            digest, name = row.split('  ', 1)
            assert hashlib.sha256((root/name).read_bytes()).hexdigest() == digest, name
    assert 'children4_stderr0_source273' in (directory/'suite.stdout').read_text()
    assert 'positive11/refusals305' in (directory/'bf16-roles.stdout').read_text()
    assert 'positive9/refusals261' in (directory/'old-precise-roles.stdout').read_text()
    assert 'jobs108/packedWords13824/faults18/maxCombinedStack3712' in (directory/'worker-suite/producer-tests/worker.stdout').read_text()
    fpu = (directory/'worker-suite/fpu.stdout').read_text()
    assert fpu.count('PASS ACTUAL full QGW4->linked returning FPU') == 5
    assert 'jobs5/outputs608/maxCombinedStack2752/canonical stripe metadata jobs22848/receiptRefusals10280' in fpu
    assert 'metadata192 matrices/jobs22848/refusals2304' in fpu
    connected = (directory/'worker-suite/connected.stdout').read_text()
    assert 'producers68/packets272/17408 BF16 values' in connected and 'rows64/exact' in connected
    assert 'maxCombinedStack3712' in connected and 'NOT executed ROOT/all119/full-model/native/hardware/TPS proof' in connected
    cached = directory/'worker-suite/cached-regression'
    for name, marker in (('original', 'original288 exact outputs'), ('image', '6 cases/704 outputs'),
                         ('warm', 'requests/416 exact outputs'), ('fault', 'fault/refusal cases32')):
        assert marker in (cached/(name+'.stdout')).read_text()
    assert not (directory/'old-root.stderr').read_bytes()
    assert '1c5fb71798d0199a1e4a8599087220582fc181bd481df84cda577764bf05f32b' in (directory/'old-root.stdout').read_text()
for name in ('artifact-sha256.json', 'source-sha256.txt', 'worker-suite/fpu.stdout', 'worker-suite/connected.stdout',
             'worker-suite/producer-tests/worker.stdout', 'bf16-roles.stdout', 'old-precise-roles.stdout'):
    assert (here/'main'/name).read_bytes() == (here/'clean'/name).read_bytes(), name
artifacts = json.loads((here/'main/artifact-sha256.json').read_text())
assert artifacts['root/program.elf'] == [208164, '44242fc00355a7f7a4421e752cc24875de66fca60a4e569365a8f50fc33edcb1']
assert artifacts['root/bf16.bin'] == [28724, '54718da9208f2c3702b89879f088ff4a2a313150b5ac57a32479124c1fe0d061']
print('PASS SAVED main+clean ROOT BUILD/worker4+7+5+12 children0,273 frozen sources;'
      '68 actual gated producers->same-worker fullK FPU/64 exact outputs,10280 receipt refusals;'
      'NOT executed whole ROOT/native/physical/full-model quality/TPS proof')
