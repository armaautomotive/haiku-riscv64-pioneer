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
          (parent / 'program.o', ('5eebafe66f69dbee63e6d48d476d933ba71eaab9ee2f1a58d2e156b987832f87',
                                 'c9bc9bfd38ccc7000bbb2f7482dd25148dd3013b6db8d435b5da7e0718242037')),
          (components / 'dense.o', 'b1960b90ba1b01b2e43b5889aeaab8a69f5db2f48cd103a340f6480941dfb9f2'),
          (components / 'unchanged-feedback/root_conv_sections.o', 'e770a9bde85baadaf9239b7e40985af6415b99650866fad745fe4691a3196395'),
          (components / 'unchanged-feedback/feedback.o', '8c2626d34501e53dc25368cb8f67c48c9de778fec5d880e21901034795c083f2'),
          (recurrent, ('c3051b325d8c5a4bb6cb9b5a1e02a5bad2f8fec8f28bbfbb0f1dc12f9bfa106c',
                       'a240f91c29ab537cb3d8abadb6a05ca09928c11113ce9905f5d612e74365b61c',
                       'de452c986ea3702e6652e1289eeb0a6e4f4baf2c0b8918a45735a8874b9acc9f',
                       'b283c169543b5293ae21df8d368bbeebdbf96d8b8b9d6c7ebfe96595d8ebdcb5',
                       '27ca02a95bcd776173b1679a7d8cbd3f0c48b352a45628185f06bdf87e65ebbc',
                       'e5216114d14a8757fd12221229561978b1ef4ca87859cc36397c778187179627',
                       '491f8c30b291ca8c7ca9ac1f5fea71e985a166f8bb420b85ca843353520db72e',
                       'eb8908e9f8630e544f59fb94ce190c2896fc638f76f7e56db8e21bd49d38c0f4',
                       '7a2f1f4d4b15d1caf888e2b23c1090aaef1ae8d19bef2ca9f3851421ec70b54d',
                       'abf2e71dcde462462061831c8f2f08fc3c2321ba50c721a2e6c78783bd0a0e48',
                       '966bd778c80f118e27e8bd26e7dbf54aa6183d94ae1aabcdc3f2501030ec1e79')))
for path, expected in inputs:
    allowed = (expected,) if isinstance(expected, str) else expected
    assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest() in allowed, path
new_recurrent = hashlib.sha256(recurrent.read_bytes()).hexdigest() in ('e5216114d14a8757fd12221229561978b1ef4ca87859cc36397c778187179627',
    '491f8c30b291ca8c7ca9ac1f5fea71e985a166f8bb420b85ca843353520db72e',
    'eb8908e9f8630e544f59fb94ce190c2896fc638f76f7e56db8e21bd49d38c0f4',
    '7a2f1f4d4b15d1caf888e2b23c1090aaef1ae8d19bef2ca9f3851421ec70b54d',
    'abf2e71dcde462462061831c8f2f08fc3c2321ba50c721a2e6c78783bd0a0e48',
    '966bd778c80f118e27e8bd26e7dbf54aa6183d94ae1aabcdc3f2501030ec1e79')
assert (hashlib.sha256((parent/'program.o').read_bytes()).hexdigest() == 'c9bc9bfd38ccc7000bbb2f7482dd25148dd3013b6db8d435b5da7e0718242037') == new_recurrent
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
if new_recurrent:
    assert symbols.count('Projection::BfDenseSeal(')==2
    assert symbols.count('PioneerQ38RecurrentQ8MatrixRootJobs::Build<')==1
else:
    assert symbols.count('Projection::BfDenseScopedSeal(')==1
if hashlib.sha256(recurrent.read_bytes()).hexdigest() in ('a240f91c29ab537cb3d8abadb6a05ca09928c11113ce9905f5d612e74365b61c',
                                                         'de452c986ea3702e6652e1289eeb0a6e4f4baf2c0b8918a45735a8874b9acc9f',
                                                         'b283c169543b5293ae21df8d368bbeebdbf96d8b8b9d6c7ebfe96595d8ebdcb5',
                                                         '27ca02a95bcd776173b1679a7d8cbd3f0c48b352a45628185f06bdf87e65ebbc',
                                                         'e5216114d14a8757fd12221229561978b1ef4ca87859cc36397c778187179627',
                                                         '491f8c30b291ca8c7ca9ac1f5fea71e985a166f8bb420b85ca843353520db72e',
                                                         'eb8908e9f8630e544f59fb94ce190c2896fc638f76f7e56db8e21bd49d38c0f4',
                                                         '7a2f1f4d4b15d1caf888e2b23c1090aaef1ae8d19bef2ca9f3851421ec70b54d',
                                                         'abf2e71dcde462462061831c8f2f08fc3c2321ba50c721a2e6c78783bd0a0e48',
                                                         '966bd778c80f118e27e8bd26e7dbf54aa6183d94ae1aabcdc3f2501030ec1e79'):
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
