#!/usr/bin/env python3
"""SAME native57 QFG3 claim/execution/Close/END compile;NO public route."""
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
subprocess.run([sys.executable, str(here/'build_qwen38_residual_rms_native.py'), str(sdk), str(output/'old-residual')], check=True, env=clean)
old = output/'old-residual/residual-rms-NATIVE56-COMPILE-ONLY.o'
assert hashlib.sha256(old.read_bytes()).hexdigest() == 'b9400c713c7f4385ec531c13ccc65496a9b49b277d7266c5d9029b74c1a97d9f', 'native56 MUST remain byte-exact'
traces = list(output.rglob('old-tag38-build.trace'));assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a];assert len(compiles) == 1
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010057ULL'
args[args.index('-c')+1] = str(here/'tests/pioneer_q38_ffn_residual_native_full_compile.cpp')
obj = output/'ffn-residual-NATIVE57-COMPILE-ONLY.o';args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True, env=clean)


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        row = line.split('\t')
        assert len(row) == 3
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', row[0]);assert match
        name = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[name] = max(result.get(name, 0), int(row[1]))
    return result


current, previous = frames(obj.with_suffix('.su')), frames(old.with_suffix('.su'));assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
sizes = [size for name, size in current.items() if 'FfnResidual' in name or 'FfnHidden' in name or 'pioneer_ffn_residual_run' in name or 'PioneerQ38ResidentDecoderOwner::Ffn' in name]
assert sizes and max(sizes) <= 2048
print('PASS ACTUAL SAME native57 QFG3 two-lineage claim/image/FFN+residual/STOP/hold/typed hidden/restore+END compile maxNewFrame='+str(max(sizes)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('ALL inherited controls/native56 EXACT;NO linked driver/public route/install/device IO/model speed claim')
