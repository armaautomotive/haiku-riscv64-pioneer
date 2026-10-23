#!/usr/bin/env python3
"""Build ONLY separate tag42/namespace13 QPS1 DEBUG qualification.

Actual complete source/owners/fixed readback/control/END. Verified old
driver/guard/default43/private-execution controls FIRST. No dirty library
read/build,install,device I/O,firmware or Assistant/model mutation.
"""
import hashlib
import os
import pathlib
import shlex
import subprocess
import sys

if any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
if len(sys.argv) != 3:
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output')
here = pathlib.Path(__file__).resolve().parent
sdk, output = map(pathlib.Path, sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
subprocess.run([sys.executable, str(here / 'tests/pioneer_q38_projection_memory_native_full_compile.py'),
                str(sdk), str(output / 'controls'), '--result-stashes'], check=True, env=clean)
assert hashlib.sha256((output / 'controls/new-stash-COMPILE-ONLY.o').read_bytes()).hexdigest() == '6121b065785fb8732f3a00770e299903663fb600baf13e385c97ac78d543de72'
commands = [shlex.split(line[2:]) for line in (output / 'controls/old-tag38-build.trace').read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
args = compiles[0]
old_object = args[args.index('-o') + 1]
args = [a for a in args if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102200010042ULL'
obj = output / 'projection-validation-tag42.o'
args[args.index('-o') + 1] = str(obj)
args += ['-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1',
         '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1', '-DPIONEER_Q38_PROJECTION_RESULT_STASH=1',
         '-DPIONEER_Q38_PROJECTION_VALIDATION_NATIVE=1']
subprocess.run(args, check=True, env=clean)
frames = []
readback = []
for line in obj.with_suffix('.su').read_text().splitlines():
    if len(line.split('\t')) != 3:
        continue
    size = int(line.split('\t')[1])
    if any(name in line for name in ('PioneerQ38EmbeddingController.h:', 'PioneerQ38ProjectionZeroController.h:',
            'PioneerQ38LearnedRmsController.h:', 'PioneerQ38ProjectionGraphOwner.h:', 'PioneerQ38ProjectionGraphMemory.h:',
            'Q38ProjectionValidationControl', 'BindQ38EmbeddingModel', 'InitializeQ38EmbeddingModel', 'RunQ38EmbeddingModel',
            'RunQ38LearnedRmsModel', 'Q38ProjectionExecution', 'PioneerSessionIO::Projection', 'PioneerSessionIO::Embedding',
            'PioneerSessionIO::LearnedRms')):
        frames.append(size)
    if 'PioneerQ38ProjectionGraphOwner::ValidationReadback' in line:
        readback.append(size)
assert frames and readback and max(frames) <= 2048
print('PASS actual tag42 new kernel frames max=' + str(max(frames)) + '/readback=' + str(max(readback)))
links = [a for a in commands if '-shared' in a and old_object in a]
assert len(links) == 1
args = [str(obj) if a == old_object else a for a in links[0]]
candidate = output / 'tenstorrent_pci_probe-driver55-projection-tag42'
args[args.index('-o') + 1] = str(candidate)
subprocess.run(args, check=True, env=clean)
print(hashlib.sha256(candidate.read_bytes()).hexdigest() + '  ' + str(candidate))
print('BUILT_ONLY exact tag42/namespace13,18fixed poisoned result pages/actual owner after ALL120holds BEFORE restore;NO_INSTALL_DEVICE_EXECUTION_WORK_OR_MODEL_SPEED_CLAIM')
