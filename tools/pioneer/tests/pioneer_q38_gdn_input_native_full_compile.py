#!/usr/bin/env python3
"""Exact tag45 SAME-owner preparation COMPILE ONLY,all old byte controls."""
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
subprocess.run([sys.executable, str(here.parent / 'build_qwen38_resident_decoder_validation_native.py'),
                str(sdk), str(output / 'controls')], check=True, env=clean)
old = output / 'controls/tenstorrent_pci_probe-driver55-decoder-debug-tag44'
assert hashlib.sha256(old.read_bytes()).hexdigest() == '6ce7b4fecae927391119163022cb49f8775ca79776555d209e2eb4c3eacc564b', 'old actual DEBUG44 MUST stay byte-identical'
traces = list(output.rglob('old-tag38-build.trace'))
assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102300010045ULL'
args[args.index('-c') + 1] = str(here / 'pioneer_q38_gdn_input_native_full_compile.cpp')
obj = output / 'gdn-input-NATIVE45-COMPILE-ONLY.o'
args[args.index('-o') + 1] = str(obj)
subprocess.run(args, check=True, env=clean)
def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split('\t')
        if len(parts) != 3:
            continue
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', parts[0])
        assert match, parts[0]
        # Same ACTUAL file/function despite wrapper's ../../../ spelling.
        key = str(pathlib.Path(match[1]).resolve()) + ':' + match[2]
        # GCC may emit multiple outlined clones with the SAME printed
        # signature. Retain the WORST frame on BOTH sides,never the last.
        result[key] = max(result.get(key, 0), int(parts[1]))
    return result
current = frames(obj.with_suffix('.su'))
previous = frames(output / 'controls/resident-decoder-validation-tag44.su')
assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in ('GdnInput', 'pioneer_gdn_input', 'PrepareGdnInputs'))]
assert new and max(new) <= 2048
print('PASS ACTUAL full tag45 SAME-owner QCB1->QGI1 native factories/scope/code/source/ledger/receipts/ALL120holds/restore/END compile maxNewFrame=' + str(max(new)))
print(hashlib.sha256(obj.read_bytes()).hexdigest() + '  ' + str(obj))
print('ALL old byte controls/tag43/actualDEBUG44 EXACT;NO PUBLIC SELECTOR/LINKED_NEW_DRIVER/INSTALL/DEVICE/INFERENCE_SPEED_CLAIM')
