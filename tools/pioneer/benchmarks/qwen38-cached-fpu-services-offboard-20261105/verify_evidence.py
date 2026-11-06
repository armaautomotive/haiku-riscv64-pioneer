#!/usr/bin/env python3
"""Replay saved component receipts; never a device/inference admission."""
import hashlib
import json
from pathlib import Path

assert __debug__
here = Path(__file__).resolve().parent
root = here.parents[3]
for line in (here/'evidence-sha256.txt').read_text().splitlines():
    digest, name = line.split('  ', 1)
    path = root/name
    assert path.is_relative_to(root) and hashlib.sha256(path.read_bytes()).hexdigest() == digest, name
expected = {'build', 'contract', 'contract-compile', 'image', 'image-compile', 'fault', 'fault-compile',
            'warm', 'warm-compile', 'original', 'original-compile', 'fraction'}
for folder in ('main', 'clean'):
    directory = here/folder
    exits = json.loads((directory/'actual-exits.json').read_text())
    assert set(exits) == expected and set(exits.values()) == {0}
    for name in exits:
        assert not (directory/(name+'.stderr')).read_bytes()
    lines = (directory/'source-sha256.txt').read_text().splitlines()
    assert len(lines) == 60
    for line in lines:
        digest, name = line.split('  ', 1)
        assert hashlib.sha256((root/name).read_bytes()).hexdigest() == digest, name
    image = (directory/'image.stdout').read_text()
    assert image.count('PASS actual cached-FPU services K') == 6 and '6 cases/704 outputs' in image
    original = (directory/'original.stdout').read_text()
    assert original.count('PASS PRIVATE cached-FPU original sample') == 9 and 'original288 exact outputs' in original
    assert 'fault/refusal cases32' in (directory/'fault.stdout').read_text()
    assert 'requests/416 exact outputs' in (directory/'warm.stdout').read_text()
    assert '128 actual RV32' in (directory/'fraction.stdout').read_text()
    assert 'children12_stderr0_frozenSources60' in (directory/'suite.stdout').read_text()
for path in (here/'main').iterdir():
    if path.name != 'test-argv.json':
        assert path.read_bytes() == (here/'clean'/path.name).read_bytes(), path.name
for role in ('cached_fpu_matrix', 'bf16_producer'):
    lines = (here/(role+'-role-refusals.txt')).read_text().splitlines()
    assert len(lines) == 26 and all(line.startswith('PASS ') for line in lines)
print('PASS SAVED cached-FPU services main+clean12 actual child exits0/stderr0/source60/'+
      '704 synthetic+288 original+416 warm outputs/32 fault cases/independent BF16;'+
      'HISTORICAL OFFBOARD ONLY,NOT current native/hardware/model-quality/TPS authority')
