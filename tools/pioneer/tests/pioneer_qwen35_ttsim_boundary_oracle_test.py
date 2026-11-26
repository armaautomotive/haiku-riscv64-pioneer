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
        'out=pathlib.Path(sys.argv[2]); warm=sys.argv[3]=="two"\n'
        'prefix="position1-" if warm else ""\n'
        'args=sys.argv[4:] if warm else sys.argv[3:]\n'
        'if args[0]=="--replace-warm-state":\n'
        ' layer=int(args[1]); source=pathlib.Path(args[2]); args=args[3:]\n'
        ' for code,name in (("H","conv"),("D","delta")):\n'
        '  data=(source/("%s-layer%d.bin" % (code,layer))).read_bytes()\n'
        '  if os.environ.get("FAKE_BAD_STATE") and code=="H": data=bytes(len(data))\n'
        '  (out/("position1-%s-state-%d.bin" % (name,layer))).write_bytes(data)\n'
        'data=bytes(2560*4)\n'
        'if args:\n'
        ' boundary=int(args[1]); source=args[2]\n'
        ' (out / (prefix+"layer%d.bin" % boundary)).write_bytes(pathlib.Path(source).read_bytes())\n'
        ' if os.environ.get("FAKE_MISMATCH"): data=struct.pack("<f",1.0)+data[4:]\n'
        ' (out / (prefix+"layer%d.bin" % (boundary+1))).write_bytes(data)\n'
        'else: (out / "position1-layer0.bin").write_bytes(data)\n'
        'if warm: (out / "token.txt").write_text("261\\n"); (out / "position1-token.txt").write_text("1850\\n")\n')
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
    (snaps / 'execution-complete.marker').touch()
    modes = [item for _ in range(31) for item in (2, 2, 1)] + [2, 2]
    count = len(modes) + 1
    (fixture / 'layout.txt').write_text(f'C 32 {len(modes)} {count} 1\n')
    records = bytearray(120 * count * 2048)
    for core in range(120):
        for step, mode in enumerate(modes):
            offset = (core * count + step) * 2048
            struct.pack_into('<6I', records, offset, 0, 0, 0, step + 1, core, 0x31535235)
            struct.pack_into('<4I', records, offset + 64, 0x31535235, 1, core, mode)
            struct.pack_into('<8I', records, offset + 64 + 20 * 4,
                             9216, 4, 1, 16, 0, 0x05000000, 0, 9216)
            struct.pack_into('<8I', records, offset + 64 + 52 * 4,
                             2560, 4, 1, 16, 0, 0x06000000, 0, 2560)
    (fixture / 'records.bin').write_bytes(records)
    for layer in range(32):
        (snaps / f'layer{layer}.bin').write_bytes(bytes(2560 * 4))
    (refs / 'position1-layer0.bin').write_bytes(bytes(2560 * 4))
    (refs / 'token.txt').write_text('261\n')
    first_token = root / 'first-token.txt'; first_token.write_text('261\n')
    warm = [sys.executable, '-B', str(script), str(oracle), 'unused', str(snaps),
            str(fixture), str(refs), str(root / 'warm'), '--layers', '32',
            '--actual-token', '1850', '--warm-first-token-file', str(first_token)]
    result = subprocess.run(warm, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    report = json.loads((root / 'warm' / 'summary.json').read_text())
    assert report['passed'] and report['position'] == 1 and report['first_token'] == 261
    assert report['token_matches'] and len(report['checks']) == 32
    assert 'prior simulator state is not injected' in report['scope']
    env = os.environ.copy(); env['FAKE_MISMATCH'] = '1'
    warm[8] = str(root / 'warm-mismatch')
    result = subprocess.run(warm, capture_output=True, text=True, env=env)
    report = json.loads((root / 'warm-mismatch' / 'summary.json').read_text())
    assert result.returncode == 1 and not report['passed']
    assert not report['checks'][1]['passed'] and report['token_matches']
    state = root / 'state'; state.mkdir()
    (state / 'worker-audit-complete.marker').touch()
    for layer in (0, 1):
        (state / f'H-layer{layer}.bin').write_bytes(struct.pack('<f',1.0) + bytes(24575 * 4))
        (state / f'D-layer{layer}.bin').write_bytes(bytes(524288 * 4))
    warm[8] = str(root / 'warm-state')
    warm.extend(['--warm-recurrent-state', str(state)])
    result = subprocess.run(warm, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    report = json.loads((root / 'warm-state' / 'summary.json').read_text())
    assert report['passed'] and set(report['recurrent_state_sha256']) == {'0', '1'}
    assert [c['prior_state'] for c in report['checks'][:3]] == [
        'supplied simulator recurrent state', 'supplied simulator recurrent state', 'CPU prior state']
    assert 'caller must verify state provenance' in report['scope']
    warm[8] = str(root / 'warm-bad-state')
    env = os.environ.copy(); env['FAKE_BAD_STATE'] = '1'
    result = subprocess.run(warm, capture_output=True, text=True, env=env)
    assert result.returncode != 0 and 'did not retain exact conv state' in result.stderr
    (state / 'D-layer1.bin').unlink()
    warm[8] = str(root / 'warm-incomplete-state')
    result = subprocess.run(warm, capture_output=True, text=True)
    assert result.returncode != 0 and not (root / 'warm-incomplete-state').exists()
    warm = warm[:-2]
    first_token.write_text('42\n')
    warm[8] = str(root / 'wrong-first-token')
    result = subprocess.run(warm, capture_output=True, text=True)
    assert result.returncode != 0 and not (root / 'wrong-first-token').exists()
print('PASS same-input audit reports mismatch and rejects incomplete snapshots; NO HARDWARE')
