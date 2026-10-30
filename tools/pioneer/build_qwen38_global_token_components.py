#!/usr/bin/env python3
"""Returning cross-token components ONLY. No boot, persistent parent or native launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
options=sys.argv[3:]
assert 3<=len(sys.argv)<=13 and len(set(options))==len(options) and set(options)<={'--inspect-candidate','--f32-hidden','--unroll-dense-seals','--boundary-seals','--unroll-quiet','--worker-local-source-guards','--precise-activation','--single-closed-audit','--worker-sealed-admission','--pushed-completion'}
inspect='--inspect-candidate' in options;f32='--f32-hidden' in options;unroll='--unroll-dense-seals' in options;boundary='--boundary-seals' in options
assert not unroll or f32,'separate dense-unroll candidate requires explicit FP32-hidden components'
assert not boundary or (f32 and unroll),'boundary candidate requires explicit FP32-hidden and unrolled components'
quiet='--unroll-quiet' in options
assert not quiet or (boundary and f32 and unroll),'queue-check candidate requires explicit boundary+FP32-hidden+unrolled components'
local_sources='--worker-local-source-guards' in options
assert not local_sources or quiet,'worker-local source candidate requires the explicit queue/boundary/FP32 profile'
precise='--precise-activation' in options
assert not precise or local_sources,'precise worker activation requires the explicit FP32/local-source profile'
single_audit='--single-closed-audit' in options
assert not single_audit or precise,'single closed audit requires the explicit QGW2 precise profile'
worker_admission='--worker-sealed-admission' in options
assert not worker_admission or single_audit,'worker-sealed admission requires the explicit QGW2 single-closed-audit profile'
pushed='--pushed-completion' in options
assert not pushed or worker_admission,'QGW3 receiver requires the entire explicit returning worker-admission profile'
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
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT=1'] if single_audit else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION=1'] if worker_admission else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1'] if pushed else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'] if precise else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS=1'] if local_sources else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL=1'] if quiet else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS=1'] if boundary else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN=1'] if f32 else []),
 '-DPIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE=1','-DPIONEER_Q38_LAYER_COMPONENT_RECURRENT=1',
 '-DPIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL=1',
 '-DPIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT=1','-c',here/'blackhole_qwen38_return_recurrent_layer.cpp','-o',out/'recurrent.o')
run(tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1','-DPIONEER_Q38_SIGMOID_PRECISION_IMAGE=1',
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT=1'] if single_audit else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION=1'] if worker_admission else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1'] if pushed else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'] if precise else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS=1'] if local_sources else []),
 *(['-DPIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL=1'] if quiet else []),
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
 ('dense.o',122944,'3ad64af1f649f6a9f67996e565d9dc851d91c9f7dfc307eba8609f4624b5532c'),
 ('token-components-NOBOOT.o',252780,'60a21a352d28a7c6e8dd351bd125816b1f76967e993b60940d652673a8bbe2e4'))
# Independent main/clean BUILD pins only;not runtime/native/Work permission.
PINS_QUIET_F32=(('recurrent.o',161264,'86f3263a04a696e8fb54e7425ed7ea37e8d2055a7bbca44754c9e07301f1827a'),
 ('dense.o',122964,'2e52cefca38eb54ac89cd49b4fe1679a35f158044f2e7bfb969d96cfe14e5d10'),
 ('token-components-NOBOOT.o',252860,'8fdae3782d66e03be750947cb8c77ffa218916e28540708e13b9c9fa53efb778'))
PINS_LOCAL_SOURCE_F32=(('recurrent.o',160776,'84873403ab0fb39d7da1c2e7faabb169ae3b5e61b9f7407e5a444f9895fcc984'),
 ('dense.o',122528,'a9ce4d4ec6ce1d1517c8809b236503293f9e20e1b3c9ddf1d6976f92c4e1c851'),
 ('token-components-NOBOOT.o',252072,'c3502896520eeab667b7c2762a2ac87cb41243a1e4b9347179572e69ea0c5d80')) # independently matched BUILD pins,NOT runtime permission
# Identical independently rebuilt main/clean components. BUILD pins only:
# connected original-model instructions/stack/numerics are separate tests.
PINS_PRECISE_F32=(('recurrent.o',173044,'28918095da4e6434b539d343f578f35ea42c79ef5da6c78b3e8e7331e9ff6582'),
 ('dense.o',136868,'dd41c80ea9b6503339794b128a773e36b942d186e105eec3931469f0bb4c0875'),
 ('token-components-NOBOOT.o',271784,'532fe03b7ba5049a39eb388ce73e4c8af82276a9505377b0fc1078c0df6c1239'))
PINS_SINGLE_AUDIT_F32=(('recurrent.o',173168,'9c8910ee2e6acb94d411c576fd94b9222dd69b1f3999fb393551b0daeaed39e3'),
 ('dense.o',136968,'c7b04051ff8ab48e507660eb0302888f9efe31c4def11c592fe31d8d2be291ad'),
 ('token-components-NOBOOT.o',271992,'66caa1158cff728ea02bf6c3774c39a22649ee32d86fb77591a8f9e348ae47fe')) # independent BUILD pins only;not runtime/fault-timing or native/Work permission
PINS_WORKER_ADMISSION_F32=(('recurrent.o',173332,'ec717b89aa276961a5afb7031fc81a4903a1a6f58e205cb156684b765914ae8c'),
 ('dense.o',137028,'c663c6db28eb3b989409b139909ae6a7ef106442a51947d1908219d4a4c7ac72'),
 ('token-components-NOBOOT.o',272180,'de5ecfb69aa172e36d63f8929c0279ef58b0486d00a4856bfa6dc500941cbd69')) # independent BUILD pins ONLY;not runtime/fault timing/native/Work qualification
PINS_PUSHED_F32=(
    ('recurrent.o', 182356, '178660fe0182a5e2483bf15de082241727c82385bdf5f79ad8c2dedc6b87bfaa'),
    ('dense.o', 145804, '9760a9aac8800b6c5abecaabb5cfbbc9ad1df0261b106b55c6d98ddc3a822a5d'),
    ('token-components-NOBOOT.o', 282060, '0a42528831ba89179ac343dcf926a48ea0ce20d144894c95596f7d2c524b4500'),
) # independent BUILD pins ONLY;not runtime/native/Work/physical/TPS.
expected=PINS_PUSHED_F32 if pushed else PINS_WORKER_ADMISSION_F32 if worker_admission else PINS_SINGLE_AUDIT_F32 if single_audit else PINS_PRECISE_F32 if precise else PINS_LOCAL_SOURCE_F32 if local_sources else PINS_QUIET_F32 if quiet else PINS_BOUNDARY_F32 if boundary else PINS_UNROLLED_DENSE_F32 if unroll else PINS_F32 if f32 else PINS
if not inspect:assert expected and tuple(observed)==expected,'unqualified component variant; inspect/freeze first'
print('BUILD ONLY actual returning recurrent path includes strict previous-global output admission;ROOTbytes'+str(text)+'/131072 Conv'+str(conv)+'/32768 maxFrame'+str(max(frames))+
 ';NO boot/persistent parent/model token-state commit/native/physical/Work/TPS or combined-stack proof. DO NOT INSTALL')
