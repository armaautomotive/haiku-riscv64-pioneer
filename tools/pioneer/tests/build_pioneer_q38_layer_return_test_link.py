#!/usr/bin/env python3
"""Link the pinned component capsule for instruction-model calls ONLY, no boot."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
if len(sys.argv)!=4 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
sdk,qualified,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and qualified.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
capsule=qualified/'layer-components-NOBOOT.o';data=capsule.read_bytes()
assert len(data)==228380 and hashlib.sha256(data).hexdigest()=='78cf19c422c773a8c9c47c180df78ecd66609e4614dd75ee2d66344cab0ea25c'
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
run(tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'pioneer_q38_layer_return_test_start.S','-o',out/'test-start.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-u','layer_return_test_begin_dense','-T',here/'pioneer_q38_layer_return_test.ld',
 out/'test-start.o',capsule,'-Map',out/'program.map','-o',out/'program.elf')
for section in ('text','conv','fault'):run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name in ('layer_return_test_begin_recurrent','layer_return_test_begin_dense','q38_return_recurrent_layer','q38_return_dense_layer','scalar_fault_park','layer_return_test_returned'):
 assert re.search(r'^[0-9a-f]+ T '+name+r'$',symbols,re.M),name
for forbidden in ('_start','scalar_entry','mixed_recurrent_block_main','mixed_live_block_main','q38_mixed_root_seal'):
 assert not re.search(r'\b'+forbidden+r'\b',symbols),forbidden
(out/'program.symbols').write_text(symbols)
sections=subprocess.check_output([tool+'objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in sections[i+1]:assert m[1] in ('.text','.conv','.fault'),row
PINS=(('text.bin',93012,'dccbad4542394c3b87bc846849ea0d276d1d55ad52c6d7dbeb0956788121adf1'),
 ('conv.bin',7500,'e30559f9964a2f744250f51e197ba30ef2d7a37bccb89e395ffaa30d180967ef'),
 ('fault.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('program.elf',119168,'af896d8a2c3a91515620005d57f4eb126c4b6432e8937ea0630462da3aa8b376'))
for name,size,digest in PINS:
 data=(out/name).read_bytes();actual=hashlib.sha256(data).hexdigest()
 assert len(data)==size and actual==digest,name
 print(actual+' '+name+' bytes'+str(len(data)))
print('PASS TEST-only fixed-address caller/actual pinned two component bodies/NO boot or native ABI/NO unresolved/writable globals. Caller initializes SP/s11 by emitted instructions;return and fault are distinct inert loops. LINK ONLY,not executed/connected pipeline/physical/Work/TPS. DO NOT INSTALL')
