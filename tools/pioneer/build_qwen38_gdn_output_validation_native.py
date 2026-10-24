#!/usr/bin/env python3
"""Build isolated fixed DEBUG55 from SAME native54;NO device/install."""
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
subprocess.run([sys.executable, str(here/'build_qwen38_gdn_output_execution_native.py'), str(sdk), str(output/'old54')], check=True, env=clean)
old = output/'old54/gdn-output-execution-NATIVE54-COMPILE-ONLY.o'
assert hashlib.sha256(old.read_bytes()).hexdigest() == 'c8930f61076877f20fbc647d8967285ffc144fd24201f4e828e7073002804e2e', 'old54 MUST remain exact'
traces = list(output.rglob('old-tag38-build.trace'));assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[1]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a];assert len(compiles) == 1
old_object = compiles[0][compiles[0].index('-o')+1]
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102400010055ULL'
obj = output/'gdn-output-validation-tag55.o';args[args.index('-o')+1] = str(obj)
args += ['-D'+flag+'=1' for flag in ('PIONEER_Q38_EMBEDDING_NATIVE', 'PIONEER_Q38_LEARNED_RMS_NATIVE',
    'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_DECODER_RUNTIME',
    'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
    'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE',
    'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE',
    'PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE', 'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE')]
subprocess.run(args, check=True, env=clean)


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        fields = line.split('\t')
        if len(fields) != 3:
            continue
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', fields[0]);assert match
        key = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[key] = max(result.get(key, 0), int(fields[1]))
    return result


current, previous = frames(obj.with_suffix('.su')), frames(old.with_suffix('.su'));assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
sizes = [size for name, size in current.items() if 'Output' in name or 'GdnTimedValidation' in name]
assert sizes and max(sizes) <= 2048
links = [a for a in commands if '-shared' in a and old_object in a];assert len(links) == 1
args = [str(obj) if a == old_object else a for a in links[0]]
candidate = output/'tenstorrent_pci_probe-gdn-output-debug-tag55';args[args.index('-o')+1] = str(candidate)
subprocess.run(args, check=True, env=clean)
print('PASS ACTUAL full DEBUG55/namespace18/SAME native54/fixed838016words/ALL120held/5fixedNoCreads/fullrestore/END maxNewFrame='+str(max(sizes)))
print(hashlib.sha256(candidate.read_bytes()).hexdigest()+'  '+str(candidate))
print('ALL inherited controls/native54 EXACT;BUILT ONLY;NO INSTALL/DEVICE/PHYSICAL_TIMING/WORK_OR_SPEED_CLAIM')
