#!/usr/bin/env python3
"""Reproduce committed QPS2 code; every root/peer identity fails atomically."""
import pathlib
import shutil
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
assert len(sys.argv) == 3
root_image, peer_image = map(pathlib.Path, sys.argv[1:])
generator = here.parent/'generate_qwen38_next_projection_images.py'
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
positives = refusals = 0
for section, suffix in (('root-main', 'RootMain'), ('root-prefix', 'RootPrefix'), ('root-graph', 'RootGraph')):
    result = subprocess.run([sys.executable, str(generator), str(root_image), str(peer_image), section], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0 and not result.stderr
    lines = result.stdout.splitlines()
    name = 'PioneerQ38NextProjectionGraph'+suffix+'.h'
    assert lines[0] == '*** Begin Patch' and lines[1] == '*** Add File: src/add-ons/kernel/drivers/misc/'+name and lines[-1] == '*** End Patch'
    assert all(line.startswith('+') for line in lines[2:-1])
    content = '\n'.join(line[1:] for line in lines[2:-1])+'\n'
    assert content == (kernel/name).read_text(), name
    positives += 1
with tempfile.TemporaryDirectory(prefix='pioneer-qps2-generator-') as temporary:
    root = pathlib.Path(temporary)
    a, b = root/'root', root/'peer'
    a.mkdir();b.mkdir()
    files = []
    for source, destination, sections in ((root_image, a, ('boot', 'text', 'prefix', 'graph')), (peer_image, b, ('boot', 'text', 'affine', 'ffn'))):
        for section in sections:
            target = destination/(section+'.bin');shutil.copyfile(source/target.name, target);files.append(target)
    for path in files:
        original = path.read_bytes()
        for corrupt in (original[:-1], bytes([original[0]^1])+original[1:]):
            path.write_bytes(corrupt)
            result = subprocess.run([sys.executable, str(generator), str(a), str(b), 'root-main'], capture_output=True, text=True, timeout=10)
            assert result.returncode != 0 and not result.stdout, (path, result.stdout)
            refusals += 1
        path.write_bytes(original)
    for invalid in ('peer-main', 'peer-affine', 'peer-helper', 'root', ''):
        result = subprocess.run([sys.executable, str(generator), str(a), str(b), invalid], capture_output=True, text=True, timeout=10)
        assert result.returncode != 0 and not result.stdout
        refusals += 1
print(f'PASS QPS2 pinned generator committed ROOT sections={positives}/atomic refusals={refusals};BOTH boots+ALL6 root/unchanged-peer sections authenticated BEFORE output;NO_SDK_DEVICE_OR_ROLE')
