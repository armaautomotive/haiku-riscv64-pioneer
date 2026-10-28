#!/usr/bin/env python3
"""Separate QGW1 worker image. BUILD ONLY: no native install/card IO."""
import ast
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit(2)
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=list(sys.argv)
try:
 sys.argv=[str(here/'build_qwen38_mixed_worker_image.py'),str(sdk),str(out/'qualified-math')]
 old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
tree=ast.parse((here/'tests/run_pioneer_q38_mixed_worker_tests.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for name,size,digest in pins:
 data=(out/'qualified-math'/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
cross=old['cross'];flags=old['flags']
def run(*args):subprocess.run([str(a) for a in args],check=True)
run(str(cross)+'-g++',*flags,'-Os','-ffixed-s11','-DPIONEER_Q38_GLOBAL_WORKER_IMAGE=1',
 '-c',here/'blackhole_qwen38_global_worker.cpp','-o',out/'global.o')
run(str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_global_worker_start.S','-o',out/'start.o')
# Internal math capsules retain their actual compiled private seal hooks.
# Only this NEW front end produces that private live seal. No old launcher,
# boot/native scope or held cohort is retagged/replaced between jobs.
link=(out/'qualified-math/program.ld').read_text()
for before,after in (('.mixed 0x20000','.globalworker 0x20000'),('.q38_mixed_start','.q38_global_start'),('*mixed.o(','*global.o(')):
 assert link.count(before)==1;link=link.replace(before,after)
assert link.count('SIZEOF(.mixed)')==2
link=link.replace('SIZEOF(.mixed)','SIZEOF(.globalworker)')
(out/'program.ld').write_text(link)
units=[n for _,n,_,_ in old['units'] if n!='mixed']
run(str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',out/'program.ld',out/'start.o',out/'global.o',
 *[out/'qualified-math'/(n+'.o') for n in units],'-Map',out/'program.map','-o',out/'program.elf')
sections=('boot','globalworker','dense','text','affine','ffn','service','gdn')
for section in sections:run(str(cross)+'-objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([str(cross)+'-nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([str(cross)+'-nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert re.search(r' T q38_global_worker_main$',symbols,re.M) and re.search(r' T q38_mixed_seal$',symbols,re.M)
for name in ('q38_mixed_main','q38_persistent_main','scalar_main'):assert not re.search(r'\b'+name+r'\b',symbols),name
dis=subprocess.check_output([str(cross)+'-objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
reserved=[s for s in dis.splitlines() if re.search(r'\bs11\b',s)]
assert len(reserved)==2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$',s)) for s in reserved)==1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$',s)) for s in reserved)==1
frames=[int(s.split('\t')[1]) for s in (out/'global.su').read_text().splitlines()];assert frames and max(frames)<=2048
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
run(sys.executable,here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',out/'program.disassembly')
rows=subprocess.check_output([str(cross)+'-objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(rows):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in rows[i+1]:assert m[1][1:] in sections,row
PINS=(('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',6264,'bdbd801a4482188efed0dea19d3a31f8c383208ea33447c842fd8312674872ba'),
 ('dense.bin',10332,'cfb3b570f61baf7010e14a4072fbe496014ef2ad792d53145ef1b794ff73233a'),
 ('text.bin',5984,'e9b643d504dc6b1e2e8d744de9c425603fa9b785ef8dab14a9e47b4249f94553'),
 ('affine.bin',5360,'47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
 ('ffn.bin',16456,'102283280e44bb8e614ff84447825fe5126f9fcd7da032f1fa8da20fc619a6d4'),
 ('service.bin',9888,'ee3f68158d9609cc3667ef9812dbe3da11666dc01baa9b4fe1baf5bfd317d2c0'),
 ('gdn.bin',35564,'a56acbed91ca3728a6e2cef4cf8566be39ae8c710fe0ad62a80b8c2032573762'),
 ('program.elf',121836,'fc8ade5bdbc69a6be95846d40ee26025d43e559ef53c7beedd52071f75e83c4f'))
for name,size,digest in PINS:
 data=(out/name).read_bytes();actual=hashlib.sha256(data).hexdigest();assert len(data)==size and actual==digest,name
 print(actual+' '+name+' bytes'+str(len(data)))
print('BUILD ONLY distinct QGW1/own boot+job+STOP/global layer64/type14/actual private down-job+wave progression/local ALL finite-row greedy partial/maxFrontFrame'+str(max(frames))+
 ';NO connected64/output ROOT/canonical input or wave coverage/native/physical/Work/token/TPS qualification. DO NOT INSTALL')
