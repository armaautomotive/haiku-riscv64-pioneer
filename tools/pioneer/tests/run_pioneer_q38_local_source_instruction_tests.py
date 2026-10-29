#!/usr/bin/env python3
"""Pinned ROOT audit-policy functions,not whole-ROOT or card execution."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
baseline,candidate=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
tree=ast.parse((here.parent/'build_qwen38_token_root.py').read_text())
pins={t.id:ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) for t in n.targets
      if isinstance(t,ast.Name) and t.id in ('PINS_CANCEL_F32_QUIET','PINS_CANCEL_F32_LOCAL_SOURCE')}
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-';maps=[]
for directory,key in ((baseline,'PINS_CANCEL_F32_QUIET'),(candidate,'PINS_CANCEL_F32_LOCAL_SOURCE')):
    assert directory.is_absolute() and pins[key], 'candidate MUST be independently pinned first'
    for name,size,digest in pins[key]:
        data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
    symbols=subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')],text=True)
    assert symbols==(directory/'program.symbols').read_text()
    names=[(int(p[0],16),p[2]) for line in symbols.splitlines() if len(p:=line.split())==3]
    def entries(name):
        result=[a for a,n in names if n==name];assert len(result)==2,(name,result);return result
    maps.append(entries('_ZN18DensePartitionRootL8BootSealEPKjPjj')+entries('_ZN18DensePartitionRootL5AdmitEPKjPjj')+entries('_ZN18DensePartitionRootL8TransferEPKjPjjjjb'))
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-local-source-instructions-',dir='/private/tmp') as tmp:
    binary=pathlib.Path(tmp)/'test';mapping=[]
    for i,addresses in enumerate(maps):
        path=pathlib.Path(tmp)/str(i);path.write_text(' '.join(format(a,'x') for a in addresses)+'\n');mapping.append(path)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    *flags,str(here/'pioneer_q38_local_source_instruction_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(baseline),str(candidate),*map(str,mapping)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
