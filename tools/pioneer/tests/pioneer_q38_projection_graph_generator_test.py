#!/usr/bin/env python3
"""Whole-family frozen DATA is refused before ANY patch on ANY bad section."""
import importlib.util
import pathlib
import shutil
import subprocess
import sys
import tempfile

directory = pathlib.Path(__file__).resolve().parent.parent
stashes = sys.argv[-1:] == ['--result-stashes']
if stashes:
    sys.argv.pop()
generator = directory / ('generate_qwen38_projection_stash_images.py' if stashes else 'generate_qwen38_projection_graph_images.py')
spec = importlib.util.spec_from_file_location('qpr_generator', generator)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
if len(sys.argv) != 3:
    raise SystemExit('expected ROOT PEER')
root, peer = map(pathlib.Path, sys.argv[1:])
cases = 0
for name, (_, _, _, suffix) in module.PINS.items():
    result = subprocess.run([sys.executable, str(generator), str(root), str(peer), name], capture_output=True, text=True, timeout=5)
    assert result.returncode == 0 and not result.stderr and result.stdout.endswith('*** End Patch\n')
    lines = result.stdout.splitlines()
    assert lines[0] == '*** Begin Patch' and lines[1] == '*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraph' + suffix + '.h'
    assert all(line.startswith('+') for line in lines[2:-1])
    body = ''.join(line[1:] + '\n' for line in lines[2:-1])
    saved = directory.parent.parent / 'src/add-ons/kernel/drivers/misc' / ('PioneerQ38ProjectionGraph' + suffix + '.h')
    assert body == saved.read_text()
    cases += 1
with tempfile.TemporaryDirectory(prefix='pioneer-projection-generator-') as temporary:
    r, p = pathlib.Path(temporary) / 'root', pathlib.Path(temporary) / 'peer'
    r.mkdir()
    p.mkdir()
    files = [(r, 'boot'), (p, 'boot')]
    for name, (section, _, _, _) in module.PINS.items():
        files.append((r if name.startswith('root-') else p, section))
    for dest, section in files:
        shutil.copyfile((root if dest == r else peer) / (section + '.bin'), dest / (section + '.bin'))
    for dest, section in files:
        path = dest / (section + '.bin')
        original = path.read_bytes()
        for mutation in ('first', 'last', 'short', 'long', 'missing'):
            bad = bytearray(original)
            if mutation == 'first':
                bad[0] ^= 1
            elif mutation == 'last':
                bad[-1] ^= 1
            elif mutation == 'short':
                bad = bad[:-4]
            elif mutation == 'long':
                bad += b'\0\0\0\0'
            if mutation == 'missing':
                path.unlink()
            else:
                path.write_bytes(bad)
            for selected in module.PINS:
                result = subprocess.run([sys.executable, str(generator), str(r), str(p), selected], capture_output=True, text=True, timeout=5)
                assert result.returncode != 0 and not result.stdout, (dest, section, mutation, selected)
                cases += 1
            path.write_bytes(original)
for args in ([], [str(root)], [str(root), str(peer)], [str(root), str(peer), 'old'], [str(root), str(peer), 'root-main', 'extra']):
    result = subprocess.run([sys.executable, str(generator), *args], capture_output=True, text=True, timeout=5)
    assert result.returncode != 0 and not result.stdout
    cases += 1
print(f'PASS complete projection DATA generator cases{cases}:six exact headers;BOTH boots/ALL sections first+last+short+long+missing refuse BEFORE ANY patch;NO_WRITES_NATIVE_OR_SPEED')
