#!/usr/bin/env python3
"""DISTINCT QGC1 all64+output build/inspection ONLY, never a native launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3] in ('--inspect-candidate','--unrolled-seals','--inspect-unrolled-candidate','--feedback','--inspect-feedback-candidate'))
mode=sys.argv[3] if len(sys.argv)==4 else ''
feedback=mode in ('--feedback','--inspect-feedback-candidate')
unrolled=feedback or mode in ('--unrolled-seals','--inspect-unrolled-candidate')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');assert pathlib.Path(tool+'g++').is_file()
out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
# FIRST reproduce the unchanged old components and literal pins. A new
# component flag is never permission to change or relabel an old image.
run(sys.executable,here/'build_qwen38_layer_return_components.py',sdk,out/'unchanged-components')
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
component=['-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1']
for kind,parent in (('recurrent','PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE'),('dense','PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE')):
 run(tool+'g++',*flags,*component,*(['-DPIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL=1'] if unrolled else []),'-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1','-DPIONEER_Q38_LAYER_COMPONENT_'+kind.upper()+'=1',
  '-D'+parent+'=1','-c',here/('blackhole_qwen38_return_'+kind+'_layer.cpp'),'-o',out/(kind+'.o'))
run(tool+'g++',*flags,*component,'-DPIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE=1',
 '-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE=1',
 '-Dscalar_main=q38_mixed_recurrent_root_conv','-Dq38_mixed_root_seal=q38_layer_return_recurrent_seal',
 '-include',here/'PioneerQ38GlobalChainComponentRole.h','-c',here/'blackhole_qwen38_conv_batch_silu_precision.cpp','-o',out/'root_conv.o')
run(tool+'objcopy','--prefix-alloc-sections','.conv',out/'root_conv.o',out/'root_conv_sections.o')
extra=[]
if feedback:
 run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK=1','-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK_CAPSULE_IMAGE=1',
  '-c',here/'blackhole_qwen38_global_feedback_decode.cpp','-o',out/'feedback.o')
 extra=[out/'feedback.o']
run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE=1',*(['-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK=1'] if feedback else []),'-c',here/'blackhole_qwen38_global_chain_root.cpp','-o',out/'program.o')
run(tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here/'blackhole_qwen38_global_chain_start.S','-o',out/'start.o')
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'blackhole_qwen38_global_chain.ld',
 out/'start.o',out/'program.o',out/'recurrent.o',out/'dense.o',out/'root_conv_sections.o',*extra,'-Map',out/'program.map','-o',out/'program.elf')
for section in ('boot','text','terminal','conv'):run(tool+'objcopy','-O','binary','-j','.'+section,out/'program.elf',out/(section+'.bin'))
symbols=subprocess.check_output([tool+'nm','-n',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
 assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
for name in ('q38_global_chain_main','q38_return_recurrent_layer','q38_return_dense_layer','q38_global_chain_learned_rms','q38_global_chain_quantize14'):
 assert re.search(r'^[0-9a-f]+ T '+name+'$',symbols,re.M),name
for name in ('q38_layer_chain_main','q38_run_final_dense_and_stop','mixed_recurrent_block_main','mixed_live_block_main',
 'mixed_live_block_stopped','mixed_recurrent_block_stopped','mixed_recurrent_block_wait','mixed_live_block_wait','q38_mixed_root_seal','q38_feedback_unbound_embedding_main'):
 assert not re.search(r'\b'+name+r'\b',symbols),name
assert bool(re.search(r'^[0-9a-f]+ T q38_global_chain_feedback_decode$',symbols,re.M))==feedback
dis=subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
writes=[s for s in dis.splitlines() if re.search(r'\bmv\s+s11,\w+\s*$',s)]
assert len(writes)==5,'one new parent + two component bind/restore pairs,NO final-dense STOP bind'
frames=[int(line.split('\t')[1]) for line in (out/'program.su').read_text().splitlines()]
component_frames=[int(line.split('\t')[1]) for name in ('recurrent','dense','root_conv') for line in (out/(name+'.su')).read_text().splitlines()]
if feedback:component_frames += [int(line.split('\t')[1]) for line in (out/'feedback.su').read_text().splitlines()]
assert frames and max(frames)<=2048,max(frames)
assert component_frames and max(component_frames)<=4096,max(component_frames) # unchanged returning-component bound,NOT combined SP proof
sections=subprocess.check_output([tool+'objdump','-h',str(out/'program.elf')],text=True).splitlines()
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if m and 'ALLOC' in sections[i+1]:assert m[1] in ('.boot','.text','.terminal','.conv'),row
(out/'program.symbols').write_text(symbols);(out/'program.disassembly').write_text(dis)
PINS=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',113840,'6f458ba36e34a427a08b2c199ffc3fedc235c6dbfe55d355c131d2a37e6eb7ce'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',7500,'5fce4465b4d1f30a616311e97f9216d06e4aaf6a09d79de34b1e4ea31b0b004a'),
 ('program.elf',145552,'43cf308d15a7db38cde9e80804d78870a2222788612b8ccf11806e05a21ebc44'))
UNROLLED_PINS=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',114404,'6add798d20f27889783f50c54285ac9129624335711943fb690c188097ed40fd'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',7500,'5fce4465b4d1f30a616311e97f9216d06e4aaf6a09d79de34b1e4ea31b0b004a'),
 ('program.elf',145552,'167f7b32faafdfecb393b9e632af88ff0647d62df164e4e772884179debab561'))
FEEDBACK_PINS=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',117016,'e58aff9bf5be041992ba5435bca334bdd43ac51c3d3c6af47c5ac6de7cdbfd8d'),
 ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
 ('conv.bin',7500,'aec9aeacaf188b46ae223bd79fafb928ac39a3369b3c9753bf8145966eb44cea'),
 ('program.elf',149964,'ca0f6beb05e1a20d243332e65ce9b0bc4e3e080fc350768067266b9e1d7209a2'))
observed=[]
for name in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):
 data=(out/name).read_bytes();item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(repr(item),flush=True)
if mode not in ('--inspect-candidate','--inspect-unrolled-candidate','--inspect-feedback-candidate'):
 expected=FEEDBACK_PINS if feedback else UNROLLED_PINS if unrolled else PINS
 assert expected and tuple(observed)==expected,'unqualified candidate; inspect/freeze independent builds first'
print('BUILD/INSPECT ONLY NEW QGC1/all64 returning arithmetic+final norm+973 vocab jobs+greedy candidate+real global STOP/maxFrame'+str(max(frames))+
 ('/on-card provisional next embedding' if feedback else '/explicit unrolled seals' if unrolled else '/baseline seals')+
 ';NO runtime/combined reserved stack/original model/native/physical/token commit/multi-token/Work/TPS qualification. DO NOT INSTALL')
