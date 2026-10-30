#!/usr/bin/env python3
"""Pinned emitted packet-check equivalence ONLY; never boot/device/model IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

eight='--unroll-packet-seals-eight' in sys.argv
if eight:
 assert sys.argv.count('--unroll-packet-seals-eight')==1
 sys.argv.remove('--unroll-packet-seals-eight')
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
baseline,candidate=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert baseline.is_absolute() and candidate.is_absolute()
tree=ast.parse((here.parent/'build_qwen38_token_root.py').read_text())
old_pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='PINS_CANCEL_F32_PUSHED' for t in n.targets))
# Experimental BUILD pins only, NOT DATA/native image selection or authority.
candidate_pins=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',130976,'b5e59a5fec54dc60e318f805e4a616886b342dae5d1411ffab48262fd48ae1c9'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',32720,'6bb4ac3fd7ac99f21c80776b4ed37c6b344c50720e8c032a77128bf0c214800b'),
 ('program.elf',190444,'bcdd540580c025abadcd97054fd8b34274be4fd8a870a10cbad8ab7f3da3edb1'))
if eight:
 candidate_pins=(
  ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
  ('text.bin',131040,'71367a70c4fa3dcf0db172edae3b6b915fc69352585cbb71f3bfb7455f72e279'),
  ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
  ('conv.bin',32720,'04d55aceadc5c743039a62ac969ccb61682e0dbf070c08fb722a19ed4beb4123'),
  ('program.elf',190444,'df4261ec5bb3d2f9ada2f11f82b1e52d8d1938c704efd70a4bc150aaab28c933'))
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
entries=[]
for directory,pins in ((baseline,old_pins),(candidate,candidate_pins)):
 for name,size,digest in pins:
  raw=(directory/name).read_bytes();assert len(raw)==size and hashlib.sha256(raw).hexdigest()==digest,(directory,name)
 symbols=subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')],text=True)
 assert symbols==(directory/'program.symbols').read_text()
 found=re.findall(r'^([0-9a-f]+) T q38_layer_return_recurrent_packet_seal$',symbols,re.M)
 assert len(found)==1;entries.append(int(found[0],16))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-packet-seal-unroll-',dir='/private/tmp') as temporary:
 out=pathlib.Path(temporary);maps=[]
 for i,entry in enumerate(entries):
  path=out/str(i);path.write_text(' '.join([format(entry,'x')]*4)+'\n');maps.append(path)
 binary=out/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_packet_seal_unroll_instruction_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(baseline),str(candidate),*map(str,maps)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
