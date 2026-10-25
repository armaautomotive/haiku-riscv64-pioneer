#!/usr/bin/env python3
"""Separate exact DEBUG64 from unchanged DEBUG52;build ONLY,NO device."""
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
subprocess.run([sys.executable, str(here/'build_qwen38_gdn_rendezvous_validation_native.py'), str(sdk), str(output/'old52')], check=True, env=clean)
old_driver = output/'old52/tenstorrent_pci_probe-driver55-gdn-rendezvous-debug-tag52'
assert hashlib.sha256(old_driver.read_bytes()).hexdigest() == 'cdacaa3a971823cbc5bbbebde8bd2c3771d48842df814729b5fe67c4dc290759', 'old52 MUST remain exact'
traces = list(output.rglob('old-tag38-build.trace'));assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a];assert len(compiles) == 1
old_object = compiles[0][compiles[0].index('-o')+1]
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010064ULL'
obj = output/'conv-silu-precision-validation-tag64.o';args[args.index('-o')+1] = str(obj)
args += ['-D'+flag+'=1' for flag in ('PIONEER_Q38_EMBEDDING_NATIVE', 'PIONEER_Q38_LEARNED_RMS_NATIVE',
    'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_DECODER_RUNTIME',
    'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
    'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE',
    'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_CONV_SILU_PRECISION_NATIVE')]
subprocess.run(args, check=True, env=clean)


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split('\t');assert len(parts) == 3
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', parts[0]);assert match
        key = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[key] = max(result.get(key, 0), int(parts[1]))
    return result


current, previous = frames(obj.with_suffix('.su')), frames((output/'old52/gdn-rendezvous-validation-tag52.o').with_suffix('.su'));assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in ('GdnRendezvousValidation', 'GdnTimedValidation', 'GdnReceipt', 'RunGdnRecurrent'))]
assert new and max(new) <= 2048
links = [a for a in commands if '-shared' in a and old_object in a];assert len(links) == 1
args = [str(obj) if a == old_object else a for a in links[0]]
candidate = output/'tenstorrent_pci_probe-driver55-conv-silu-precision-debug-tag64';args[args.index('-o')+1] = str(candidate)
subprocess.run(args, check=True, env=clean)
print('PASS ACTUAL DEBUG64 precise QCB1/SAME native52 parents/namespace17/fixed829632LOCAL words/ALL120held/full restore/END maxNewFrame='+str(max(new)))
print(hashlib.sha256(candidate.read_bytes()).hexdigest()+'  '+str(candidate))
print('Old52 cdacaa3a and ALL inherited controls EXACT;BUILT ONLY;NO INSTALL/DEVICE/PHYSICAL_TIMING/WORK_OR_SPEED_CLAIM')
