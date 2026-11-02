#!/usr/bin/env python3
"""OFFLINE standalone wide-delta instruction image;NEVER install/select/run a card."""
import hashlib
import os
import pathlib
import subprocess
import sys

PINS={'boot.bin':(4,'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
      'code.bin':(3408,'1ab17ccc83b072f6064fb13885ce4e3c4bdfc48ea9814440bf385a5694937979'),
      'program.elf':(12588,'abd969919d1952e79ed127b37e35e318c78a2cbc8c473a3b808617f807219c83')}
assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==4
assert not inspect or sys.argv[3]=='--inspect-candidate'
sdk,out=map(pathlib.Path,sys.argv[1:3])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
here=pathlib.Path(__file__).resolve().parent
cross=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
assert pathlib.Path(str(cross)+'-g++').is_file()
out.mkdir(mode=0o700)
compile_args=[str(cross)+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding',
 '-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off',
 '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage',
 '-c',str(here/'blackhole_qwen38_wide_delta.cpp'),'-o',str(out/'program.o')]
subprocess.run(compile_args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(compile_args)+'\n')
subprocess.run([str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',str(here/'blackhole_qwen38_wide_delta_start.S'),'-o',str(out/'start.o')],check=True)
subprocess.run([str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'blackhole_qwen38_wide_delta.ld'),str(out/'start.o'),str(out/'program.o'),'-Map',str(out/'program.map'),'-o',str(out/'program.elf')],check=True)
for section,name in (('.text','code.bin'),('.boot','boot.bin')):
 subprocess.run([str(cross)+'-objcopy','-O','binary','-j',section,str(out/'program.elf'),str(out/name)],check=True)
for option,name in (('-n','program.symbols'),('-d','program.disassembly')):
 tool='-nm' if option=='-n' else '-objdump'
 (out/name).write_bytes(subprocess.check_output([str(cross)+tool,option,str(out/'program.elf')]))
rows=[r.split('\t') for r in (out/'program.su').read_text().splitlines()]
assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=512 for r in rows)
assert not subprocess.check_output([str(cross)+'-nm','-u',str(out/'program.elf')]).strip()
for name in ('boot.bin','code.bin','program.elf'):
 data=(out/name).read_bytes();digest=hashlib.sha256(data).hexdigest()
 if not inspect:assert (len(data),digest)==PINS[name],(name,len(data),digest)
 print(digest,len(data),name,flush=True)
print('OFFLINE_BUILD_ONLY NEW local wide-delta/13 bounded imports/new256B delta scratch/512x16 FP32 Dst/ONLY8LRegs;NOT native-selected,ownership,arithmetic acceptance,physical,Work or TPS',flush=True)
