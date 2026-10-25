#!/usr/bin/env python3
"""Same ACTUAL ROOT/119-peer VM for separately pinned -O2/-O3 images."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) in (4, 5) and sys.argv[3] in ('--O2', '--O3', '--O3-backoff')
assert len(sys.argv) == 4 or sys.argv[4] in ('--smoke', '--positives', '--stalled')
here = pathlib.Path(__file__).resolve().parent
image, worker = map(pathlib.Path, sys.argv[1:3])
variants = {
    '--O2': (
        ('text', 92, '248a577375be39d27d0b1e2e3baab1a6f3132444479381b9eb6c5153d57b7dd7'),
        ('conv', 6472, 'd0cb7585d0426084672077904ec9b15d46a273fdb6e07fa57c560ad13b07a0b7'),
        ('cohort', 18064, '6ff91721b371c85ffa33ae3d7f342e88355fdff5311938f57ec89b3c6d3ac4d5'),
    ),
    '--O3': (
        ('text', 92, 'a51e604ed1dc3252cb09ffa1d641e4cdee8ca09beccf8ddf1c124aef471a80ca'),
        ('conv', 6472, '9c19f8476ab15d6f09c0d1a07c3b2b73ad70c58bbdb54f042399f75c9d795d27'),
        ('cohort', 36808, '84208d5e9b9a93b2b73d3d7007deb17856987f8693afe79bfaa9daa3596b704c'),
    ),
    '--O3-backoff': (
        ('text', 92, '874faf837e78bacfb006eba25bb85bec23ac3ea284a7f0f8972fd509ae12b974'),
        ('conv', 6472, '19d7092a22366e405987251b2b25447083d6a34022d271398a9201ae5c265498'),
        ('cohort', 36880, '0c8dc8357c0381167d063e7e8de03afcebcff33dc3aa6699ccfac6268e1d056b'),
    ),
}
assert hashlib.sha256((image/'boot.bin').read_bytes()).hexdigest() == 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
for section, size, digest in variants[sys.argv[3]]:
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', '47ff39e8f9709f9e87469e44919b374ccfbea3edd3f1f1f1b3020ecfd545c30b'),
    ('affine', 'd8734c6a6e1cea65e8abac370292033f8be1406fa063c0f84501646aaaedca5c'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', '3498f0507019f44b4c76101ca8df2df84dc4d57e3a288703c437fb441fa0eb43'),
    ('gdn', '67f202ea9afbf902488af532418c876c8983e146b21d272b1cbf88108b412648'),
):
    assert hashlib.sha256((worker/(section+'.bin')).read_bytes()).hexdigest() == digest
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-cohort-optimized-test-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    str(here/'pioneer_q38_resident_cohort_image_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(image), str(worker), *sys.argv[4:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
