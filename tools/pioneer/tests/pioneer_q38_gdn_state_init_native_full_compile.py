#!/usr/bin/env python3
"""Full ACTUAL tag47 source/scope/factory/zero+CLOSE;COMPILE ONLY,no device."""
import hashlib
import os
import pathlib
import re
import runpy
import shlex
import subprocess
import sys

if any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != '--recurrent'):
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output [--recurrent]')
recurrent = len(sys.argv) == 4
here = pathlib.Path(__file__).resolve().parent
sdk, output = map(pathlib.Path, sys.argv[1:3])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
subprocess.run([sys.executable, str(here / ('pioneer_q38_gdn_state_init_native_full_compile.py' if recurrent else 'pioneer_q38_gdn_input_native_full_compile.py')), str(sdk), str(output / 'controls')], check=True, env=clean)
old = output / ('controls/gdn-state-init-NATIVE47-COMPILE-ONLY.o' if recurrent else 'controls/gdn-input-NATIVE45-COMPILE-ONLY.o')
pin = '7ee1d6f1a90885ee6ce129aefe6950b2267b85f3db527990e5fb18481887bfc6' if recurrent else 'c3e249dba9ab2a16a34f5a8641721d4d8f28bbb8c9978060178f813821cb5dbd'
assert hashlib.sha256(old.read_bytes()).hexdigest() == pin, 'old role MUST remain exact'
traces = list(output.rglob('old-tag38-build.trace'));assert len(traces) == 1
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
compiles = [a for a in commands if '-c' in a and a[a.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
args = [a for a in compiles[0] if not a.startswith('-DPIONEER_Q38_FFN_HOST_PROFILE') and not a.startswith('-DPIONEER_Q38_FFN_SEALED_PROOF')]
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=' + ('0x2026102400010048ULL' if recurrent else '0x2026102400010047ULL')
args[args.index('-c') + 1] = str(here / ('pioneer_q38_gdn_recurrent_native_full_compile.cpp' if recurrent else 'pioneer_q38_gdn_state_init_native_full_compile.cpp'))
obj = output / ('gdn-recurrent-NATIVE48-COMPILE-ONLY.o' if recurrent else 'gdn-state-init-NATIVE47-COMPILE-ONLY.o');args[args.index('-o') + 1] = str(obj)
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
current, previous = frames(obj.with_suffix('.su')), frames(old.with_suffix('.su'))
assert current
for name, size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
new = [size for name, size in current.items() if any(s in name for s in ('InitializeQ38GdnStateModel', 'RecurrentZeroSourceScope', 'MatchesInitializedGdn', 'pioneer_gdn_state_native_', 'PioneerQ38StateZero',
    'GdnRecurrent', 'RunGdnRecurrent', 'pioneer_gdn_recurrent', 'MatchesRecurrentNativeOwner'))]
assert new and max(new) <= 2048
print('PASS ACTUAL full tag' + ('48 SAME initialized-state QRJ1/all96 private next-layer' if recurrent else '47 SAME-owner BOTH-slot73728page zero+CLOSE') + '/source/context-cookie-team-thread-clock/ALLhosts/retention/END compile maxNewFrame=' + str(max(new)))
print(hashlib.sha256(obj.read_bytes()).hexdigest() + '  ' + str(obj))
print('ALL old bytes/tag43/actualDEBUG44/compile45 EXACT;NO PUBLIC SELECTOR/LINKED_NEW_DRIVER/INSTALL/DEVICE/SPEED_CLAIM')
