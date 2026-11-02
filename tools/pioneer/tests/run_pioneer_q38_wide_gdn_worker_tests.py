#!/usr/bin/env python3
"""Actual frozen full worker/shared tensor-state tests;synthetic endpoints only."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require,stable_bytes

require(len(sys.argv)==5,'OLD_MIXED PRECISE_BETA OLD_TWIN NEW_WIDE_IMAGE')
require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'no inherited role overrides')
here=pathlib.Path(__file__).resolve().parent
directories=list(map(pathlib.Path,sys.argv[1:]))
scripts=[here/'run_pioneer_q38_mixed_worker_tests.py',here.parent/'build_qwen38_precise_beta_worker_image.py',here.parent/'build_qwen38_token_twin_batch_prefetch_worker.py',here.parent/'build_qwen38_wide_gdn_worker_image.py']
snapshots=[]
for directory,script in zip(directories,scripts):
 require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(),'absolute original image directory')
 tree=ast.parse(script.read_text());pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
 require(bool(pins),'frozen image pins required before TEST execution')
 data=[]
 for name,size,digest in pins:
  raw=stable_bytes(directory/name,size);require(hashlib.sha256(raw).hexdigest()==digest,'whole selected image '+name);data.append((name,raw))
 snapshots.append(data)
# MixedPeer constructs the historical six-section fixture before replacing
# ALL its ROM before the first instruction. Preserve that real constructor;
# snapshot its dependency too, rather than omitting it or bypassing checks.
legacy_script=here/'run_pioneer_q38_resident_gdn_output_operator_service_image_tests.py'
legacy_tree=ast.parse(legacy_script.read_text())
legacy_pins=next(ast.literal_eval(n.value) for n in legacy_tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
legacy_records=[(name+'.bin',size,digest) for name,size,digest in legacy_pins]
legacy_records.append(('program.elf',96148,'44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19'))
legacy_snapshot=[]
for name,size,digest in legacy_records:
 raw=stable_bytes(directories[0]/'unchanged-worker'/name,size)
 require(hashlib.sha256(raw).hexdigest()==digest,'whole historical fixture '+name)
 legacy_snapshot.append((name,raw))
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-wide-gdn-worker-execution-',dir='/private/tmp') as temporary:
 root=pathlib.Path(temporary);images=[]
 for index,items in enumerate(snapshots):
  image=root/('AUTHENTICATED_ROM_'+str(index));image.mkdir(mode=0o700)
  for name,raw in items:
   target=image/name
   with target.open('xb') as stream:stream.write(raw)
   target.chmod(0o400)
  # Derive symbols ONLY from that same private authenticated whole ELF,
  # never a caller-provided text file that could redirect a TEST terminal.
  (image/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(image/'program.elf')]))
  (image/'program.symbols').chmod(0o400);images.append(image)
 legacy=images[0]/'unchanged-worker';legacy.mkdir(mode=0o700)
 for name,raw in legacy_snapshot:
  target=legacy/name
  with target.open('xb') as stream:stream.write(raw)
  target.chmod(0o400)
 # ENTIRE old twin suite and all its earlier controls FIRST,not a pilot.
 subprocess.run([sys.executable,str(here/'run_pioneer_q38_twin_batch_prefetch_worker_tests.py'),*map(str,images[:3])],check=True)
 binary=root/'wide-test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_wide_gdn_worker_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(images[0]),str(images[3])],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('CLOSED actual new full wide worker tests AFTER ENTIRE unchanged old twin+ancestors/same-open exact ROM snapshots/ONLY shared cfg,thread,LReg,physicalDst arrays;NO native owner,original model,physical,Work or TPS grant')
