#!/usr/bin/env python3
"""Offline role isolation and exact complete QFP3 artifacts;NO device access."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-projection-gates-') as temporary:
    base = pathlib.Path(temporary)

    def refuse(name, env, extra=()):
        global cases
        sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('never-' + str(cases))
        args = ['sh', str(directory / name), str(sdk), str(output), *extra]
        if name == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env={**clean, **env}, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, env, result)
        cases += 1

    for value in ['1', '01', '2', '-1', 'bad']:
        for name in ['build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh',
                     'build_qwen38_inference.sh', 'build_qwen38_ffn_persistent_image.sh']:
            refuse(name, {'PIONEER_Q38_PROJECTION_PERSISTENT': value,
                          'PIONEER_Q38_FFN_PERSISTENT': '1'})
        refuse('build_qwen38_ffn_persistent_image.sh',
               {'PIONEER_Q38_PROJECTION_PERSISTENT': value, 'PIONEER_Q38_FFN_PERSISTENT': '1'},
               ['--projection-input'])
    # The separate wrapper owns every selector,not just this new feature.
    for key in ['PIONEER_Q38_PROJECTION_PERSISTENT', 'PIONEER_Q38_FFN_PERSISTENT',
                'PIONEER_Q38_FFN_GRAPH_ROOT', 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT',
                'PIONEER_Q38_WORK_RAW_UNPACK_CACHE', 'PIONEER_Q38_FFN_SEALED_PROOF',
                'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_UNKNOWN_TEST_SELECTOR']:
        for value in ['1', '01', '2', '-1', 'bad', '']:
            refuse('build_qwen38_projection_persistent_image.sh', {key: value})
    for argument in ['--projection-input=1', '--projection', 'bad', '']:
        refuse('build_qwen38_ffn_persistent_image.sh', {'PIONEER_Q38_FFN_PERSISTENT': '1'}, [argument])

if len(sys.argv) != 8:
    raise SystemExit('usage: test.py NEW_PEER REPEAT_PEER OLD_PEER LOCAL_INPUT REPEAT_INPUT GATHER REPEAT_GATHER')
peer, repeat, old, local, local_repeat, gather, gather_repeat = map(pathlib.Path, sys.argv[1:])
expected = {
    'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
    'text': '783bd17e5f1114365bac3e7fa0589f205d70a225a2223a4de7089503ef90a615',
    'affine': 'a55331eeab74b50a50012a62c0d476e9a247f653eefe2211a08d0a75b2da85ca',
    'ffn': 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9',
}
old_expected = {
    'boot': expected['boot'],
    'text': 'd2052fbbf43dcc022747ea2b228e7b1bc0f829f4984821ab4dfca20473e3a8ef',
    'affine': '96e92065722a477b08ba41abc2f623064d44970c5807b37c6d739e8dc3697570',
    'ffn': 'd7c6f3458d795f65635b80c3c035c93a7da5b325629f19e0b0fdecdc70fc629d',
}
for artifact, hashes in [(peer, expected), (repeat, expected), (old, old_expected)]:
    for name, digest in hashes.items():
        assert hashlib.sha256((artifact / (name + '.bin')).read_bytes()).hexdigest() == digest, (artifact, name)
    symbols = (artifact / 'program.symbols').read_text()
    assert not re.search(r'\b(?:U|[BbDdGgSs])\s', symbols), 'unresolved/writable state'
    assert not re.search(r'\b(?:memcpy|memset|malloc|free|__.*(?:div|mod|float|fix).*|_GLOBAL_OFFSET_TABLE_)\b', symbols)
    for name, pc in [('block_entry', 0x72000), ('block_peer_wait', 0x72040),
                     ('block_peer_stopped', 0x72060), ('block_fault_park', 0x72080)]:
        assert re.search(r'^%08x\s+[Tt]\s+%s$' % (pc, name), symbols, re.M), (artifact, name)
    if artifact != old:
        image_map = (artifact / 'program.map').read_text()
        for name, address, size in [('boot', 0x6000, 4), ('ffn', 0x64000, 0x3fe0),
                                    ('affine', 0x70b00, 0x1480), ('text', 0x72000, 0x1fe4)]:
            assert re.search(r'^\.%s\s+0x0*%x\s+0x%x\s*$' % (name, address, size), image_map, re.M)
            assert len((artifact / (name + '.bin')).read_bytes()) == size
        assert re.search(r'^\.fetch\s+0x0*70000\s+0x0\s*$', image_map, re.M)
        # Cold read-only word tables lie in existing affine ROM,not live
        # 70000..70b00 scratch or the unowned 67000..68000 helper suffix.
        for address, name in re.findall(r'^([0-9a-f]+) [Tt] (\S+)$', symbols, re.M):
            if name.endswith(('E5delta', 'E5field', 'E4size', 'E5local', 'E5route')):
                assert 0x70b00 <= int(address, 16) < 0x72000
        assert 'q38_projection_persistent_input' in symbols
    else:
        assert 'q38_projection_persistent_input' not in symbols

for artifact in [local, local_repeat]:
    for name, digest in [('code', '9538d7e293cde56af2a7d9a0bbb13503b3088c0b72ebb7f4a70481c26a37db56'),
                         ('boot', 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6')]:
        assert hashlib.sha256((artifact / (name + '.bin')).read_bytes()).hexdigest() == digest
assert len((peer / 'ffn.bin').read_bytes()) > 12288, 'NEW owner/catalog16KiB required;old12KiB NOT a grant'
assert (peer / 'boot.bin').read_bytes() == (old / 'boot.bin').read_bytes(), 'shared marker/jump is intentionally NOT authority'
for artifact in [gather, gather_repeat]:
    for name, digest in [('code', 'e4e811535e7a0c6f55f45238625d5f1bb2aada7b5a454ce0bc5f03b52fa17ee8'),
                         ('boot', 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6')]:
        assert hashlib.sha256((artifact / (name + '.bin')).read_bytes()).hexdigest() == digest
    symbols = (artifact / 'program.symbols').read_text()
    assert not re.search(r'\b(?:U|[BbDdGgSs])\s', symbols)
    assert not re.search(r'\b(?:memcpy|memset|malloc|free|_GLOBAL_OFFSET_TABLE_)\b', symbols)
    for name, pc in [('scalar_entry', 0x64000), ('scalar_done', 0x6400c),
                     ('scalar_park', 0x64038), ('scalar_fault_park', 0x6403c)]:
        assert re.search(r'^%08x\s+[Tt]\s+%s$' % (pc, name), symbols, re.M)
    assert len((artifact / 'code.bin').read_bytes()) <= 0x4000
print(f'PASS {cases} before-SDK role refusals;exact QFP3 four-section/repeat/PC/no-global pins,old QFP2/local-input/new ROOT-gather controls;NO native12KiB grant or model-speed evidence')
