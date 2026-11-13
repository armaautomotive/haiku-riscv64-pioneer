#!/usr/bin/env python3
"""Link and simulate inspection-only recurrent controller helpers."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv) not in (4,5) or (len(sys.argv)==5 and sys.argv[4]!='--small') or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_SDK ABS_EXISTING_ROOT ABS_NEW_RECURRENT_OBJECT [--small]')
sdk,root,recurrent=map(Path,sys.argv[1:4]);small=len(sys.argv)==5;here=Path(__file__).resolve().parent
assert sdk.is_absolute() and root.is_absolute() and recurrent.is_absolute()
assert sdk.is_dir() and root.is_dir() and recurrent.is_file()
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-'
old=root/'qualified-components'
with tempfile.TemporaryDirectory(prefix='q38-recurrent-actual-root-',dir='/private/tmp') as directory:
    tmp=Path(directory);raw=tmp/'raw.o';combined=tmp/'combined.o';image=tmp/'program.elf';binary=tmp/'test'
    subprocess.run([str(tool)+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
                    '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
                    '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
                    str(recurrent),str(old/'dense.o'),str(old/'unchanged-feedback/root_conv_sections.o'),'-o',str(raw)],check=True)
    subprocess.run([str(tool)+'objcopy',*['--strip-symbol='+n for n in
        ('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')],
        str(raw),str(combined)],check=True)
    subprocess.run([str(tool)+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',str(here/'pioneer_q38_recurrent_controller.ld'),
                    str(root/'start.o'),str(root/'program.o'),str(combined),str(old/'unchanged-feedback/feedback.o'),
                    '-o',str(image)],check=True)
    assert not subprocess.check_output([str(tool)+'nm','-u',str(image)]).strip()
    for section in ('bf16','text','terminal','conv'):
        subprocess.run([str(tool)+'objcopy','-O','binary','-j','.'+section,str(image),str(tmp/(section+'.bin'))],check=True)
    (tmp/'program.demangled').write_bytes(subprocess.check_output([str(tool)+'nm','-nC',str(image)]))
    flags=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
           'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_SIMULATION',
           'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT',
           'PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION','PIONEER_Q38_QGWE_BATCH_DENSE_PLAN',
           'PIONEER_Q38_QGWF_DUAL_BATCH_PLAN')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation','-ffp-contract=off',
                    '-fsanitize=address,undefined',*['-D'+name+'=1' for name in flags],
                    *(['-DPIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION=1'] if small else []),
                    str(here/'pioneer_q38_recurrent_actual_root_test.cpp'),'-o',str(binary)],check=True)
    symbols=subprocess.check_output([str(tool)+'nm','-C',str(recurrent)],text=True)
    scoped='Projection::BfDenseScopedSeal(' in symbols
    assert not (small and scoped)
    subprocess.run([str(binary),str(tmp),*(['--expect-scoped'] if scoped else ['--small'] if small else [])],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
