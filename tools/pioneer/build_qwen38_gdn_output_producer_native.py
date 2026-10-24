#!/usr/bin/env python3
"""Actual native53 gated producer/ssm_out DATA compile;NO linked driver/card."""
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
subprocess.run([sys.executable, str(here/'build_qwen38_gdn_rendezvous_native.py'), str(sdk), str(output/'old51')], check=True, env=clean)
old = output/'old51/gdn-rendezvous-NATIVE51-COMPILE-ONLY.o'
assert hashlib.sha256(old.read_bytes()).hexdigest() == 'dbb550a34baeb76b1f68c2216d74bd2e2a42e7ba7633154c6e82c295f5f1de53', 'old51 MUST remain exact'
traces = list(output.rglob('old-tag38-build.trace')); assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]; assert len(compiles) == 1
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]; assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010053ULL'
args[args.index('-c')+1] = str(here/'tests/pioneer_q38_gdn_output_producer_native_full_compile.cpp')
obj = output/'gdn-output-producer-NATIVE53-COMPILE-ONLY.o'; args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True, env=clean)
def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split('\t')
        if len(parts) != 3:
            continue
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', parts[0]); assert match
        key = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[key] = max(result.get(key, 0), int(parts[1]))
    return result
current, previous = frames(obj.with_suffix('.su')), frames(old.with_suffix('.su')); assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in ('GatedOutput', 'GdnOutput', 'pioneer_gdn_output_producer'))]
assert new and max(new) <= 2048
print('PASS ACTUAL full native53 SAME gated producer/complete ssm_out canonical DATA compile maxNewFrame='+str(max(new)))
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('ALL old bytes through native51 EXACT;NO ssm_out execution/PUBLIC SELECTOR/LINKED DRIVER/INSTALL/DEVICE_OR_SPEED_CLAIM')
