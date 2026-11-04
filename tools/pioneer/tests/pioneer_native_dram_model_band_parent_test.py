#!/usr/bin/env python3
"""Actual diagnostic81 parent/output/prior/exchange/one-shot/idle gates;NO GPU."""
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile

assert __debug__, 'Assertions required'
assert sys.argv[1:] in ([], ['--alignment64-math'])
aligned = len(sys.argv) == 2
tag = '2026110400010084' if aligned else '2026110400010081'
label, generation, saved = ('GDDR64', 5, 55372) if aligned else ('GDDR', 11, 52812)
output_stem = 'gddr64' if aligned else 'gddr'
prior_stem = 'load' if aligned else 'learned'
prior_state = 'STABLE041_RESTORED_LOAD64_ONLY_PAYLOAD_AND_HEADERS_PASSED' if aligned else 'STABLE041_RESTORED_LEARNED_BANDS_EXACT192_PASSED'
prior_hash = 'a8884f789db23da897578ffb73a1c0849fa8e1e972f6e7701d10647b51df698c' if aligned else '71f8686db0d94af820be42c0e87f2f4b1bb4aa53acf3fbfb9b6e92cd2222d7e6'
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / ('pioneer_native_dram_model_band64_physical.sh' if aligned else 'pioneer_native_dram_model_band_physical.sh')).read_text()
assert 'CLIENT_HASH_TO_BE_FROZEN' not in source
flow = source.split('# NATIVE_GDDR_FLOW_BEGIN\n', 1)[1].split('# NATIVE_GDDR_FLOW_END', 1)[0]
raw = (here.parent / 'benchmarks/qwen38-native-model-band-preparation-20261104/learned-reference.log').read_bytes()
assert hashlib.sha256(raw).hexdigest() == '35142021d16a51e50b38657418800f3dcb102ddadc3961ec680d16ebf693ce83'
expected = {}
for line in raw.decode().splitlines():
    match = re.fullmatch(r'LEARNED_FRACTION_REFERENCE sample([012]) first([0-9]+) row([0-9]+) word([0-9a-f]{8})', line)
    if match:
        sample, first, row = map(int, match.group(1, 2, 3))
        assert first == (0, 8704, 17376)[sample] and row < 32 and (sample, row) not in expected
        expected[sample, row] = match.group(4)
assert len(expected) == 96
normal = ['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
          'AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK']
positions = []
for sample, first in enumerate((0, 8704, 17376)):
    for job in range(2):
        for row in range(32):
            positions.append(len(normal))
            word = expected[sample, row]
            normal.append(f'NATIVE_GDDR_BAND_RETURNED_WORD sample{sample} first{first} pass{job} row{row} got{word} expected{word}')
    normal.append(f'PASS_NATIVE_{label}_BAND sample{sample} tensor7 first{first} width5120 phases4 dual_jobs2 exact_words64 polls1 generation{generation} requests320 flits2880 saved_l1{saved} saved_gddr46080;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
normal.append(f'PASS_NATIVE_{label}_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-gddr-learned-parent.'))
client = folder / 'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-gddr-learned-bands ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then echo UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
'''.replace('--fixed-gddr-learned-bands', '--fixed-gddr64-learned-bands' if aligned else '--fixed-gddr-learned-bands'))
client.chmod(0o700)
prefix = '''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;reference=$stage/reference
export stage events mode
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
'''
program = folder / 'flow.sh'
program.write_text(prefix + flow)
cases = [('pass', normal), ('runfail', []), ('dirty', normal), ('false-pass', normal), ('swapfail', normal), ('busy', normal),
         ('missing-auth', normal[1:]), ('missing-verified', normal[:1] + normal[2:]),
         ('duplicate-auth', normal[:1] + normal), ('duplicate-verified', normal[:2] + normal[1:]),
         ('early-final', [normal[-1]] + normal[:-1]), ('extra-final', normal + [normal[-1]]), ('garbage', normal + ['NOISE']), ('short-prior', normal)]
if aligned:
    cases.append(('old81-record', [line.replace('GDDR64_', 'GDDR_').replace('generation5', 'generation11').replace('saved_l155372', 'saved_l152812') for line in normal]))
    for name, first, second in (('reordered-rows',2,3), ('reordered-jobs',2,34), ('reordered-last-rows',194,195)):
        bad = list(normal);bad[first],bad[second] = bad[second],bad[first];cases.append((name,bad))
for number, position in enumerate(positions):
    cases.append((f'missing-{number}', normal[:position] + normal[position + 1:]))
    cases.append((f'duplicate-{number}', normal[:position] + [normal[position]] + normal[position:]))
    wrong = list(normal)
    wrong[position] = re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}', 'got00000000 expected00000000', wrong[position])
    cases.append((f'both-wrong-{number}', wrong))
for sample in range(3):
    marker = 66 + sample * 65
    for name, old, new in (('generation', 'generation%d' % generation, 'generation9'), ('poll-zero', 'polls1', 'polls0'),
                            ('poll-large', 'polls1', 'polls20001'), ('fidelity', 'phases4', 'phases1'), ('geometry', 'width5120', 'width6144'),
                            ('requests', 'requests320', 'requests319'), ('flits', 'flits2880', 'flits2879'),
                            ('l1-journal', 'saved_l1%d' % saved, 'saved_l150500'), ('gddr-journal', 'saved_gddr46080', 'saved_gddr46079'),
                            ('clock-end', 'CLOCK_PCI_RESTORED', 'PCI_RESTORED')):
        bad = list(normal)
        assert old in bad[marker]
        bad[marker] = bad[marker].replace(old, new)
        cases.append((f'{name}-{sample}', bad))
for mode, output in cases:
    stage = folder / mode
    stage.mkdir()
    (stage / 'reference').write_bytes(raw if mode != 'short-prior' else b'')
    (stage / 'output').write_text('\n'.join(output) + '\n')
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0 and events.count('run') == 1, (result.stdout, result.stderr)
        assert events.count('exchange:' + tag + ':2026102100010012') == 1
        assert (stage / 'state').read_text().strip() == f'STABLE041_RESTORED_{label}_BANDS_EXACT192_PASSED'
    else:
        assert result.returncode != 0 and 'exchange:' + tag + ':2026102100010012' not in events, (mode, events)
        if 'run' in events:
            assert events[-1] == 'run', (mode, events) # no status/restore/IO after unknown
idle = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
idle_program = folder / 'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle + '\n}\nidle\n')
base = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-dram-model-band-20261104-a/parent\n'
inventories = ((base, True), (base + '99 /boot/system/Tracker\n', True),
               (base + '99 /bin/bash -c stage=/boot/home/pioneer-native-dram-model-band-20261104-a\n', False),
               (base + '99 /boot/home/pioneer-native-dram-model-band-diagnostic\n', False),
               (base + '99 /boot/home/HaikuAssistantService\n', False), ('Id Team\n', False), (base + 'malformed\n', False))
for index, (text, accepted) in enumerate(inventories):
    inventory = folder / ('inventory-' + str(index))
    inventory.write_text(text)
    result = subprocess.run(['sh', str(idle_program), str(inventory)], text=True, capture_output=True)
    assert (result.returncode == 0) == accepted
gate = source.split('# NATIVE_GDDR_PRIOR_GATE_BEGIN\n', 1)[1].split('# NATIVE_GDDR_PRIOR_GATE_END', 1)[0]
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
prior = here.parent / ('benchmarks/qwen38-native-dram-load64-observed-20261104' if aligned else 'benchmarks/qwen38-native-model-band-physical-20261104')
prior_raw = (prior / (prior_stem + '.stdout')).read_bytes()
assert hashlib.sha256(prior_raw).hexdigest() == prior_hash
gate_cases = ('pass', 'bad-state', 'bad-exit', 'dirty-learned', 'dirty-stderr', 'dirty-reference', 'missing-reference',
              'linked-reference', 'linked-model', 'missing-model', 'bad-model-permission', 'bad-owner', 'linked-prior')
for mode in gate_cases:
    stage = folder / ('prior-' + mode)
    (stage / 'prior').mkdir(parents=True)
    (stage / 'prior/state').write_text(prior_state + '\n' if mode != 'bad-state' else 'OTHER\n')
    (stage / ('prior/' + prior_stem + '.exit')).write_text('0\n' if mode != 'bad-exit' else '1\n')
    (stage / ('prior/' + prior_stem + '.stderr')).write_bytes(b'' if mode != 'dirty-stderr' else b'X')
    if mode == 'linked-prior':
        (stage / ('prior/' + prior_stem + '.stdout')).symlink_to(prior / (prior_stem + '.stdout'))
    else:
        (stage / ('prior/' + prior_stem + '.stdout')).write_bytes(prior_raw if mode != 'dirty-learned' else prior_raw + b'X')
    if mode == 'linked-reference':
        (stage / 'target').write_bytes(raw)
        (stage / 'learned-reference.log').symlink_to(stage / 'target')
    elif mode != 'missing-reference':
        (stage / 'learned-reference.log').write_bytes(raw if mode != 'dirty-reference' else raw + b'X')
    if mode == 'linked-model':
        (stage / 'model-target').write_bytes(b'METADATA_FIXTURE_NOT_MODEL')
        (stage / 'original').symlink_to(stage / 'model-target')
    elif mode != 'missing-model':
        (stage / 'original').write_bytes(b'METADATA_FIXTURE_NOT_MODEL')
    result = subprocess.run(['sh', str(gate_program), str(stage), mode], text=True, capture_output=True)
    assert (result.returncode == 0) == (mode == 'pass'), (mode, result.stdout, result.stderr)
# Actual exchange helper,local fixture files and failure at EACH observable
# command;no endpoint/action after the first error,no second quiesce/swap.
exchange = source.split('exchange(){\n', 1)[1].split('\n}\n', 1)[0]
exchange_program = folder / 'exchange.sh'
exchange_program.write_text('''set -eu
stage=$1;failure=$2;installed=$stage/installed;replacement=$stage/replacement;client=$stage/client;client_hash=CLIENT;admin=Admin
step(){ n=$(cat "$stage/count");n=$((n+1));echo "$n" > "$stage/count";echo "$1" >> "$stage/events";[ "$n" != "$failure" ]; }
exact(){ step exact; };idle(){ step idle; };metadata(){ step metadata; };Admin(){ step "admin:$1"; }
cp(){ step cp && command cp "$@"; };chmod(){ step chmod && command chmod "$@"; }
mv(){ step mv && command mv "$@"; };sync(){ step sync; }
printf(){ step printf && command printf "$@"; }
exchange(){
''' + exchange + '\n}\nexchange OLD NEW oldtag newtag 10 11 "$replacement" || exit 1\n')
exchange_count = None
for failure in range(100):
    stage = folder / ('exchange-' + str(failure));stage.mkdir()
    (stage / 'count').write_text('0\n');(stage / 'installed').write_text('OLD');(stage / 'replacement').write_text('NEW')
    result = subprocess.run(['sh', str(exchange_program), str(stage), str(failure)], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if not failure:
        assert result.returncode == 0 and (stage / 'installed').read_text() == 'NEW'
        assert events.count('admin:--quiesce') == 1 and events.count('admin:--wait') == 1
        exchange_count = len(events)
    else:
        assert result.returncode != 0 and len(events) == failure, (failure, events, result.stdout, result.stderr)
    if failure == exchange_count:
        break
assert exchange_count and exchange_count < 100
once = 'for name in .trial-once' + source.split('for name in .trial-once', 1)[1].split('# NATIVE_GDDR_FLOW_BEGIN', 1)[0]
once_program = folder / 'once.sh'
once_program.write_text('set -eu\nstage=$1;stable=$stage/stable;stable_hash=STABLE\nexact(){ :; }\n' + once)
once_names = ('.trial-once', 'stable041.backup', 'driver.incoming', output_stem + '.stdout', output_stem + '.stderr', output_stem + '.exit', 'parent.pid', 'state')
once_count = 0
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
    once_count += 1
print(f'PASS actual GDDR parent:{len(cases)} flow/output+7 idle+{len(gate_cases)} prior/source+{exchange_count + 1} exchange-step+{once_count + 1} one-shot cases;all192 missing/duplicate/equal-but-wrong words refused;no retry/reset/status/restore after uncertainty;NO hardware proof')
print('CPU-only actual GDDR parent evidence:', folder)
