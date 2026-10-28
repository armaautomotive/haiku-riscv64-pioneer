#!/usr/bin/env python3
"""Literal-pinned check-routine equivalence ONLY. NEVER boot/hardware/model."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3
baseline,candidate=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert all(p.is_absolute() for p in (baseline,candidate))
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
tree=ast.parse((here.parent/'build_qwen38_global_chain_root.py').read_text())
pins={t.id:ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) for t in n.targets if isinstance(t,ast.Name) and t.id in ('PINS','UNROLLED_PINS')}
nm='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
maps=[]
for directory,key in ((baseline,'PINS'),(candidate,'UNROLLED_PINS')):
 assert pins[key]
 for name,size,digest in pins[key]:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
 symbols=subprocess.check_output([nm,'-n',str(directory/'program.elf')],text=True)
 symbols=[(int(p[0],16),p[2]) for line in symbols.splitlines() if len(p:=line.split())==3]
 def one(name):
  found=[a for a,n in symbols if n==name];assert len(found)==1,(name,found);return found[0]
 seals=[a for a,n in symbols if n=='_ZN18DensePartitionRootL4SealEPKj'];assert len(seals)==2
 maps.append((one('q38_layer_return_recurrent_seal'),seals[0],one('q38_layer_return_dense_seal'),seals[1]))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-word-seal-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test';paths=[]
 for i,mapping in enumerate(maps):
  path=pathlib.Path(tmp)/str(i);path.write_text(' '.join(format(a,'x') for a in mapping)+'\n');paths.append(path)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_word_seal_instruction_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(baseline),str(candidate),*map(str,paths)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
