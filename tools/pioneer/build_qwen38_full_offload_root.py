#!/usr/bin/env python3
"""Build separate complete full-card EOG ROOT;never open a device/install/run."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

args=sys.argv[1:]
inspect_candidate='--inspect-candidate' in args
if inspect_candidate:
    if args.count('--inspect-candidate')!=1:raise SystemExit(2)
    args.remove('--inspect-candidate')
PINS_FFN_SHARED_INPUT_REUSE=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
  ('text.bin',131040,'641babd1746c1f89aef05b30057808b9887f235a3c757006472c33c8f69ed9f0'),
  ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
  ('conv.bin',32708,'54d2b80ea35c20d37e523764024307e609b11160c4b5499655ce64fd266964bf'),
  ('program.elf',190500,'ed959881d2d85d2f135a5236a6abf075dcc6e5871d73d7233e7684aac7125110'))
ffn_shared_reuse='--ffn-shared-input-reuse' in args
if ffn_shared_reuse:
    if args.count('--ffn-shared-input-reuse')!=1:raise SystemExit(2)
    args.remove('--ffn-shared-input-reuse')
assert __debug__ and len(args)==2
assert not inspect_candidate or ffn_shared_reuse
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,args)
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_root.py'),str(build),str(out/'unchanged-pushed'),
              '--cancel-boundary','--f32-hidden','--unroll-dense-seals','--boundary-seals','--unroll-quiet',
              '--worker-local-source-guards','--precise-activation','--single-closed-audit','--worker-sealed-admission','--pushed-completion',
              *(('--inspect-candidate','--ffn-shared-input-reuse') if ffn_shared_reuse else ())]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
tool,components=old['tool'],old['components'];commands=[]
def run(command):
    commands.append(command);subprocess.run(command,check=True)
flags=old['flags']
run([tool+'g++',*flags,'-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK=1',
     '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1','-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
     '-DPIONEER_Q38_TOKEN_CANCEL_BOUNDARY=1','-DPIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE=1',
     '-c',str(here/'blackhole_qwen38_token_root.cpp'),'-o',str(out/'program.o')])
run([tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'blackhole_qwen38_global_chain.ld'),
     str(out/'unchanged-pushed/start.o'),str(out/'program.o'),str(components/'token-components-NOBOOT.o'),
     str(components/'unchanged-feedback/feedback.o'),'-Map',str(out/'program.map'),'-o',str(out/'program.elf')])
for section in ('boot','text','terminal','conv'):
    run([tool+'objcopy','-O','binary','-j','.'+section,str(out/'program.elf'),str(out/(section+'.bin'))])
assert (out/'boot.bin').read_bytes()==(out/'unchanged-pushed/boot.bin').read_bytes()
assert (out/'terminal.bin').read_bytes()==(out/'unchanged-pushed/terminal.bin').read_bytes()
assert 0<len((out/'text.bin').read_bytes())<=0x20000 and 0<len((out/'conv.bin').read_bytes())<=0x8000
assert (out/'text.bin').read_bytes()!=(out/'unchanged-pushed/text.bin').read_bytes()
assert (out/'conv.bin').read_bytes()!=(out/'unchanged-pushed/conv.bin').read_bytes()
symbols=subprocess.check_output([tool+'nm','-nC',str(out/'program.elf')],text=True)
assert not subprocess.check_output([tool+'nm','-u',str(out/'program.elf')],text=True).strip()
assert 'GlobalChain::FullOffloadEog(unsigned int)' in symbols
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
for name,address in (('_start',0x6000),('scalar_entry',0x40000),('scalar_fault_park',0x60000),('q38_global_chain_stopped',0x60004)):
    assert re.search('^'+format(address,'08x')+' T '+name+'$',symbols,re.M),name
frames=[r.split('\t') for r in (out/'program.su').read_text().splitlines()]
assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in frames)
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(subprocess.check_output([tool+'objdump','-d',str(out/'program.elf')],text=True))
subprocess.run([sys.executable,str(here/'tests/pioneer_q38_full_offload_eog_isa_test.py'),str(build),str(out)],check=True)
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
for name in ('boot','text','terminal','conv'):
    data=(out/(name+'.bin')).read_bytes();print(name,len(data),hashlib.sha256(data).hexdigest(),flush=True)
if ffn_shared_reuse:
    if not inspect_candidate:
        for name,size,digest in PINS_FFN_SHARED_INPUT_REUSE:
            data=(out/name).read_bytes()
            assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
if inspect_candidate:
    print('UNQUALIFIED INSPECT candidate only; hashes not pinned; no runtime, install or physical/numerical grant',flush=True)
else:
    print('PASS BUILD_ONLY unchanged complete G3 ROOT control FIRST;new on-card all5-EOG predicate;whole4-section image fits original journal/boot+STOP exact/static frames<=2048;no runtime or physical/numerical grant',flush=True)
