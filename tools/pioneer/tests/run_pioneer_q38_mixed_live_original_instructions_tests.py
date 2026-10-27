#!/usr/bin/env python3
"""Fixed ORIGINAL bytes + actual instructions; separate private CPU oracle.

NO device IO/native/Work driver. A numerical mismatch is a failing test,
never a fitted tolerance or a throughput claim.
"""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) >= 7
options=sys.argv[7:]
assert len(set(options))==len(options) and set(options)<={'--bytes-only','--verify-only','--sigmoid-candidate'}
assert not {'--bytes-only','--verify-only'}<=set(options)
sigmoid='--sigmoid-candidate' in options
here = pathlib.Path(__file__).resolve().parent
model, embedding, root, worker, qdp, qda = map(pathlib.Path, sys.argv[1:7])
assert model.stat().st_size == 18973870528

def records(name):
    tree = ast.parse((here / name).read_text())
    return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))

def pin(directory, entries):
    for name, size, digest in entries:
        data = (directory / name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)

if sigmoid:
    candidate=ast.parse((here.parent/'build_qwen38_mixed_live_sigmoid_root_image.py').read_text())
    candidate_pins=next(ast.literal_eval(n.value) for n in candidate.body if isinstance(n,ast.Assign)
                        and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    pin(root,candidate_pins)
else:
    pin(root, records('run_pioneer_q38_mixed_live_block_root_tests.py'))
pin(worker, records('run_pioneer_q38_mixed_worker_tests.py'))
pin(embedding, (('code.bin', 3856, 'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'),))
pin(qdp, (('code.bin', 4836, 'ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),))
pin(qda, (('code.bin', 14428, 'e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736'),))
for directory in (embedding, qdp, qda):
    pin(directory, (('boot.bin', 4, 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),))
if '--verify-only' in options:
    print('PASS exact original-instruction artifact pins/'+('EXPLICIT_SIGMOID_CANDIDATE' if sigmoid else 'DEFAULT')+'/root+worker+QPE1+QDP+QDA boots/code;NO model SHA,execution or hardware claim')
    sys.exit(0)
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
          '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-original-instructions-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    objects = []
    for source, options in (
        ('pioneer_q38_mixed_live_original_instructions.cpp', flags),
        ('pioneer_q38_mixed_live_original_embedding.cpp', []),
        ('../pioneer_q38_mixed_live_original_source.cpp', []),
        ('pioneer_q38_mixed_live_original_instructions_test.cpp', []),
    ):
        output = base / (pathlib.Path(source).stem + '.o')
        subprocess.run([*common, *options, '-c', str(here / source), '-o', str(output)], check=True)
        objects.append(str(output))
    binary = base / 'test'
    subprocess.run([*common, *objects, '-o', str(binary)], check=True)
    result = subprocess.run([str(binary), *map(str, (model, embedding, root, worker, qdp, qda)),
                             *(['--bytes-only'] if '--bytes-only' in options else [])],
                            env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    sys.exit(result.returncode)
