#!/usr/bin/env python3
"""Execute linked ROOT receipt against emitted worker in simulation only."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv)!=5 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute QGWA worker, leaf, QGWA root and old root required')
worker,leaf,root,old=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent
pins={
 worker:{'program.elf':'e0d7bb5a1434181a8560867f1bfee701a7787a878dff780bce215adf79f2fc47',
  'program.symbols':'6952522a83486c07ad7c301d3d60050de334e2b3667c94d2aa3d2432a5b7a22c',
  'boot.bin':'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c',
  'globalworker.bin':'894747f7bf2a282438932a7dc3b56003d9ec64abcda30914b0fd891b70034207',
  'bf16producer.bin':'c32a3790c4e5c1387d2a1c6ac2798514d923f7529b2ab073049b02e78d26f8f1',
  'dense.bin':'1299424f4919a4fd742deef56be3bcbaddb4bdddfece4b00e810884f24c86d45',
  'cachedfpu.bin':'46b4b2e86bc7b09383a3945ba5241aa8745d6939c0868d73636fe71de0913753',
  'ffn.bin':'ef59bdf258b33c35b7a2571182564a1245b52a8dc541e4e0c99399e220e8c6fe',
  'service.bin':'c4284f945050469db9671d1070b789ee9004ffa04bb4b025a00ffd7f0d72c220',
  'affine.bin':'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78',
  'text.bin':'262c677556d6b747c8eb1a3fc6a96c34e4c616af8b6c31be8ca2092baa0961d8',
  'gdn.bin':'35a5dc8cf1c964098a98e953097c283b1a46c81451da454b64fa39af83a9a1e5'},
 leaf:{'boot.bin':'d47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd',
       'text.bin':'77d87adaf6ab3f9c34fc9f1c50c627d6174676b700f24bca34c8a28f0bcd1dd0'},
 root:{'program.elf':'99a0dc86b01efa012323b8602668e74b757547927a60a1a4b8808df76a9c07de',
       'program.symbols':'a6970f3a89abb967a67ff8f02642ab66ed1807836762d979349d1ca5f3e5f185',
       'text.bin':'9645df311faf71b9a34f257b36740785a2c36934752b273dc56c6fd830db203f',
       'bf16.bin':'e42c522785f379490f85c028894b12b75064a25eff8a7ab3b1277c888d987fe2',
       'conv.bin':'ff367c1ab96337b40216b961f67231f0fab60d95ae01acc924564547b955574f',
       'terminal.bin':'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'},
 old:{'program.elf':'9f89d736b357b302edb87be9ed1a4cacd1de9a71b6bc4ee2fc3b6c0e030398c9',
      'program.symbols':'bd9fe205885c8ff5dec2a46a8cc07c001c2ab035bc172556eccd396a13b11ec6',
      'text.bin':'b0f5d73099a5efedb7238d43fc1cfd6decd7741502ae93a3e65126a3329a6101',
      'bf16.bin':'727622e6d4e029c9dfd5e42c03f4f5ee073198af91f102a5516f4373cb70b988',
      'conv.bin':'29953eb1717f17395a4280898aa51753954ea09093a752ea122f4a364bd29b7b',
      'terminal.bin':'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'}}
if len(pins)!=4:raise SystemExit('artifact directories must be distinct')
for directory,files in pins.items():
    if not directory.is_absolute() or directory.is_symlink() or not directory.is_dir():raise SystemExit('invalid directory')
    for name,digest in files.items():
        path=directory/name
        if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise SystemExit('identity refused: '+str(path))
roles=('PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
       'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
       'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER',
       'PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT')
with tempfile.TemporaryDirectory(prefix='qgwa-linked-receipt-',dir='/private/tmp') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                    '-fsanitize=address,undefined',*['-D'+n+'=1' for n in roles],
                    str(here/'pioneer_q38_qgwa_linked_root_receipt_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(worker),str(leaf),str(root),str(old)],check=True,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
