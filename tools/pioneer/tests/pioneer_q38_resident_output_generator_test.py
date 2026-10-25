#!/usr/bin/env python3
"""ALL11 complete code DATA generator/roundtrip/refusals, NEVER card IO."""
import hashlib
import pathlib
import re
import runpy
import shutil
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
here = pathlib.Path(__file__).resolve().parent
generator = here.parent/'generate_qwen38_resident_output_images.py'
module = runpy.run_path(str(generator))
pins, generate = module['PINS'], module['generate']
assert len(pins) == 11
image = pathlib.Path(sys.argv[1])
count = 0
for name, (section, size, digest, suffix) in pins.items():
    path = image if name.startswith('root-') else image/'exact-companion'
    original = (path/(section+'.bin')).read_bytes()
    assert len(original) == size and hashlib.sha256(original).hexdigest() == digest
    patch = generate([str(image), str(image/'exact-companion'), name])
    assert patch.startswith('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutput'+suffix+'.h\n')
    assert patch.endswith('*** End Patch\n') and '// SHA256 '+digest in patch and 'DATA,NOT native/model/source/init/owner authority' in patch
    words = [int(x, 16) for x in re.findall(r'0x([0-9a-f]{8})U', patch)]
    assert struct.pack('<'+'I'*len(words), *words) == original
    count += 1
with tempfile.TemporaryDirectory(prefix='pioneer-output-generator-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)/'root'
    peer = pathlib.Path(temporary)/'peer'
    root.mkdir();peer.mkdir()
    for name, (section, _, _, _) in pins.items():
        shutil.copyfile((image if name.startswith('root-') else image/'exact-companion')/(section+'.bin'),
                        (root if name.startswith('root-') else peer)/(section+'.bin'))
    for name, (section, _, _, _) in pins.items():
        target = (root if name.startswith('root-') else peer)/(section+'.bin')
        original = target.read_bytes()
        # Corruption in ANY other family/section must prevent even a tiny
        # requested boot header; no partially authenticated patch is leaked.
        for changed in (bytes([original[0]^1])+original[1:], original[:-4], original+bytes(4)):
            target.write_bytes(changed)
            result = subprocess.run([sys.executable, str(generator), str(root), str(peer), 'root-boot'], capture_output=True, text=True)
            assert result.returncode != 0 and not result.stdout
            count += 1
        target.write_bytes(original)
    for extra in ([], ['unknown'], ['root-main', 'EXTRA']):
        result = subprocess.run([sys.executable, str(generator), str(root), str(peer), *extra], capture_output=True, text=True)
        assert result.returncode != 0 and not result.stdout
        count += 1
print(f'PASS resident-output code DATA generator{count}/ALL11 exact binary roundtrips/BOTH boots+ALL sections verified BEFORE ANY output/corruption+length+unknown arity refused;NO SDK/readwrite owner/install/launch/model or speed grant')
