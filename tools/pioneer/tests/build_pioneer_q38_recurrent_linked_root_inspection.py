#!/usr/bin/env python3
"""Link a persistent recurrent ROOT inspection image; never install or run it."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) != 5 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: ABS_SDK ABS_PARENT ABS_RECURRENT_OBJECT NEW_ABS_OUTPUT')
sdk, parent, recurrent, output = map(Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk, parent, recurrent, output))
assert sdk.is_dir() and parent.is_dir() and recurrent.is_file() and not recurrent.is_symlink()
assert not output.exists() and not output.is_symlink()
here = Path(__file__).resolve().parent
components = parent / 'qualified-components'
inputs = ((parent / 'start.o', '20ec8f9bb5690b079ad0dbb99dc18a9c89b15d940eef09a6dcf271c535383b46'),
          (parent / 'program.o', '5eebafe66f69dbee63e6d48d476d933ba71eaab9ee2f1a58d2e156b987832f87'),
          (components / 'dense.o', 'b1960b90ba1b01b2e43b5889aeaab8a69f5db2f48cd103a340f6480941dfb9f2'),
          (components / 'unchanged-feedback/root_conv_sections.o', 'e770a9bde85baadaf9239b7e40985af6415b99650866fad745fe4691a3196395'),
          (components / 'unchanged-feedback/feedback.o', '8c2626d34501e53dc25368cb8f67c48c9de778fec5d880e21901034795c083f2'),
          (recurrent, ('c3051b325d8c5a4bb6cb9b5a1e02a5bad2f8fec8f28bbfbb0f1dc12f9bfa106c',
                       'a240f91c29ab537cb3d8abadb6a05ca09928c11113ce9905f5d612e74365b61c',
                       'de452c986ea3702e6652e1289eeb0a6e4f4baf2c0b8918a45735a8874b9acc9f')))
for path, expected in inputs:
    allowed = (expected,) if isinstance(expected, str) else expected
    assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest() in allowed, path
output.mkdir(mode=0o700)
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
raw, combined, image = output / 'raw.o', output / 'combined.o', output / 'program.elf'
def run(*args): subprocess.run([str(a) for a in args], check=True)
run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
    '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
    '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
    recurrent,components/'dense.o',components/'unchanged-feedback/root_conv_sections.o','-o',raw)
run(tool+'objcopy',*['--strip-symbol='+n for n in
    ('mixed_recurrent_block_wait','mixed_recurrent_block_stopped','mixed_live_block_wait','mixed_live_block_stopped')],raw,combined)
run(tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here/'pioneer_q38_recurrent_controller.ld',
    parent/'start.o',parent/'program.o',combined,components/'unchanged-feedback/feedback.o','-o',image)
assert not subprocess.check_output([tool+'nm','-u',str(image)]).strip()
symbols=subprocess.check_output([tool+'nm','-nC',str(image)],text=True)
assert symbols.count('Projection::BfDenseScopedSeal(')==1
if hashlib.sha256(recurrent.read_bytes()).hexdigest() in ('a240f91c29ab537cb3d8abadb6a05ca09928c11113ce9905f5d612e74365b61c',
                                                         'de452c986ea3702e6652e1289eeb0a6e4f4baf2c0b8918a45735a8874b9acc9f'):
    assert symbols.count('PioneerQ38HybridMatrixReceipt::Matrix(') == 1
    assert symbols.count('PioneerQ38RecurrentHybridMatrixReceipt::Matrix(') == 1
(output/'program.demangled').write_text(symbols)
(output/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',str(image)]))
for section in ('boot','bf16','text','terminal','conv'):
    target=output/(section+'.bin')
    run(tool+'objcopy','-O','binary','-j','.'+section,image,target)
    data=target.read_bytes()
    print(section,len(data),hashlib.sha256(data).hexdigest(),flush=True)
print('ROOT_INSPECTION_LINK_ONLY; NO DEVICE, NATIVE, WORK OR PERFORMANCE AUTHORITY')
