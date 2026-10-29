#!/usr/bin/env python3
"""Returning cross-token components ONLY. No boot, persistent parent or native launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
options=sys.argv[3:]
assert 3<=len(sys.argv)<=7 and len(set(options))==len(options) and set(options)<={'--inspect-candidate','--f32-hidden','--unroll-dense-seals','--boundary-seals'}
inspect='--inspect-candidate' in options;f32='--f32-hidden' in options;unroll='--unroll-dense-seals' in options;boundary='--boundary-seals' in options
assert not unroll or f32,'separate dense-unroll candidate requires explicit FP32-hidden components'
assert not boundary or (f32 and unroll),'boundary candidate requires explicit FP32-hidden and unrolled components'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');assert pathlib.Path(tool+'g++').is_file()
out.mkdir(mode=0o700)
def run(*args):subprocess.run([str(a) for a in args],check=True)
# FIRST preserve a whole current global image and all three old returning
# object pins. No old default image gets the new admission flag.
run(sys.executable,here/'build_qwen38_global_chain_root.py',sdk,out/'unchanged-feedback','--feedback')
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS=1'] if boundary else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN=1'] if f32 else []),
 '-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1','-DPIONEER_Q38_LAYER_COMPONENT_RECURRENT=1',
 '-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL=1',
 '-DPIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT=1','-c',here/'blackhole_qwen38_return_recurrent_layer.cpp','-o',out/'recurrent.o')
run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS=1'] if boundary else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN=1'] if f32 else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL=1'] if unroll else []),
 '-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1','-DPIONEER_Q38_LAYER_COMPONENT_DENSE=1',
 '-DPIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND=1',
 '-c',here/'blackhole_qwen38_return_dense_layer.cpp','-o',out/'dense.o')
raw=out/'token-components-raw.o';capsule=out/'token-components-NOBOOT.o';old=out/'unchanged-feedback'
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections','-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
 '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',out/'recurrent.o',out/'dense.o',old/'root_conv_sections.o','-o',raw)
dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')
relocations=subprocess.check_output([tool+'objdump','-r',str(raw)],text=True)
for name in dead:assert not re.search(r'\b'+name+r'\b',relocations),name
run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,capsule)
symbols=subprocess.check_output([tool+'nm','-C',str(capsule)],text=True)
assert subprocess.check_output([tool+'nm','-u',str(capsule)],text=True).split()==['U','scalar_fault_park']
for name in ('q38_return_recurrent_layer','q38_return_dense_layer','q38_global_chain_learned_rms','q38_global_chain_quantize14'):
 assert re.search(r' T '+name+r'$',symbols,re.M),name
assert 'DensePartitionRoot::AdmitGlobalOutput(' in symbols,'the REAL returning admission path must retain the new code'
assert 'Joined::AppendTokenKv(' in symbols,'the real dense body must retain current-row GDDR append/readback'
assert ('PioneerQ38F32HiddenRms(' in symbols)==f32
assert ('PioneerQ38F32HiddenResidualRms(' in symbols)==f32
for name in (*dead,'mixed_recurrent_block_main','mixed_live_block_main','q38_global_chain_main','q38_run_final_dense_and_stop','_start','scalar_entry'):
 assert not re.search(r'\b'+name+r'\b',symbols),name
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
dis=subprocess.check_output([tool+'objdump','-d',str(capsule)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b',dis)
for line in dis.splitlines():
 m=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
 if m:assert int(m[1],16)&127!=0x73,'no controller CSR'
assert len([s for s in dis.splitlines() if re.search(r'\bmv\s+s11,\w+\s*$',s)])==4,'two existing bind/restore pairs only'
frames=[int(line.split('\t')[1]) for path in (out/'recurrent.su',out/'dense.su',old/'root_conv.su') for line in path.read_text().splitlines()]
assert frames and max(frames)<=4096,max(frames)
sections=subprocess.check_output([tool+'objdump','-h',str(capsule)],text=True).splitlines();text=conv=0
for i,row in enumerate(sections):
 m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
 if not m or 'ALLOC' not in sections[i+1]:continue
 name,size=m[1],int(m[2],16);assert name.startswith(('.text','.rodata','.srodata','.conv')),name
 if name.startswith('.conv'):conv+=size
 else:text+=size
assert text<=131072 and conv<=32768,(text,conv)
(out/'components.symbols').write_text(symbols);(out/'components.disassembly').write_text(dis)
PINS=(('recurrent.o',159428,'45263bb08b5cee6af697e2163cdf570b3ad2a86e1234a8927148674e9e25e60a'),
 ('dense.o',120792,'38e7bc8cc2e8d00c33eb9ee0424891e9a8c2099fbb767701420b57d347270278'),
 ('token-components-NOBOOT.o',248836,'d3838c28e675d0e399267a63d6259498ccd26b9b47382117685f0e3482aa5839'))
PINS_F32=(('recurrent.o',159452,'a87da94e9af7f2bb75b538a7d650304f9f6c10ce1cfadbc679f6a3b265e572f2'),
 ('dense.o',120816,'0d0cee2d978f71ba68c8a740a7ddb52ee8b486c739a2c1fca9aa1a2e58d0de6d'),
 ('token-components-NOBOOT.o',248876,'6c3017d088287e45ef8a2cbcdb31b6a0eb57dd7147c179781c7eb9cfc71ccae9'))
observed=[]
for name in ('recurrent.o','dense.o','token-components-NOBOOT.o'):
 data=(out/name).read_bytes();item=(name,len(data),hashlib.sha256(data).hexdigest());observed.append(item);print(repr(item),flush=True)
# Independent main/clean BUILD pins only. All old default pins stay fixed.
PINS_UNROLLED_DENSE_F32=(('recurrent.o',159452,'a87da94e9af7f2bb75b538a7d650304f9f6c10ce1cfadbc679f6a3b265e572f2'),
 ('dense.o',121224,'d641ff45c021d315a6c51bbb2bd95a6a52681766858f63b200e175842004c2a3'),
 ('token-components-NOBOOT.o',249276,'d44043068039b572e1b84ab0118ae87c97706423ebcdf50ccfaabd50bac7d9a9'))
# Independent main/clean BUILD pins only;runtime and changed audit timing
# require separate qualification. The original/default pins stay fixed.
PINS_BOUNDARY_F32=(('recurrent.o',161244,'a207749f83d633423c5fdcc03e3043868470a29909f488e012e1fd290f717b82'),
 ('dense.o',122564,'41ac17dea4d437a898c4d1de9fe9291b4baf1569b980a478a683c6454eb5a46e'),
 ('token-components-NOBOOT.o',252304,'716320828657aeadac1251e819f32b9b3a2c4b4292997106f81cca686f5e9a63'))
expected=PINS_BOUNDARY_F32 if boundary else PINS_UNROLLED_DENSE_F32 if unroll else PINS_F32 if f32 else PINS
if not inspect:assert expected and tuple(observed)==expected,'unqualified component variant; inspect/freeze first'
print('BUILD ONLY actual returning recurrent path includes strict previous-global output admission;ROOTbytes'+str(text)+'/131072 Conv'+str(conv)+'/32768 maxFrame'+str(max(frames))+
 ';NO boot/persistent parent/model token-state commit/native/physical/Work/TPS or combined-stack proof. DO NOT INSTALL')
