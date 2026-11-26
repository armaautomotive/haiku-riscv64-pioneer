#!/usr/bin/env python3
"""Exercise same-input audit failure reporting without Pioneer hardware."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

script = Path(__file__).with_name('pioneer_qwen35_ttsim_boundary_oracle.py')
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    fixture = root / 'fixture'; fixture.mkdir()
    snaps = root / 'snapshots'; snaps.mkdir()
    refs = root / 'references'; refs.mkdir()
    (snaps / 'execution-complete.marker').touch()
    for directory in (snaps, refs):
        for layer in range(2):
            (directory / f'layer{layer}.bin').write_bytes(bytes(2560 * 4))
    command = bytearray(128)
    struct.pack_into('<2I', command, 32, 42, 0)
    (fixture / 'command.bin').write_bytes(command)
    (fixture / 'layout.txt').write_text('C 2 5 6 0\n')
    records = bytearray(120 * 6 * 2048)
    for core in range(120):
        for step, mode in enumerate((2, 2, 1, 2, 2)):
            offset = (core * 6 + step) * 2048
            struct.pack_into('<6I', records, offset, 0, 0, 0, step + 1, core, 0x31535235)
            struct.pack_into('<4I', records, offset + 64, 0x31535235, 1, core, mode)
            struct.pack_into('<8I', records, offset + 64 + 20 * 4,
                             9216, 4, 1, 16, 0, 0x05000000, 0, 9216)
            struct.pack_into('<8I', records, offset + 64 + 52 * 4,
                             2560, 4, 1, 16, 0, 0x06000000, 0, 2560)
    (fixture / 'records.bin').write_bytes(records)
    oracle = root / 'fake-oracle'
    oracle.write_text('#!' + sys.executable + '\n'
        'import os, pathlib, struct, sys\n'
        'out=pathlib.Path(sys.argv[2]); boundary=int(sys.argv[4])\n'
        '(out / ("layer%d.bin" % boundary)).write_bytes(pathlib.Path(sys.argv[5]).read_bytes())\n'
        'data=bytes(2560*4)\n'
        'if os.environ.get("FAKE_MISMATCH"): data=struct.pack("<f",1.0)+data[4:]\n'
        '(out / ("layer%d.bin" % (boundary+1))).write_bytes(data)\n')
    oracle.chmod(0o700)
    def run(name, mismatch=False):
        env = os.environ.copy()
        if mismatch: env['FAKE_MISMATCH'] = '1'
        else: env.pop('FAKE_MISMATCH', None)
        result = subprocess.run([sys.executable, '-B', str(script), str(oracle), 'unused',
                                 str(snaps), str(fixture), str(refs), str(root / name),
                                 '--layers', '2'], capture_output=True, text=True, env=env)
        return result, json.loads((root / name / 'summary.json').read_text())
    passed, good = run('pass')
    assert passed.returncode == 0 and good['passed'] and all(x['passed'] for x in good['checks'])
    failed, bad = run('fail', True)
    assert failed.returncode == 1 and not bad['passed'] and bad['checks'][0]['passed'] and not bad['checks'][1]['passed']
    assert '1/2560' in bad['checks'][1]['detail']
    (snaps / 'execution-complete.marker').unlink()
    missing = subprocess.run([sys.executable, '-B', str(script), str(oracle), 'unused',
                              str(snaps), str(fixture), str(refs), str(root / 'missing'),
                              '--layers', '2'], capture_output=True, text=True)
    assert missing.returncode != 0 and not (root / 'missing').exists()
print('PASS same-input audit reports mismatch and rejects incomplete snapshots; NO HARDWARE')
