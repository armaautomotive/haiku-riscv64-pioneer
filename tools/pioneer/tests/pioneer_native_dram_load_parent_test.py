#!/usr/bin/env python3
"""Actual fixed LOAD-ONLY82 parent/output/prior/idle/exchange/one-shot fixtures."""
import ast
import hashlib
import pathlib
import subprocess
import sys
import tempfile

assert __debug__, 'Assertions required'
assert len(sys.argv) == 1 or sys.argv[1:] == ['--alignment64-control']
aligned = len(sys.argv) == 2
role = 83 if aligned else 82
word_count = 55372 if aligned else 52812
header_count = 5120 if aligned else 2560
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / ('pioneer_native_dram_load64_physical.sh' if aligned else 'pioneer_native_dram_load_physical.sh')).read_text()
flow = source.split('# NATIVE_LOAD_ONLY_FLOW_BEGIN\n', 1)[1].split('# NATIVE_LOAD_ONLY_FLOW_END', 1)[0]
normal = ['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
          'AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 sample0;LOAD_ONLY_NO_FPU_MODEL_OR_WORK',
          'PASS_NATIVE_DRAM_LOAD_ONLY sample0 tensor7 width5120 requests320 flits2880 saved_l152812 saved_gddr46080 payload_checked46080 headers_checked2560 polls2 generation3;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FPU_MODEL_QUALITY_OR_SPEED_CLAIM']
if aligned:
    normal = [v.replace('LOAD_ONLY', 'LOAD64_ONLY').replace('saved_l152812', 'saved_l155372').replace('headers_checked2560', 'headers_checked5120') for v in normal]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-load-only-parent.'))
client = folder / 'client'
client.write_text(('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-load-only-band ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then echo UNKNOWN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo DIRTY >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
''').replace('--fixed-load-only-band', '--fixed-load64-only-band' if aligned else '--fixed-load-only-band'))
client.chmod(0o700)
program = folder / 'flow.sh'
program.write_text('''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events
export stage events mode
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; };idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; };health(){ echo health >> "$events"; }
''' + flow)
cases = [('pass', normal), ('runfail', []), ('dirty', normal), ('false-pass', normal), ('swapfail', normal), ('busy', normal),
         ('extra', normal + ['NOISE']), ('old-math-marker', normal[:2] + [normal[2].replace('NO_FPU_MODEL_', 'NO_FULL_MODEL_')])]
if aligned:
    cases += [('old32-result', [v.replace('LOAD64_ONLY', 'LOAD_ONLY').replace('saved_l155372', 'saved_l152812').replace('headers_checked5120', 'headers_checked2560') for v in normal]),
              ('old32-journal', normal[:2] + [normal[2].replace('saved_l155372', 'saved_l152812')])]
for i in range(3):
    cases.append(('missing-' + str(i), normal[:i] + normal[i+1:]))
    cases.append(('duplicate-' + str(i), normal[:i] + [normal[i]] + normal[i:]))
for i, (old, new) in enumerate((('sample0', 'sample1'), ('tensor7', 'tensor8'), ('width5120', 'width6144'),
                               ('requests320', 'requests321'), ('flits2880', 'flits2879'), ('saved_l1' + str(word_count), 'saved_l150500'),
                               ('saved_gddr46080', 'saved_gddr0'), ('payload_checked46080', 'payload_checked46079'),
                               ('headers_checked' + str(header_count), 'headers_checked0'), ('polls2', 'polls0'), ('polls2', 'polls20001'),
                               ('generation3;', 'generation11;'), ('CLOCK_PCI_RESTORED', 'PCI_RESTORED'))):
    cases.append(('bad-field-' + str(i), normal[:2] + [normal[2].replace(old, new)]))
for mode, output in cases:
    stage = folder / mode;stage.mkdir();(stage / 'output').write_text('\n'.join(output) + '\n')
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0 and events.count('run') == 1
        assert events.count('exchange:20261104000100%d:2026102100010012' % role) == 1
        assert (stage / 'state').read_text().strip() == ('STABLE041_RESTORED_LOAD64_ONLY_PAYLOAD_AND_HEADERS_PASSED' if aligned else 'STABLE041_RESTORED_LOAD_ONLY_PAYLOAD_AND_HEADERS_PASSED')
    else:
        assert result.returncode and 'exchange:20261104000100%d:2026102100010012' % role not in events, (mode, events)
        if 'run' in events:
            assert events[-1] == 'run'
idle = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
idle_program = folder / 'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle + '\n}\nidle\n')
base = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-dram-load-only-20261104-a/parent\n'
inventories = ((base, True), (base + '99 /boot/system/Tracker\n', True),
               (base + '99 /bin/sh -c stage=/boot/home/pioneer-native-dram-load-only-20261104-a\n', False),
               (base + '99 /boot/home/pioneer-native-dram-load-diagnostic\n', False),
               (base + '99 /boot/home/HaikuAssistantService\n', False), ('Id Team\n', False), (base + 'malformed\n', False))
for i, (text, accepted) in enumerate(inventories):
    inventory = folder / ('inventory-' + str(i));inventory.write_text(text)
    result = subprocess.run(['sh', str(idle_program), str(inventory)], text=True, capture_output=True)
    assert (result.returncode == 0) == accepted
gate = source.split('# NATIVE_LOAD_ONLY_PRIOR_GATE_BEGIN\n', 1)[1].split('# NATIVE_LOAD_ONLY_PRIOR_GATE_END', 1)[0]
gate = gate.replace('original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf', 'original=$stage/original')
gate_program = folder / 'prior.sh'
gate_program.write_text('''set -eu
stage=$1;observed=$stage/prior;mode=$2
stat(){
 if [ "$mode" = bad-owner ];then echo 1:1
 elif [ "$2" = '%u:%a' ];then echo 0:700
 elif [ "$2" = '%u:%h:%a:%s' ];then
  if [ "$mode" = bad-model-permission ];then echo 0:1:644:18973870528;else echo 0:1:600:18973870528;fi
 else echo 0:1;fi
}
sha256sum(){ shasum -a 256 "$@"; }
''' + gate)
prior = here.parent / ('benchmarks/qwen38-native-dram-load-failed-20261104' if aligned else 'benchmarks/qwen38-native-dram-band-failed-20261104')
pins = {'gddr.stdout': '8c864eaa071bb226406bc5b10b96fea1e826b04db6333a1cc5a9c007bb9f7d36',
        'gddr.stderr': 'c853295ad73f590a350937b721fe97ce754fb229ecf73b4da17f17e5f5c37e05'}
if aligned:
    pins = {'load.stdout': '63f6c0619cb2769932daf9d8b83d644076a70c953a867ab061c6135ec597d99d',
            'load.stderr': 'f21f376681d8ab635a7df556cc6f231f3ba52073f92668c27a3922981094da7e'}
gate_cases = ('pass', 'bad-state', 'bad-exit', 'bad-recovery', 'dirty-stdout', 'dirty-stderr', 'linked-prior',
              'linked-model', 'missing-model', 'bad-model-permission', 'bad-owner')
for mode in gate_cases:
    stage = folder / ('prior-' + mode);(stage / 'prior').mkdir(parents=True)
    failed_state = 'UNPROVEN_LOAD_ONLY_NO_FURTHER_END_RESET_RETRY_RESTORE' if aligned else 'UNPROVEN_GDDR_BANDS_NO_END_RESET_RETRY_RESTORE'
    (stage / 'prior/state').write_text(failed_state + '\n' if mode != 'bad-state' else 'CLEAN\n')
    (stage / ('prior/load.exit' if aligned else 'prior/gddr.exit')).write_text('1\n' if mode != 'bad-exit' else '0\n')
    (stage / 'prior/recovery.state').write_text('NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED\n' if mode != 'bad-recovery' else 'NOT_RECOVERED\n')
    for name, digest in pins.items():
        raw = (prior / name).read_bytes();assert hashlib.sha256(raw).hexdigest() == digest
        if mode == 'linked-prior' and name.endswith('.stdout'):
            (stage / 'prior' / name).symlink_to(prior / name)
        else:
            (stage / 'prior' / name).write_bytes(raw + (b'X' if mode == 'dirty-' + name.split('.')[1] else b''))
    if mode == 'linked-model':
        (stage / 'original').symlink_to(stage / ('prior/load.stdout' if aligned else 'prior/gddr.stdout'))
    elif mode != 'missing-model':
        (stage / 'original').write_bytes(b'METADATA_FIXTURE_NOT_MODEL')
    result = subprocess.run(['sh', str(gate_program), str(stage), mode], text=True, capture_output=True)
    assert (result.returncode == 0) == (mode == 'pass'), (mode, result.stdout, result.stderr)
# Reuse existing exact conditional exchange/one-shot fixtures with only the
# captured source helper and file names substituted,not the production body.
tree = ast.parse((here / 'pioneer_native_dram_model_band_parent_test.py').read_text())
exchange = source.split('exchange(){\n', 1)[1].split('\n}\n', 1)[0]
strings = [n.value for n in ast.walk(tree) if isinstance(n, ast.Constant) and isinstance(n.value, str)]
prefixes = [v for v in strings if v.startswith('set -eu\nstage=$1;failure=$2;installed=')]
assert len(prefixes) == 1
exchange_program = folder / 'exchange.sh'
exchange_program.write_text(prefixes[0] + exchange + '\n}\nexchange OLD NEW oldtag newtag 2 3 "$replacement" || exit 1\n')
exchange_count = None
for failure in range(100):
    stage = folder / ('exchange-' + str(failure));stage.mkdir()
    (stage / 'count').write_text('0\n');(stage / 'installed').write_text('OLD');(stage / 'replacement').write_text('NEW')
    result = subprocess.run(['sh', str(exchange_program), str(stage), str(failure)], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if not failure:
        assert result.returncode == 0 and (stage / 'installed').read_text() == 'NEW'
        exchange_count = len(events)
    else:
        assert result.returncode and len(events) == failure, (failure, events)
    if failure == exchange_count:
        break
once = 'for name in .trial-once' + source.split('for name in .trial-once', 1)[1].split('# NATIVE_LOAD_ONLY_FLOW_BEGIN', 1)[0]
once_program = folder / 'once.sh'
once_program.write_text('set -eu\nstage=$1;stable=$stage/stable;stable_hash=STABLE\nexact(){ :; }\n' + once)
once_names = ('.trial-once', 'stable041.backup', 'driver.incoming', 'load.stdout', 'load.stderr', 'load.exit', 'parent.pid', 'state')
for name in ('pass', *once_names):
    stage = folder / ('once-' + name);stage.mkdir();(stage / 'stable').write_text('STABLE')
    if name != 'pass':
        (stage / name).write_text('EXISTING')
    result = subprocess.run(['sh', str(once_program), str(stage)], text=True, capture_output=True)
    assert (result.returncode == 0) == (name == 'pass')
    if name == 'pass':
        before = (stage / 'state').read_bytes(), (stage / 'parent.pid').read_bytes()
        again = subprocess.run(['sh', str(once_program), str(stage)], text=True, capture_output=True)
        assert again.returncode and before == ((stage / 'state').read_bytes(), (stage / 'parent.pid').read_bytes())
    else:
        assert (stage / name).read_text() == 'EXISTING'
print(f'PASS actual LOAD ONLY{role} parent:{len(cases)} flow/output+7 idle+{len(gate_cases)} failed-prior/recovery/model+{exchange_count + 1} exchange-step+10 one-shot cases;NO status/restore/retry after uncertainty;NO native byte/hardware/model/speed proof')
print('CPU-only LOAD ONLY parent evidence:', folder)
