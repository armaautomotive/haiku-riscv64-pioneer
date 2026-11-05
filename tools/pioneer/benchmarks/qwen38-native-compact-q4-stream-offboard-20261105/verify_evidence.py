#!/usr/bin/env python3
"""Verify frozen OFFBOARD receipts/source;optional private binary recheck.NO device IO."""
import hashlib
import json
import pathlib
import re
import sys

assert __debug__ and len(sys.argv) in (1, 2)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[3]
manifest = json.loads((here / 'manifest.json').read_text())
assert manifest['version'] == 1 and len(manifest['files']) == 48

def digest(data):
    return hashlib.sha256(data).hexdigest()

def source_pins(tree):
    lines = (here / tree / 'source-sha256.txt').read_text().splitlines()
    assert all(re.fullmatch(r'[0-9a-f]{64}  [^\n]+', line) for line in lines)
    pins = {line[66:]: line[:64] for line in lines}
    assert len(pins) == len(lines)
    return pins

for name, pin in manifest['files'].items():
    path = pathlib.PurePosixPath(name)
    assert not path.is_absolute() and '..' not in path.parts
    data = (here / path).read_bytes()
    assert len(data) == pin['bytes'] and digest(data) == pin['sha256'], name
    if name.endswith('.stderr'):
        assert not data, name

labels = ('sdk', 'contract-compile', 'contract', 'lease-compile', 'lease',
          'real_lease-compile', 'real_lease', 'native')
for tree, session in (('main', 49969), ('clean', 71892)):
    terminal = manifest['terminal'][tree]
    assert terminal['session_id'] == session and terminal['exit'] == 0
    exits = (here / tree / 'actual-child-exits.txt').read_text().splitlines()
    assert exits == [name + ' 0' for name in labels]
    lines = (here / tree / 'outer.log').read_text().splitlines()
    assert len(lines) == 9 and lines[-1].startswith('PASS OFFBOARD diagnostic104 ')
    for label, line in zip(labels, lines[:-1]):
        expected = digest((here / tree / (label + '.log')).read_bytes())
        assert line == f'PASS_CLOSED {label} exit0 stdout_sha{expected} stderr0'
for label in labels[:-1]:
    assert (here / 'main' / (label + '.log')).read_bytes() == (here / 'clean' / (label + '.log')).read_bytes()
a, b = [(here / tree / 'native.log').read_text().replace(manifest['terminal'][tree]['qualification_path'], '<QUALIFICATION>') for tree in ('main', 'clean')]
assert a == b and 'valid317 refused679' in a

main, clean = map(source_pins, ('main', 'clean'))
assert (len(main), len(clean)) == (898, 895)
assert not set(clean) - set(main)
assert sorted(set(main) - set(clean)) == manifest['preexisting_main_only']
different = sorted(name for name in clean if clean[name] != main[name])
assert different == manifest['preexisting_differences'] and len(different) == 6
assert len(manifest['preexisting_main_only']) == 3
for name, pin in clean.items():
    path = pathlib.PurePosixPath(name)
    assert not path.is_absolute() and '..' not in path.parts
    current = digest((root / path).read_bytes())
    allowed = {pin, main[name]} if name in different else {pin}
    assert current in allowed, name
for name in manifest['preexisting_main_only']:
    path = root / name
    if path.exists():
        assert digest(path.read_bytes()) == main[name], name
assert len(manifest['new_sources']) == 14
for name, pin in manifest['new_sources'].items():
    assert main[name] == clean[name] == pin == digest((root / name).read_bytes()), name
assert manifest['matched_private_artifacts']['tenstorrent_pci_probe-default43-control']['sha256'] == '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
assert manifest['matched_private_artifacts']['tenstorrent_pci_probe-native-q4-fpu-stream104']['sha256'] == '4722e956bfda10a5f8dbc2317bd5dd12d2de1b1099c3d33286b4b6447930f61e'

if len(sys.argv) == 2:
    private = pathlib.Path(sys.argv[1])
    assert private.is_absolute() and len(manifest['matched_private_artifacts']) == 12
    for name, pin in manifest['matched_private_artifacts'].items():
        for tree in ('main', 'clean-b'):
            data = (private / tree / 'build' / name).read_bytes()
            assert len(data) == pin['bytes'] and digest(data) == pin['sha256'], (tree, name)
    for name, pin in (('PioneerNativeCompactQ4StreamData.h', manifest['private_data_sha256']),
                      ('PioneerNativeCompactQ4StreamMetadata.h', manifest['private_prediction_sha256'])):
        assert digest((private / 'export' / name).read_bytes()) == pin
    print('PASS optional private recheck:all12 artifacts BOTH trees/unchanged private data+predictions;NO BYTES PRINTED')
print('PASS frozen native104 OFFBOARD:48 exact receipts/eight actual0 children per tree/14 new source pins/898+895 lexical closures with explicit unrelated differences;NOT hardware/full-model-quality/speed proof')
