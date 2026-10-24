#!/usr/bin/env python3
"""Build separate DEBUG49 with exact old46/47/48 byte controls;NO device."""
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
subprocess.run([sys.executable, str(here / 'build_qwen38_gdn_input_validation_native.py'),
                str(sdk), str(output / 'old46')], check=True, env=clean)
assert hashlib.sha256((output / 'old46/tenstorrent_pci_probe-driver55-gdn-input-debug-tag46').read_bytes()).hexdigest() == '32a661c4d01cf1aebaed1edd5399bd0748bb37f5188d84a07cf8a7476cb3e47c'
subprocess.run([sys.executable, str(here / 'tests/pioneer_q38_gdn_state_init_native_full_compile.py'),
                str(sdk), str(output / 'old48'), '--recurrent'], check=True, env=clean)
assert hashlib.sha256((output / 'old48/gdn-recurrent-NATIVE48-COMPILE-ONLY.o').read_bytes()).hexdigest() == 'fcaf34450815bd4c01a8f6a0dbbc2240b987bfd5b88d342c90a377808179d722'
traces = list((output / 'old46').rglob('old-tag38-build.trace'));assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
old_object = compiles[0][compiles[0].index('-o') + 1]
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010049ULL'
obj = output / 'gdn-recurrent-validation-tag49.o';args[args.index('-o') + 1] = str(obj)
args += ['-D' + flag + '=1' for flag in ('PIONEER_Q38_EMBEDDING_NATIVE', 'PIONEER_Q38_LEARNED_RMS_NATIVE',
         'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_DECODER_RUNTIME',
         'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
         'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE')]
subprocess.run(args, check=True, env=clean)
def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split('\t')
        if len(parts) != 3:
            continue
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', parts[0]);assert match
        key = str(pathlib.Path(match[1]).resolve()) + ':' + match[2]
        result[key] = max(result.get(key, 0), int(parts[1]))
    return result
current = frames(obj.with_suffix('.su'))
previous = frames(output / 'old48/gdn-recurrent-NATIVE48-COMPILE-ONLY.su')
assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in ('GdnRecurrent', 'RunGdnRecurrent', 'InitializeQ38GdnStateModel'))]
assert new and max(new) <= 2048
links = [a for a in commands if '-shared' in a and old_object in a];assert len(links) == 1
args = [str(obj) if a == old_object else a for a in links[0]]
candidate = output / 'tenstorrent_pci_probe-driver55-gdn-recurrent-debug-tag49';args[args.index('-o') + 1] = str(candidate)
subprocess.run(args, check=True, env=clean)
print('PASS ACTUAL complete DEBUG49/namespace16 initialized GDN/ONE QRJ1/fixed823488LOCALwords/ALL120held/restore/END maxNewFrame=' + str(max(new)))
print(hashlib.sha256(candidate.read_bytes()).hexdigest() + '  ' + str(candidate))
print('ALL old bytes INCLUDING46/47/48 EXACT;BUILT ONLY,NO INSTALL/DEVICE/SPEED_CLAIM;CLIENT/HOTSWAP_QUALIFICATION_STILL_REQUIRED')
