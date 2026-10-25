#!/usr/bin/env python3
"""Exact native66 resident precision kernel compile;NO link/install/device."""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
qualified, output = map(pathlib.Path, sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
old52 = qualified/'old52/tenstorrent_pci_probe-driver55-gdn-rendezvous-debug-tag52'
assert hashlib.sha256(old52.read_bytes()).hexdigest() == 'cdacaa3a971823cbc5bbbebde8bd2c3771d48842df814729b5fe67c4dc290759'
traces = list(qualified.rglob('old-tag38-build.trace'));assert len(traces) == 1
here = pathlib.Path(__file__).resolve().parent
driver = here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
output.mkdir(mode=0o700)
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
old = output/'unchanged-native65.o'
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010065ULL'
args[args.index('-c')+1] = str(here/'tests/pioneer_q38_next_ffn_residual_native_full_compile.cpp')
args[args.index('-o')+1] = str(old)
subprocess.run(args, check=True, env=clean)
assert hashlib.sha256(old.read_bytes()).hexdigest() == '61faa093c5f247f8f120a6712a24c7e5ef6989c6dc8aeb7466241dca6d52a821', 'old65 MUST remain byte-exact'
obj = output/'resident-silu-precision-NATIVE66-COMPILE-ONLY.o'
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010066ULL'
args[args.index('-c')+1] = str(here/'tests/pioneer_q38_resident_silu_precision_native_full_compile.cpp')
args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True, env=clean)


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split('\t');assert len(parts) == 3
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', parts[0]);assert match
        key = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[key] = max(result.get(key, 0), int(parts[1]))
    return result


current, previous = frames(obj.with_suffix('.su')), frames(old.with_suffix('.su'));assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
sizes = [size for name, size in current.items() if any(s in name for s in ('NextFfn', 'next_ffn_residual_run', 'RunNextConv', 'PrepareConv'))]
assert sizes and max(sizes) <= 2048
print('PASS ACTUAL native66 SAME native65/full original prefix/BOTH precise QCB1/private layer2/strict holds+deadlines/restore+END compile maxNewFrame='+str(max(sizes)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('Old65 BYTE-IDENTICAL61faa093;COMPILE ONLY;NO linked driver/public selector/install/device/inference speed grant')
