#!/usr/bin/env python3
"""Exact QPR1 role isolation/whole images;NO native install/card access."""
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
with tempfile.TemporaryDirectory(prefix='pioneer-parent-gates-') as temporary:
    base = pathlib.Path(temporary)

    def refuse(name, environment, local=False):
        global cases
        sdk, output = base / 'MUST_NOT_OPEN_SDK', base / ('never-' + str(cases))
        args = ['sh', str(directory / name), *(['projection_gather'] if local else []), str(sdk), str(output)]
        if name == 'build_qwen38_inference.sh':
            args.append(str(base / 'MUST_NOT_OPEN_LIBRARIES'))
        result = subprocess.run(args, env={**clean, **environment}, capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, environment, result)
        cases += 1

    for name, role in [('build_qwen38_banks_driver.sh', {}), ('build_qwen38_banks_client.sh', {}),
                       ('build_qwen38_inference.sh', {}),
                       ('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_ROOT': '1'}),
                       ('build_qwen38_ffn_matrix_image.sh', {'PIONEER_Q38_FFN_RESIDENT_MATRIX': '1'}),
                       ('build_qwen38_ffn_persistent_image.sh', {'PIONEER_Q38_FFN_PERSISTENT': '1'}),
                       ('build_qwen38_projection_matrix_image.sh', {}), ('build_qwen38_projection_persistent_image.sh', {}),
                       ('build_qwen38_local_image.sh', {})]:
        for value in ['1', '01', '2', '-1', 'bad']:
            refuse(name, {**role, 'PIONEER_Q38_PROJECTION_GRAPH_ROOT': value}, local=name == 'build_qwen38_local_image.sh')
    for key in ['PIONEER_Q38_PROJECTION_GRAPH_ROOT', 'PIONEER_Q38_PROJECTION_MATRIX_ROOT', 'PIONEER_Q38_PROJECTION_PERSISTENT',
                'PIONEER_Q38_FFN_GRAPH_ROOT', 'PIONEER_Q38_FFN_PERSISTENT', 'PIONEER_Q38_FFN_RESIDENT_MATRIX',
                'PIONEER_Q38_WORK_RAW_UNPACK_CACHE', 'PIONEER_Q38_FFN_SEALED_PROOF', 'PIONEER_DRIVER_BUILD_TAG',
                'PIONEER_UNKNOWN_TEST_SELECTOR']:
        for value in ['1', '01', '2', '-1', 'bad', '']:
            refuse('build_qwen38_projection_graph_image.sh', {key: value})

if len(sys.argv) != 6:
    raise SystemExit('usage: test.py PARENT REPEAT_PARENT QFP3 OLD_QPM OLD_QFG2')
parent, repeat, peer, qpm, qfg = map(pathlib.Path, sys.argv[1:])
boot = 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
parent_pins = {'boot': boot, 'text': 'c623cad56856ac690a79e9b60566148afdba062646bafbb5bf32bcabaa7cb108',
               'prefix': 'cfb512c2b507d511b671bbb73101b724ec230f853a3486616336bacd66706357',
               'graph': '13992bf95da5c695aab493db6d2760d74b139ed7384c04552df0751fa13ceaf9'}
for artifact, pins in [(parent, parent_pins), (repeat, parent_pins),
    (peer, {'boot': boot, 'text': '783bd17e5f1114365bac3e7fa0589f205d70a225a2223a4de7089503ef90a615',
            'affine': 'a55331eeab74b50a50012a62c0d476e9a247f653eefe2211a08d0a75b2da85ca',
            'ffn': 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'}),
    (qpm, {'boot': boot, 'text': 'a28ec7d505a837c260939a9cbbbba9aa98ac4f13069045de8b73f84878221bad',
           'prefix': '5236efe8942629ebf00e8e7bea83b332473a3c180a09990e71b73217fd631017'}),
    (qfg, {'boot': boot, 'text': 'a363fb40ba949badda8ba8c863385484bee9cd7c52e1cf0a7ef587b029abcba9',
           'prefix': '255ff2bb850e0e30a4a0e08dd501a245eb0d021d70302e58c0b5ae014dde8740',
           'graph': 'a0b2fdf23c3e5eead311e79a80989ea113dbe9fcad7c0033a281381d23bdd998'})]:
    for section, digest in pins.items():
        assert hashlib.sha256((artifact / (section + '.bin')).read_bytes()).hexdigest() == digest, (artifact, section)
    symbols = (artifact / 'program.symbols').read_text()
    assert not re.search(r'\b(?:U|[BbDdGgSs])\s', symbols)
    assert not re.search(r'\b(?:memcpy|memset|malloc|free|_GLOBAL_OFFSET_TABLE_)\b', symbols)
for artifact in [parent, repeat]:
    symbols = (artifact / 'program.symbols').read_text()
    for name, pc in [('block_entry', 0x72000), ('block_park', 0x72038), ('block_fault_park', 0x72060)]:
        assert re.search(r'^%08x\s+[Tt]\s+%s$' % (pc, name), symbols, re.M)
    sizes = {'text': 7812, 'prefix': 6508, 'graph': 16192}
    for section, size in sizes.items():
        assert len((artifact / (section + '.bin')).read_bytes()) == size
    assert not re.search(r'\b(?:divu?|remu?|mulh(?:u|su)?)\b', (artifact / 'program.disassembly').read_text())
assert len((peer / 'ffn.bin').read_bytes()) > 12288, 'old12KiB owner NOT a new16KiB grant'
print(f'PASS {cases} before-SDK parent/old-role refusals;complete QPR1 repeat/PC/no-global pins,exact QFP3/old QPM1/QFG2 controls;NO_NATIVE_CATALOG_MAPPING_SOURCE_PRODUCER_OR_SPEED_GRANT')
