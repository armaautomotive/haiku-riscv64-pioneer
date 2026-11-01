#!/usr/bin/env python3
"""Prove the dormant DATA selector leaves the entire old SDK/module unchanged."""
import hashlib
import os
import pathlib
import subprocess
import sys

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
previous, out = map(pathlib.Path, sys.argv[1:])
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here = pathlib.Path(__file__).resolve().parent
object_sha = '04da32526ffb4b2bc297ae21375a15e3d3df7141c60cca6747abba64189a3f4a'
module_sha = '1f3bcff838ff9f95d5f450c53000f02f80ddcd073aa5a5cc3018ee4208e0847c'
old_object = previous / 'retained-cohort-benchmark-native.o'
old_module = previous / 'tenstorrent_pci_probe-token-RETAINED-COHORT34-BENCHMARK'
assert hashlib.sha256(old_object.read_bytes()).hexdigest() == object_sha
assert hashlib.sha256(old_module.read_bytes()).hexdigest() == module_sha
compile_bytes = (previous / 'compile-argv.txt').read_bytes()
link_bytes = (previous / 'link-argv.txt').read_bytes()
assert (hashlib.sha256(compile_bytes).hexdigest(), hashlib.sha256(link_bytes).hexdigest()) in (
    ('dc9a56861470aeb045ef38ec7b414b64550f9667f1bedaa56181a8a210889933',
     '1117a912649ecc7479e1b031c2ddf74a9c6be242c60ca3d8b593246410c9986e'),
    ('9b6ac8470781f470ed476d03bae09f582146be23c9b4bdf2337e64742c28ffb4',
     'b325fa50c37b3de69469268317af354418a54bcc42ce5965d5341028f9ee3fa4'))
args = compile_bytes.decode().splitlines()
link = link_bytes.decode().splitlines()
compiler = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert args[0] == compiler and args.count('-c') == args.count('-o') == 1 and '-shared' not in args
assert pathlib.Path(args[args.index('-c') + 1]).name == 'pioneer_q38_token_retained_cohort_benchmark_native_compile.cpp'
assert pathlib.Path(args[args.index('-o') + 1]) == old_object
assert '-DPIONEER_DRIVER_BUILD_TAG=0x2026103100010075ULL' in args
assert link[:4] == [compiler, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[6] == str(old_object) and link[-2:] == ['-o', str(old_module)]
assert all(pathlib.Path(value).is_file() for value in link[4:-2])
out.mkdir(mode=0o700)
obj, module = out / old_object.name, out / old_module.name
args[args.index('-c') + 1] = str(here / 'pioneer_q38_token_retained_cohort_benchmark_native_compile.cpp')
args[args.index('-o') + 1] = str(obj)
subprocess.run(args, check=True)
assert obj.read_bytes() == old_object.read_bytes()
link[6], link[-1] = str(obj), str(module)
subprocess.run(link, check=True)
assert module.read_bytes() == old_module.read_bytes()
(out / 'compile-argv.txt').write_text('\n'.join(args) + '\n')
(out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
print('PASS dormant commit-boundary DATA role: ENTIRE old04da3252 SDK object+1f3bcff8 diagnostic module '
      'byte-identical after independent current-source compile/link;no candidate native selector,install,execution or TPS grant')
