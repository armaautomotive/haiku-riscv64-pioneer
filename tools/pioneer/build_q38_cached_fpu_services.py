#!/usr/bin/env python3
"""Build returning Q4/cache and live-FP32 BF16 services.OFFBOARD only."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
for name,role in (('cached_fpu_matrix','PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE'),('bf16_producer','PIONEER_Q38_BF16_PRODUCER_IMAGE')):
 folder=out/name;folder.mkdir(mode=0o700);source=here/('blackhole_q38_'+name+'.cpp')
 compile=[tool+'g++','-D'+role+'=1','-std=c++17','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
  '-fno-tree-loop-distribute-patterns','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
  '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0',
  '-mno-relax','-march=rv32i','-mabi=ilp32','-ffixed-s11','-fstack-usage','-c',str(source),'-o',str(folder/'program.o')]
 commands=[compile,[tool+'as','-march=rv32i','-mabi=ilp32','-mno-relax',str(here/('blackhole_q38_'+name+'_start.S')),'-o',str(folder/'start.o')],
  [tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/('blackhole_q38_'+name+'.ld')),str(folder/'start.o'),str(folder/'program.o'),'-Map',str(folder/'program.map'),'-o',str(folder/'program.elf')]]
 for section in ('boot','text'):commands.append([tool+'objcopy','-O','binary','-j','.'+section,str(folder/'program.elf'),str(folder/(section+'.bin'))])
 for command in commands:subprocess.run(command,cwd=folder,check=True)
 if name=='cached_fpu_matrix':
  returning=[arg.replace(str(folder/'program.o'),str(folder/'return-only.o')) for arg in compile]
  returning.insert(1,'-DPIONEER_Q38_CACHED_FPU_RETURN_ONLY=1')
  subprocess.run(returning,cwd=folder,check=True);commands.append(returning)
  unresolved=subprocess.check_output([tool+'nm','-u',str(folder/'return-only.o')],text=True)
  assert set(row.split()[-1] for row in unresolved.splitlines())<={'cached_fpu_fault','memset'}
 cases=[('-U'+role,),('-U'+role,'-D'+role+'=0'),('-U'+role,'-D'+role+'=2'),('-ffast-math',)]
 macros=('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
  'PIONEER_DRIVER_BUILD_TAG','PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC','PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE',
  'PIONEER_Q38_BF16_PRODUCER_IMAGE' if name=='cached_fpu_matrix' else 'PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE')
 for macro in macros:
  for value in (0,1):cases.append(('-D'+macro+'='+str(value),))
 for extra in cases:
  assert compile[-4]=='-c'
  result=subprocess.run(compile[:-4]+list(extra)+['-E',str(source)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
  assert result.returncode and b'error:' in result.stderr,(name,extra)
 (folder/'role-refusals.txt').write_text(''.join('PASS '+' '.join(extra)+'\n' for extra in cases))
 assert not subprocess.check_output([tool+'nm','-u',str(folder/'program.elf')]).strip()
 symbols=subprocess.check_output([tool+'nm','-n',str(folder/'program.elf')],text=True)
 assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
 (folder/'program.symbols').write_text(symbols)
 (folder/'program.disassembly').write_bytes(subprocess.check_output([tool+'objdump','-d',str(folder/'program.elf')]))
 (folder/'compile-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
 frames=[row.split('\t') for row in (folder/'program.su').read_text().splitlines()]
 assert frames and all(len(row)==3 and row[2]=='static' and int(row[1])<=4096 for row in frames)
 for leaf in ('boot.bin','text.bin','program.elf'):
  data=(folder/leaf).read_bytes()
  assert len(data)==4 if leaf=='boot.bin' else leaf!='text.bin' or (0<len(data)<=16384 and not len(data)%4)
  print('CACHED_FPU_SERVICE_BUILD',name,leaf,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('PASS BUILD_ONLY separate16KiB service placements,not104 ROM changes;NO execution/native/Work/owner/model-quality/speed authority')
