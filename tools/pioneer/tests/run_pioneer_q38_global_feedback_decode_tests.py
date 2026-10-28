#!/usr/bin/env python3
"""Pinned RETURNING arithmetic only, no owner/whole-graph or device grant."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv) in (2,3)
image=pathlib.Path(sys.argv[1]);here=pathlib.Path(__file__).resolve().parent
assert image.is_absolute() and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
tree=ast.parse((here.parent/'build_qwen38_global_chain_root.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='FEEDBACK_PINS' for t in n.targets));assert pins
for name,size,digest in pins:
 data=(image/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(name,size)
nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
symbols=subprocess.check_output([nm,'-n',str(image/'program.elf')],text=True)
entries=re.findall(r'^([0-9a-f]+) T q38_global_chain_feedback_decode$',symbols,re.M);assert len(entries)==1
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-feedback-decode-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test';mapping=pathlib.Path(tmp)/'entry';mapping.write_text(entries[0]+'\n')
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_global_feedback_decode_test.cpp'),str(here/'pioneer_q38_global_output_original_source.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(image),str(mapping),*sys.argv[2:]],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
