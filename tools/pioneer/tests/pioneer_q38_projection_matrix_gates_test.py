#!/usr/bin/env python3
"""Before-SDK isolated QPM1 role and COMPLETE new/legacy image controls."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
cases = 0
with tempfile.TemporaryDirectory(prefix='pioneer-projection-matrix-gates-') as temporary:
    base = pathlib.Path(temporary)

    def refuse(name, environment, extra=()):
        global cases
        sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('never-' + str(cases))
        args = ['sh', str(directory / name), str(sdk), str(output), *extra]
        if name == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env={**clean, **environment}, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, environment, result)
        cases += 1

    for name, owner in [
        ('build_qwen38_banks_driver.sh', {}), ('build_qwen38_banks_client.sh', {}),
        ('build_qwen38_inference.sh', {}),
        ('build_qwen38_ffn_matrix_image.sh', {'PIONEER_Q38_FFN_RESIDENT_MATRIX': '1'}),
        ('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_ROOT': '1'}),
        ('build_qwen38_ffn_persistent_image.sh', {'PIONEER_Q38_FFN_PERSISTENT': '1'}),
    ]:
        for value in ['1', '01', '2', '-1', 'bad']:
            refuse(name, {**owner, 'PIONEER_Q38_PROJECTION_MATRIX_ROOT': value})
    for key in ['PIONEER_Q38_PROJECTION_MATRIX_ROOT', 'PIONEER_Q38_PROJECTION_PERSISTENT',
                'PIONEER_Q38_FFN_RESIDENT_MATRIX', 'PIONEER_Q38_FFN_GRAPH_ROOT',
                'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT', 'PIONEER_Q38_FFN_PERSISTENT',
                'PIONEER_Q38_OUTPUT_TRANSFER_ROOT', 'PIONEER_Q38_WORK_RAW_UNPACK_CACHE',
                'PIONEER_Q38_FFN_SEALED_PROOF', 'PIONEER_DRIVER_BUILD_TAG', 'PIONEER_UNKNOWN_TEST_SELECTOR']:
        for value in ['1', '01', '2', '-1', 'bad', '']:
            refuse('build_qwen38_projection_matrix_image.sh', {key: value})

if len(sys.argv) != 6:
    raise SystemExit('usage:test.py ROOT REPEAT_ROOT OLD_QFM OLD_QFG2 QFP3')
root, repeat, old_matrix, old_graph, peer = map(pathlib.Path, sys.argv[1:])
boot = 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
pins = [
    (root, {'boot': boot, 'text': 'a28ec7d505a837c260939a9cbbbba9aa98ac4f13069045de8b73f84878221bad',
            'prefix': '5236efe8942629ebf00e8e7bea83b332473a3c180a09990e71b73217fd631017'}),
    (repeat, {'boot': boot, 'text': 'a28ec7d505a837c260939a9cbbbba9aa98ac4f13069045de8b73f84878221bad',
              'prefix': '5236efe8942629ebf00e8e7bea83b332473a3c180a09990e71b73217fd631017'}),
    (old_matrix, {'boot': boot, 'text': '67ff940a48f37219114de10135c010e1e88bdcd90e608758b538a44e58b09dbe',
                  'prefix': '492dfc44e80d85af1de9e74fee56b68d5215d386fd31d591557850759606b6d8'}),
    (old_graph, {'boot': boot, 'text': 'a363fb40ba949badda8ba8c863385484bee9cd7c52e1cf0a7ef587b029abcba9',
                 'prefix': '255ff2bb850e0e30a4a0e08dd501a245eb0d021d70302e58c0b5ae014dde8740',
                 'graph': 'a0b2fdf23c3e5eead311e79a80989ea113dbe9fcad7c0033a281381d23bdd998'}),
    (peer, {'boot': boot, 'text': '783bd17e5f1114365bac3e7fa0589f205d70a225a2223a4de7089503ef90a615',
            'affine': 'a55331eeab74b50a50012a62c0d476e9a247f653eefe2211a08d0a75b2da85ca',
            'ffn': 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'}),
]
for artifact, sections in pins:
    for section, digest in sections.items():
        data = (artifact / (section + '.bin')).read_bytes()
        assert data and not len(data) % 4 and hashlib.sha256(data).hexdigest() == digest, (artifact, section)
    symbols = (artifact / 'program.symbols').read_text()
    assert not re.search(r'\b(?:U|[BbDdGgSs])\s', symbols), 'unresolved/writable state'
    assert not re.search(r'\b(?:memcpy|memset|malloc|free|_GLOBAL_OFFSET_TABLE_)\b', symbols)
for artifact in [root, repeat]:
    assert len((artifact / 'text.bin').read_bytes()) == 7808 and len((artifact / 'prefix.bin').read_bytes()) == 6412
    symbols = (artifact / 'program.symbols').read_text()
    for name, pc in [('block_entry', 0x72000), ('block_park', 0x72038), ('block_fault_park', 0x72060)]:
        assert re.search(r'^%08x\s+[Tt]\s+%s$' % (pc, name), symbols, re.M)
    assert not re.search(r'\b(?:divu?|remu?|mulh(?:u|su)?)\b', (artifact / 'program.disassembly').read_text())
    image_map = (artifact / 'program.map').read_text()
    for name, address, size in [('boot', 0x6000, 4), ('text', 0x72000, 7808), ('prefix', 0x64000, 6412)]:
        assert re.search(r'^\.%s\s+0x0*%x\s+0x%x\s*$' % (name, address, size), image_map, re.M)
# The code's physical PC checks must match the EXACT full QFP3 companion,
# not the shared four-byte boot jump or an old smaller12KiB helper owner.
source = (directory / 'blackhole_qwen38_dispatch.cpp').read_text()
for section, start in [('ffn', 0x64000), ('affine', 0x70b00), ('text', 0x72000)]:
    end = start + len((peer / (section + '.bin')).read_bytes())
    assert 'value<0x%x' % end in source
assert len((peer / 'ffn.bin').read_bytes()) > 12288
assert (root / 'boot.bin').read_bytes() == (old_matrix / 'boot.bin').read_bytes()
print(f'PASS {cases} before-SDK role refusals;complete QPM1/repeat/PC/no-global pins,exact QFP3 companion extent,old QFM1/QFG2 byte controls;NO native catalog/16KiB owner/source/producer or model-speed grant')
