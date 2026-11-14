#!/usr/bin/env python3
"""Compile experimental recurrent controller component only; never install or run."""
import os
from pathlib import Path
import re
import subprocess
import sys
if len(sys.argv) not in (3,4,6) or (len(sys.argv)>=4 and sys.argv[3] not in ('--baseline','--scoped-seal','--small-result','--small-reuse','--gate-reuse','--both-reuse','--all-reuse','--ffn-reuse')) or (len(sys.argv)==6 and (sys.argv[3]!='--baseline' or sys.argv[4]!='--source-root')) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_BUILD NEW_ABS_OBJECT [--scoped-seal | --small-result | --small-reuse | --gate-reuse | --both-reuse | --all-reuse | --ffn-reuse | --baseline [--source-root ABS_ROOT]]')
build,out=map(Path,sys.argv[1:3]);baseline=len(sys.argv)>=4 and sys.argv[3]=='--baseline';scoped=len(sys.argv)==4 and sys.argv[3]=='--scoped-seal';ffnReuse=len(sys.argv)==4 and sys.argv[3]=='--ffn-reuse';allReuse=len(sys.argv)==4 and sys.argv[3] in ('--all-reuse','--ffn-reuse');reuse=len(sys.argv)==4 and sys.argv[3] in ('--small-reuse','--both-reuse','--all-reuse','--ffn-reuse');gateReuse=len(sys.argv)==4 and sys.argv[3] in ('--gate-reuse','--both-reuse','--all-reuse','--ffn-reuse');small=len(sys.argv)==4 and (sys.argv[3]=='--small-result' or reuse or gateReuse);here=Path(__file__).resolve().parent
source=Path(sys.argv[5])/'tools/pioneer' if len(sys.argv)==6 else here.parent
if not source.is_absolute() or not source.is_dir():raise SystemExit('invalid source root')
if not build.is_absolute() or not build.is_dir() or not out.is_absolute() or out.exists() or out.is_symlink():
    raise SystemExit('invalid build or output')
tool=build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-'
roles=('PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','PIONEER_Q38_SIGMOID_PRECISION_IMAGE',
 'PIONEER_Q38_QGWD_ROOT_IMAGE','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_QGWE_BATCH_DENSE_ROOT','PIONEER_Q38_QGWF_DUAL_BATCH_ROOT',
 'PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU','PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP',
 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
 'PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION','PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT',
 'PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION','PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION',
 'PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS','PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL',
 'PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS','PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN',
 'PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE','PIONEER_Q38_LAYER_COMPONENT_RECURRENT',
 'PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL',
 'PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT','PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION')
flags=['-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
 '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections',
 '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax',
 '-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11']
subprocess.run([str(tool)+'g++',*flags,*['-D'+r+'=1' for r in (roles[:-1] if baseline else roles)],
                *(['-DPIONEER_Q38_RECURRENT_SCOPED_SEAL_EXPERIMENT=1'] if scoped else []),'-c',
                *(['-DPIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION=1'] if small else []),
                *(['-DPIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION=1'] if reuse else []),
                *(['-DPIONEER_Q38_RECURRENT_GATE_INPUT_REUSE_INSPECTION=1'] if gateReuse else []),
                *(['-DPIONEER_Q38_QKV_ALPHA_INPUT_REUSE_INSPECTION=1'] if allReuse else []),
                *(['-DPIONEER_Q38_FFN_INPUT_REUSE_INSPECTION=1'] if ffnReuse else []),
                str(source/'blackhole_qwen38_return_recurrent_layer.cpp'),'-o',str(out)],check=True)
undefined=subprocess.check_output([str(tool)+'nm','-u',str(out)],text=True)
assert {line.split()[-1] for line in undefined.splitlines()}=={
 'mixed_recurrent_block_stopped','mixed_recurrent_block_wait','q38_mixed_recurrent_root_conv','scalar_fault_park'}
disassembly=subprocess.check_output([str(tool)+'objdump','-d',str(out)],text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*)\b',disassembly),'unsupported controller arithmetic'
root_code=subprocess.check_output([str(tool)+'objdump','-d','-j','.bf16.root',str(out)],text=True)
assert not re.search(r'\bsb\b',root_code),'controller BF16 path must not use byte stores'
frames=out.with_suffix('.su').read_text().splitlines()
schedule=[row.split('\t') for row in frames if 'PioneerQ38RecurrentQ8MatrixRootJobs::Build' in row]
assert not baseline or not schedule
assert baseline or (len(schedule)==1 and schedule[0][-1]=='static' and int(schedule[0][-2])<=2048),schedule
print('PASS recurrent controller component cross-build; scheduler frame',schedule[0][-2] if schedule else 'baseline', 'small-result',int(small),
      '; NO LINK, DEVICE, OWNERSHIP, MODEL OR SPEED CLAIM')
