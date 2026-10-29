#!/usr/bin/env python3
"""Isolated pinned boundary-audit controller tests; no native or hardware IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3 or (len(sys.argv)==4 and sys.argv[3]=='--quiet')
quiet=len(sys.argv)==4
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
baseline,candidate=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
tree=ast.parse((here.parent/'build_qwen38_token_root.py').read_text())
pins={t.id:ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) for t in n.targets if isinstance(t,ast.Name) and t.id in ('PINS_CANCEL_F32_UNROLLED_DENSE','PINS_CANCEL_F32_BOUNDARY','PINS_CANCEL_F32_QUIET')}
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-';maps=[]
for directory,key in ((baseline,'PINS_CANCEL_F32_BOUNDARY' if quiet else 'PINS_CANCEL_F32_UNROLLED_DENSE'),(candidate,'PINS_CANCEL_F32_QUIET' if quiet else 'PINS_CANCEL_F32_BOUNDARY')):
 assert directory.is_absolute() and pins[key]
 for name,size,digest in pins[key]:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
 symbols=subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')],text=True)
 assert symbols==(directory/'program.symbols').read_text()
 names=[(int(p[0],16),p[2]) for line in symbols.splitlines() if len(p:=line.split())==3]
 def entries(name,count):
  result=[a for a,n in names if n==name];assert len(result)==count,(name,result);return result
 addresses=(*entries('_ZN18DensePartitionRootL14PhysicalSourceEPKjPjj',2),*entries('_ZN20MixedRecurrentCohortL7SourcesEPjj',1),
              *entries('_ZN18DensePartitionRootL8TransferEPKjPjjjjb',2),*entries('_ZN20MixedRecurrentCohortL6PacketEPjjjjb',1))
 if key in ('PINS_CANCEL_F32_BOUNDARY','PINS_CANCEL_F32_QUIET'):
  addresses+=(*entries('q38_layer_return_dense_packet_frame',1),*entries('q38_layer_return_dense_packet_seal',1),*entries('q38_joined_dense_frame',1))
 maps.append(addresses)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-boundary-instructions-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test';mapping=[]
 for i,addresses in enumerate(maps):
  path=pathlib.Path(tmp)/str(i);path.write_text(' '.join(format(a,'x') for a in addresses)+'\n');mapping.append(path)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *flags,str(here/'pioneer_q38_boundary_seal_instruction_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(baseline),str(candidate),*map(str,mapping)],check=True,
  env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
