#!/usr/bin/env python3
"""Replay actual SDK compile against new sources;old native63 must be exact."""
import hashlib
import os
import pathlib
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
driver = here.parents[2]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a];assert len(compiles) == 1
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010063ULL'
args[args.index('-c')+1] = str(here/'pioneer_q38_next_residual_rms_native_full_compile.cpp')
output.mkdir(mode=0o700)
obj = output/'unchanged-native63.o';args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True, env={k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')})
assert hashlib.sha256(obj.read_bytes()).hexdigest() == '47c11c7f4ff8f3518ea9baaa81266ff7dbf880887ef44d25221d71f04a8e36c3'
print('PASS actual read-only-SDK native63 compile BYTE-IDENTICAL47c11c7f after DEBUG64 selection changes;old52 cdacaa3a EXACT;NO install/device/speed grant')
