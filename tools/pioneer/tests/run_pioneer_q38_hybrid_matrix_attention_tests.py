#!/usr/bin/env python3
"""Run QGW7 interleaving and dispatcher observers. Never access hardware."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 6:
    raise SystemExit('usage: ... SDK QGW7_DIR BFP4_LEAF QGW7_BFP8_TEST_DIR BFP8_LEAF')
sdk, image, leaf4, image8, leaf8 = map(Path, sys.argv[1:])
assert all(path.is_absolute() and path.is_dir() for path in (sdk,image,leaf4,image8,leaf8))
pins = {
 'boot.bin':(4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 'globalworker.bin':(10756,'67d079a1223f841526657a210d5dda2764e8fea1275d65b0fe4b267c93e7e9c5'),
 'bf16producer.bin':(7360,'8b3e477d51eeca467e155b72234bed728c7109947f78cfb4361ce6dbc7afb2c3'),
 'cachedfpu.bin':(12668,'bcc704d361936935e26c57b138693accc5a22207ab5e45019d78baec1385abc9'),
 'dense.bin':(13832,'3be13d88ab953799921b7d84a86b947f296d01d236000572de9f9a54359e44ab'),
 'text.bin':(5984,'2548ed0d4fdb68ce48325cd6f8581bff6bdc5b34a87f3ba217e36e4a37ed6e5c'),
 'affine.bin':(5360,'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
 'ffn.bin':(16456,'c1dc115dadb20b1ee1d18fb04ce61f15a1d1644a523ccfec8fea70dbd5164e45'),
 'service.bin':(9888,'3b3cdbd88d049b3f0c72016d8eccfad7eab3fd6861a5353a50f26d7d4b46795f'),
 'gdn.bin':(35564,'dd982ac614d163795442942529ef9f4e5519552097b438f5fc4464b40eab3436'),
 'program.elf':(156896,'bb13d0710198c39a02e787f9ef0739569d5d3100df6753a5d29f65e45c8a2b3b'),
}
pins8 = dict(pins)
pins8['globalworker.bin']=(10756,'cf4e01c5f3b84ce9706a9dd2be0de22d626adae885469edbfea2279f481a7f2f')
pins8['program.elf']=(156896,'9efca2fddc0703591546c759bdde52bf4ec52fadcddf1da8cff4dceccaaee0bb')
def check(folder, entries):
    for name, (size, digest) in entries.items():
        data=(folder/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest, (folder,name)
check(image,pins);check(image8,pins8)
boot=(4,'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd')
check(leaf4,{'boot.bin':boot,'text.bin':(6928,'04d43a4e5ad5b766fea07d0d3f9f48168557abda4772826d804f643db644faaf')})
check(leaf8,{'boot.bin':boot,'text.bin':(6724,'4cbd9db9777a528cde91cece928ba5aa205adcbdac3435a2571a89259c181b67')})
nm=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
for folder in (image,image8):
    assert (folder/'program.symbols').read_bytes()==subprocess.check_output([str(nm),'-n',str(folder/'program.elf')])
here=Path(__file__).resolve().parent
flags=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
       '-fsanitize=address,undefined','-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
       '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
with tempfile.TemporaryDirectory(prefix='pioneer-qgw7-transition-',dir='/private/tmp') as temporary:
    temporary=Path(temporary)
    for name, source, extra, worker, leaf in (
        ('transition','pioneer_q38_hybrid_matrix_attention_transition_test.cpp',[],image,leaf4),
        ('bfp4','pioneer_q38_hybrid_worker_image_test.cpp',[],image,leaf4),
        ('bfp8','pioneer_q38_hybrid_worker_image_test.cpp',['-DPIONEER_TEST_BFP8=1',
          '-DPIONEER_Q38_HYBRID_TEST_BFP8_LAYER0=1'],image8,leaf8),
    ):
        binary=temporary/name
        subprocess.run([os.environ.get('CXX','c++'),*flags,*extra,str(here/source),'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(worker),str(leaf)],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
