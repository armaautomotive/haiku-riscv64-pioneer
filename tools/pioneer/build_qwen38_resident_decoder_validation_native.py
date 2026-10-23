#!/usr/bin/env python3
"""Build ONLY separate DEBUG44 fixed first-token/layer0 convolution driver.

Complete real source,not the compile witness. First reproduce ALL old
byte controls and baretag43 and compile the actual DEBUG path. No device,
install,dirty library,Assistant,firmware,model or boot actions.
"""
import hashlib
import os
import pathlib
import re
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
subprocess.run([sys.executable, str(here / 'tests/pioneer_q38_resident_decoder_native_full_compile.py'),
                str(sdk), str(output / 'controls'), '--validation'], check=True, env=clean)
assert hashlib.sha256((output / 'controls/resident-decoder-NATIVE-COMPILE-ONLY.o').read_bytes()).hexdigest() == '3445a61f0473616a4aa2c352e730df39b6509fc34e78acbe3e2f2d72827bc3cb'
commands = [shlex.split(line[2:]) for line in (output / 'controls/controls/controls/old-tag38-build.trace').read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
old_object = compiles[0][compiles[0].index('-o') + 1]
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102300010044ULL'
obj = output / 'resident-decoder-validation-tag44.o'
args[args.index('-o') + 1] = str(obj)
args += ['-D' + flag + '=1' for flag in ('PIONEER_Q38_EMBEDDING_NATIVE', 'PIONEER_Q38_LEARNED_RMS_NATIVE',
         'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_DECODER_RUNTIME',
         'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE')]
subprocess.run(args, check=True, env=clean)
def frames(path):
    return {re.sub(r':\d+:\d+:', ':', line.split('\t')[0]): int(line.split('\t')[1])
            for line in path.read_text().splitlines() if len(line.split('\t')) == 3}
current = frames(obj.with_suffix('.su'))
# Compare ACTUAL source to byte-identical ACTUAL tag42,not a wrapper's
# include-relative path spelling. Both pre-existing oversized frames must
# be present with NO growth;new frames still have the strict2048B limit.
previous = frames(output / 'controls/controls/projection-validation-tag42.su')
assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in (
    'PioneerQ38ResidentDecoder', 'PioneerQ38ConvHistory', 'InitializeQ38ConvHistoryModel',
    'RunQ38ResidentDecoderModel', 'CloseQ38ResidentDecoderModel', 'Q38ResidentDecoderValidationControl'))]
assert new and max(new) <= 2048
links = [a for a in commands if '-shared' in a and old_object in a]
assert len(links) == 1
args = [str(obj) if a == old_object else a for a in links[0]]
candidate = output / 'tenstorrent_pci_probe-driver55-decoder-debug-tag44'
args[args.index('-o') + 1] = str(candidate)
subprocess.run(args, check=True, env=clean)
print('PASS actual complete DEBUG44 native driver maxNewFrame=' + str(max(new)) + ';ALL old byte controls/baretag43 EXACT')
print(hashlib.sha256(candidate.read_bytes()).hexdigest() + '  ' + str(candidate))
print('BUILT_ONLY exact DEBUG44/namespace14/40 fixed poisoned readback pages/first-token-layer0;NO_INSTALL_DEVICE_OR_INFERENCE_SPEED_CLAIM')
