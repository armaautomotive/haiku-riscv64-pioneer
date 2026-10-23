#!/usr/bin/env python3
"""Actual new native factory/source/ownership/END compile;all old byte controls."""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys

here = pathlib.Path(__file__).resolve().parent
if any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != '--validation'):
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output [--validation]')
validation = len(sys.argv) == 4
sdk, output = map(pathlib.Path, sys.argv[1:3])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
subprocess.run([sys.executable, str(here.parent / 'build_qwen38_projection_validation_native.py'), str(sdk), str(output / 'controls')], check=True, env=clean)
assert hashlib.sha256((output / 'controls/tenstorrent_pci_probe-driver55-projection-tag42').read_bytes()).hexdigest() == 'a2bcfb34baeba0bdca4e029164901c874c27ee7c3494be62912711e7a0c164e9'
commands = [shlex.split(line[2:]) for line in (output / 'controls/controls/old-tag38-build.trace').read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [args for args in commands if '-c' in args and args[args.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in args]
assert len(compiles) == 1
args = [arg for arg in compiles[0] if not arg.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not arg.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tag = [i for i, arg in enumerate(args) if arg.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tag) == 1
args[tag[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102300010043ULL'
args[args.index('-c') + 1] = str(here / 'pioneer_q38_resident_decoder_native_full_compile.cpp')
obj = output / 'resident-decoder-NATIVE-COMPILE-ONLY.o'
args[args.index('-o') + 1] = str(obj)
subprocess.run(args, check=True, env=clean)
assert hashlib.sha256(obj.read_bytes()).hexdigest() == '3445a61f0473616a4aa2c352e730df39b6509fc34e78acbe3e2f2d72827bc3cb', 'bare tag43/control MUST remain byte-identical'
if validation:
    args[tag[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102300010044ULL'
    args += ['-DPIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE=1']
    obj = output / 'resident-decoder-DEBUG44-COMPILE-ONLY.o'
    args[args.index('-o') + 1] = str(obj)
    subprocess.run(args, check=True, env=clean)
def frames(path):
    return {re.sub(r':\d+:\d+:', ':', line.split('\t')[0]): int(line.split('\t')[1])
            for line in path.read_text().splitlines() if len(line.split('\t')) == 3}
current = frames(obj.with_suffix('.su'))
previous = frames(output / 'controls/controls/old-execution-CONTROL.su')
assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in (
    'PioneerQ38ResidentDecoder', 'PioneerQ38ConvHistory', 'pioneer_decoder_native_', 'InitializeQ38ConvHistoryModel',
    'RunQ38ResidentDecoderModel', 'CloseQ38ResidentDecoderModel', 'DecoderCallScopeMatches', 'ResidentDecoderScope',
    'Q38ResidentDecoderValidationControl', 'PioneerQ38ResidentDecoderOwner::ValidationReadback'))]
assert new and max(new) <= 2048
print('PASS ACTUAL ' + ('DEBUG44 fixed40page control/routing/readback plus baretag43 EXACT;' if validation else 'tag43;')
      + ' complete native history/retained arithmetic factories/source/context-cookie-team-thread-clock/ALLhosts/retention/END compile maxNewFrame=' + str(max(new))
      + ';all oversized legacy frames <=BYTE-IDENTICAL control;oldtag38/guard/default43/private2d12/stash6121/tag42a2bc EXACT;NO_NEW_LINKED_DRIVER_INSTALL_OR_DEVICE')
print(hashlib.sha256(obj.read_bytes()).hexdigest() + '  ' + str(obj))
