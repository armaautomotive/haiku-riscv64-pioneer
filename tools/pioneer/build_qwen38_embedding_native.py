#!/usr/bin/env python3
"""Build ONLY exact isolated tag39 embedding arithmetic qualification.

Derive actual compiler/linker argv from verified old controls,never a shell
eval or dirty library build. No install,device IO,firmware or Assistant edit.
"""
import hashlib
import os
import pathlib
import shlex
import subprocess
import sys

if any(key.startswith('PIONEER_') and value != '0' for key, value in os.environ.items()):
    raise SystemExit(2)
if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != '--validation'):
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output [--validation]')
validation = len(sys.argv) == 4
tag = '40' if validation else '39'
here = pathlib.Path(__file__).resolve().parent
sdk, output = map(pathlib.Path, sys.argv[1:3])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
subprocess.run([sys.executable, str(here / 'tests/pioneer_q38_embedding_native_full_compile.py'), str(sdk), str(output / 'controls')],
               check=True, env=clean)
commands = [shlex.split(line[2:]) for line in (output / 'controls/old-tag38.trace').read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compile_commands = [args for args in commands if '-c' in args and args[args.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in args]
assert len(compile_commands) == 1
args = compile_commands[0]
old_object = args[args.index('-o') + 1]
args = [arg for arg in args if not arg.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not arg.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, arg in enumerate(args) if arg.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x20261022000100' + tag + 'ULL'
args[args.index('-o') + 1] = str(output / ('embedding-tag' + tag + '.o'))
args += ['-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_QUALIFICATION=1']
if validation:
    args += ['-DPIONEER_Q38_EMBEDDING_VALIDATION_NATIVE=1']
subprocess.run(args, check=True, env=clean)
frames = []
for line in (output / ('embedding-tag' + tag + '.su')).read_text().splitlines():
    if len(line.split('\t')) == 3 and any(name in line for name in (
            'PioneerQ38EmbeddingController.h:', 'PioneerQ38ProjectionZeroController.h:', 'Q38EmbeddingControl',
            'BindQ38EmbeddingModel', 'InitializeQ38EmbeddingModel', 'RunQ38EmbeddingModel',
            'PioneerSessionIO::Embedding', 'PioneerSessionIO::ProjectionZero')):
        frames.append(int(line.split('\t')[1]))
assert frames and max(frames) <= 2048
print('PASS linked qualification actual kernel stack frames,max=' + str(max(frames)))
links = [args for args in commands if '-shared' in args and old_object in args]
assert len(links) == 1
args = [str(output / ('embedding-tag' + tag + '.o')) if arg == old_object else arg for arg in links[0]]
candidate = output / ('tenstorrent_pci_probe-driver55-embedding-tag' + tag)
args[args.index('-o') + 1] = str(candidate)
subprocess.run(args, check=True, env=clean)
print(hashlib.sha256(candidate.read_bytes()).hexdigest() + '  ' + str(candidate))
print('BUILT_ONLY tag' + tag + '/namespace' + ('11 DEBUG GDDR NUMERIC VALIDATION' if validation else '10 no-reference producer')
      + ';NO install/device execution/Assistant/model-speed claim')
