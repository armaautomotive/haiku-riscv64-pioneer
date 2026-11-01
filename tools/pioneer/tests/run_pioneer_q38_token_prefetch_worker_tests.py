#!/usr/bin/env python3
"""Frozen actual current-QGW3 worker; synthetic endpoints, NO device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

args=sys.argv[1:]
pilot=args.pop() if args[-1:] in (['--matrix-pilot'],['--async-pilot']) else None
assert len(args)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
old,baseline,image=map(pathlib.Path,args)
here=pathlib.Path(__file__).resolve().parent
for directory,script in ((old,here/'run_pioneer_q38_mixed_worker_tests.py'),
                         (baseline,here.parent/'build_qwen38_precise_beta_worker_image.py'),
                         (image,here.parent/'build_qwen38_token_prefetch_worker.py')):
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    tree=ast.parse(script.read_text())
    pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    assert pins
    for name,size,digest in pins:
        path=directory/name
        assert path.is_file() and not path.is_symlink()
        data=path.read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
# Whole immutable ELF and extracted fetch bytes above are pinned. These
# are observer PCs ONLY, never guest state or a completion substitution.
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
symbols=subprocess.check_output([tool+'nm','-n',str(image/'program.elf')],text=True)
assert symbols==(image/'program.symbols').read_text()
for address,name in (('00070000','_ZL16TilePrefetchWaitj'),('0007018c','_ZL17TilePrefetchStagejjjjjjjRj')):
    assert re.search(r'^'+address+r' t '+name+r'$',symbols,re.M)
# Old default-false observer path must still pass before the new candidate.
subprocess.run([sys.executable,str(here/'run_pioneer_q38_precise_beta_worker_tests.py'),str(old),str(baseline)],check=True)
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-q38-token-prefetch-execution-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_token_prefetch_worker_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(old),str(baseline),str(image),*([pilot] if pilot else [])],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
