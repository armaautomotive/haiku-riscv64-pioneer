#!/usr/bin/env python3
"""Build separate complete full-card EOG ROOT;never open a device/install/run."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

args=sys.argv[1:]
inspect_candidate='--inspect-candidate' in args
if inspect_candidate:
    if args.count('--inspect-candidate')!=1:raise SystemExit(2)
    args.remove('--inspect-candidate')
compact_inspection='--compact-inspection' in args
if compact_inspection:
    if args.count('--compact-inspection')!=1:raise SystemExit(2)
    args.remove('--compact-inspection')
PINS_FFN_SHARED_INPUT_REUSE=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
  ('text.bin',131040,'641babd1746c1f89aef05b30057808b9887f235a3c757006472c33c8f69ed9f0'),
  ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
  ('conv.bin',32708,'54d2b80ea35c20d37e523764024307e609b11160c4b5499655ce64fd266964bf'),
  ('program.elf',190500,'ed959881d2d85d2f135a5236a6abf075dcc6e5871d73d7233e7684aac7125110'))
PINS_COMPACT_INSPECTION=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
  ('text.bin',120320,'b44ed20a498394ffcf0c30e29ae9db4d19985f687be537c896f76d73e33180d4'),
  ('terminal.bin',8,'6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
  ('conv.bin',29500,'3516ece88ba1e926c158fdd02865e18b8c62b1e52835e94384aa35730fdc68b0'),
  ('program.elf',184348,'c7037c9d2a52db5e4a0d6a7b3234b0d2ee100ddf31e3757e00610f848c9a12a9'))
ffn_shared_reuse='--ffn-shared-input-reuse' in args
if ffn_shared_reuse:
    if args.count('--ffn-shared-input-reuse')!=1:raise SystemExit(2)
    args.remove('--ffn-shared-input-reuse')
assert __debug__ and len(args)==2
assert not compact_inspection or inspect_candidate,'compact controller/components are inspection-only'
assert not (compact_inspection and ffn_shared_reuse),'compact inspection profile cannot combine with FFN reuse'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,args)
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
# Some locally staged Haiku SDKs have the devel package headers but an empty
# compiler sysroot include directory.  Make the SDK's public headers visible
# to every nested compiler invocation without overriding an existing CPATH.
headers=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
if (headers/'posix/stdint.h').is_file() and (headers/'config/types.h').is_file():
    inherited=os.environ.get('CPATH','')
    sdk_cpath=os.pathsep.join((str(headers),str(headers/'posix')))
    os.environ['CPATH']=sdk_cpath+(os.pathsep+inherited if inherited else '')
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector',
 '-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
commands=[]
def run(command):
    commands.append(command);subprocess.run(command,check=True)
base=out/'unchanged-pushed';base.mkdir(mode=0o700)
components=base/'qualified-components'
run([sys.executable,str(here/'build_qwen38_global_token_components.py'),str(build),str(components),
     '--f32-hidden','--unroll-dense-seals','--boundary-seals','--unroll-quiet','--worker-local-source-guards',
     '--precise-activation','--single-closed-audit','--worker-sealed-admission','--pushed-completion',
     *(('--inspect-candidate',) if inspect_candidate else ()),
     *(('--compact-inspection',) if compact_inspection else ()),
     *(('--ffn-shared-input-reuse',) if ffn_shared_reuse else ())])
run([tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',str(here/'blackhole_qwen38_global_chain_start.S'),'-o',str(base/'start.o')])
root_flags=[('-Os' if compact_inspection and flag=='-O2' else flag) for flag in flags]
run([tool+'g++',*root_flags,*(['-include',str(here/'PioneerQ38FreestandingMemcpy.h')] if compact_inspection else []),
     '-DPIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE=1','-DPIONEER_Q38_GLOBAL_CHAIN_FEEDBACK=1',
     '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1','-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
     '-DPIONEER_Q38_TOKEN_CANCEL_BOUNDARY=1','-DPIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE=1',
     '-c',str(here/'blackhole_qwen38_token_root.cpp'),'-o',str(out/'program.o')])
run([tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'blackhole_qwen38_global_chain.ld'),
     str(base/'start.o'),str(out/'program.o'),str(components/'token-components-NOBOOT.o'),
     str(components/'unchanged-feedback/feedback.o'),'-Map',str(out/'program.map'),'-o',str(out/'program.elf')])
for section in ('boot','text','terminal','conv'):
    run([tool+'objcopy','-O','binary','-j','.'+section,str(out/'program.elf'),str(out/(section+'.bin'))])
assert hashlib.sha256((out/'boot.bin').read_bytes()).hexdigest()=='32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'
assert hashlib.sha256((out/'terminal.bin').read_bytes()).hexdigest()=='6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'
assert 0<len((out/'text.bin').read_bytes())<=0x20000 and 0<len((out/'conv.bin').read_bytes())<=0x8000
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
if compact_inspection:
    for name,size,digest in PINS_COMPACT_INSPECTION:
        data=(out/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,name
    print('PASS reproducible compact inspection image; fixed regions fit; still UNQUALIFIED for runtime/install/physical inference',flush=True)
if inspect_candidate:
    print('UNQUALIFIED INSPECT candidate only; compact-profile hashes pinned where selected; no runtime, install or physical/numerical grant',flush=True)
else:
    print('PASS BUILD_ONLY unchanged complete G3 ROOT control FIRST;new on-card all5-EOG predicate;whole4-section image fits original journal/boot+STOP exact/static frames<=2048;no runtime or physical/numerical grant',flush=True)
