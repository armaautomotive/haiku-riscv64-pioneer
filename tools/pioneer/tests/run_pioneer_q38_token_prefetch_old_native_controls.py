#!/usr/bin/env python3
"""Reproduce ENTIRE old native subjects after shared DATA guard changes.

This does NOT build/install a prefetch driver or grant a new native owner.
"""
import hashlib
import os
import pathlib
import subprocess
import sys

assert len(sys.argv)==4 and sys.argv[1] in ('--private','--physical')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
physical=sys.argv[1]=='--physical'
previous,out=map(pathlib.Path,sys.argv[2:]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and previous.is_dir() and not previous.is_symlink()
assert out.is_absolute() and not out.exists() and not out.is_symlink()
name='flat-matrix-benchmark-native.o' if physical else 'flat-matrix-context-NODRV.o'
source='pioneer_q38_token_flat_matrix_benchmark_native_compile.cpp' if physical else 'pioneer_q38_token_flat_matrix_native_context_compile.cpp'
expected='ab0686f8d354ee5c032211003f86dd2ddbea369b168fb61134e17c99c14021f4' if physical else '98db93fac0a8a59d8fead7e71d974ab8e6b70e02c0f5dc281100ced7656ebacf'
old=previous/name
assert hashlib.sha256(old.read_bytes()).hexdigest()==expected
raw=(previous/'compile-argv.txt').read_bytes()
argv_pins=(('fcb7eabf5ca4d2d925b2a3275154bcce95db092755257084e40423e55e7b5a1b',
            '9a8cb65d390569756cb69125083b808877e5e333386ae7063f08f9bf836dcd1c') if physical else
           ('d5d0ec9024aa056f1a68accad3b27476c96afd1c662fcccd273ec65c28d912e3',
            '4c2d1605de93253dd1c5c2127b0377f1fec99d9932e9add5285ecb29e774ffa4'))
assert hashlib.sha256(raw).hexdigest() in argv_pins
args=raw.decode().splitlines()
assert args[0]=='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert args.count('-c')==args.count('-o')==1 and '-shared' not in args
assert pathlib.Path(args[args.index('-c')+1]).name==source
assert pathlib.Path(args[args.index('-o')+1])==old
out.mkdir(mode=0o700);target=out/name
args[args.index('-c')+1]=str(here/source);args[args.index('-o')+1]=str(target)
subprocess.run(args,check=True)
assert target.read_bytes()==old.read_bytes(),'ENTIRE previous qualified native object must reproduce'
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
print('PASS_UNCHANGED_OLD_NATIVE_OBJECT '+expected+' '+name,flush=True)
if physical:
    module_name='tenstorrent_pci_probe-token-FLAT-MATRIX34-BENCHMARK'
    old_module=previous/module_name
    digest='8612962e38cab47999b8aecf67268c8e7fd651917b8a570fd79f79b78e1b165f'
    assert hashlib.sha256(old_module.read_bytes()).hexdigest()==digest
    raw_link=(previous/'link-argv.txt').read_bytes()
    assert hashlib.sha256(raw_link).hexdigest() in (
        '7a3c9129421fa7704eaf93c9873bafdbf31cd2f1a0ef21f7f7b6e33f42ccdbc0',
        '815134e26419cf424de8e56eaaf967057862ebb0b721853ea88918694fcb5f80')
    link=raw_link.decode().splitlines()
    assert link[0]==args[0] and link[1]=='-shared' and link[6]==str(old) and link[-1]==str(old_module)
    module=out/module_name;link[6]=str(target);link[-1]=str(module)
    subprocess.run(link,check=True)
    assert module.read_bytes()==old_module.read_bytes(),'ENTIRE old8612 driver must reproduce'
    (out/'link-argv.txt').write_text('\n'.join(link)+'\n')
    print('PASS_UNCHANGED_OLD_NATIVE_MODULE '+digest+' '+module_name,flush=True)
print('PASS_OLD_NATIVE_CONTROL_ONLY current shared guards preserve qualified code;NO prefetch native owner,new driver,install,lifecycle,model or speed grant')
