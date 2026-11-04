#!/usr/bin/env python3
"""Actual learned-band parent/output/provenance/idle gates;NO hardware."""
import hashlib
import pathlib
import re
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_native_model_band_physical.sh').read_text()
flow = source.split('# NATIVE_LEARNED_FLOW_BEGIN\n', 1)[1].split('# NATIVE_LEARNED_FLOW_END', 1)[0]
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
word_positions = []
for sample, first in enumerate((0, 8704, 17376)):
    for job in range(2):
        for row in range(32):
            word_positions.append(len(normal))
            word = expected[sample, row]
            normal.append(f'NATIVE_LEARNED_BAND_RETURNED_WORD sample{sample} first{first} pass{job} row{row} got{word} expected{word}')
    normal.append(f'PASS_NATIVE_LEARNED_BAND sample{sample} tensor7 first{first} width5120 phases4 dual_jobs2 exact_words64 polls1 generation9;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
normal.append('PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-learned-parent.'))
client = folder / 'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-learned-bands ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then echo UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
exit 0
''')
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
         ('early-final', [normal[-1]] + normal[:-1]), ('extra-final', normal + [normal[-1]]), ('garbage', normal + ['NOISE']),
         ('short-prior', normal)]
for number, position in enumerate(word_positions):
    cases.append((f'missing-{number}', normal[:position] + normal[position + 1:]))
    cases.append((f'duplicate-{number}', normal[:position] + [normal[position]] + normal[position:]))
    wrong = list(normal)
    wrong[position] = re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}', 'got00000000 expected00000000', wrong[position])
    cases.append((f'both-wrong-{number}', wrong))
for sample in range(3):
    marker = 66 + sample * 65
    for label, old, new in (('generation', 'generation9', 'generation7'), ('poll-zero', 'polls1', 'polls0'),
                            ('poll-large', 'polls1', 'polls20001'), ('fidelity', 'phases4', 'phases1'), ('geometry', 'width5120', 'width6144')):
        bad = list(normal)
        bad[marker] = bad[marker].replace(old, new)
        cases.append((f'{label}-{sample}', bad))
for mode, output in cases:
    stage = folder / mode
    stage.mkdir()
    (stage / 'reference').write_bytes(raw if mode != 'short-prior' else b'')
    (stage / 'output').write_text('\n'.join(output) + '\n')
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0 and events.count('run') == 1, (result.stdout, result.stderr)
        assert events.count('exchange:2026110400010080:2026102100010012') == 1
        assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_LEARNED_BANDS_EXACT192_PASSED'
    else:
        assert result.returncode != 0 and 'exchange:2026110400010080:2026102100010012' not in events, (mode, events)
        if 'run' in events:
            assert events[-1] == 'run' # no next status/restore/IO after unknown result
idle = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
idle_program = folder / 'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle + '\n}\nidle\n')
base = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-model-band-20261104-a/parent\n'
inventories = ((base, True), (base + '99 /boot/system/Tracker\n', True),
               (base + '99 /bin/bash -c stage=/boot/home/pioneer-native-model-band-20261104-a\n', False),
               (base + '99 /boot/home/pioneer-native-model-band-diagnostic\n', False),
               (base + '99 /boot/home/HaikuAssistantService\n', False), ('Id Team\n', False), (base + 'malformed\n', False))
for index, (text, accepted) in enumerate(inventories):
    inventory = folder / ('inventory-' + str(index))
    inventory.write_text(text)
    result = subprocess.run(['sh', str(idle_program), str(inventory)], text=True, capture_output=True)
    assert (result.returncode == 0) == accepted
gate = source.split('# NATIVE_LEARNED_PRIOR_GATE_BEGIN\n', 1)[1].split('# NATIVE_LEARNED_PRIOR_GATE_END', 1)[0]
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
prior = here.parent / 'benchmarks/qwen38-native-bfp-band-physical-20261104/remaining'
gate_cases = ('pass', 'bad-state', 'bad-exit', 'dirty-6144', 'dirty-17408', 'dirty-reference', 'missing-reference',
              'linked-reference', 'linked-model', 'missing-model', 'bad-model-permission', 'bad-owner')
for mode in gate_cases:
    stage = folder / ('prior-' + mode)
    (stage / 'prior').mkdir(parents=True)
    (stage / 'prior/state').write_text('STABLE041_RESTORED_REMAINING_FULL_K_EXACT128_PASSED\n' if mode != 'bad-state' else 'OTHER\n')
    for width in (6144, 17408):
        data = (prior / f'band{width}.stdout').read_bytes()
        (stage / f'prior/band{width}.stdout').write_bytes(data if mode != f'dirty-{width}' else data + b'X')
        (stage / f'prior/band{width}.exit').write_text('0\n' if mode != 'bad-exit' else '1\n')
    if mode == 'linked-reference':
        (stage / 'target').write_bytes(raw)
        (stage / 'learned-reference.log').symlink_to(stage / 'target')
    elif mode != 'missing-reference':
        (stage / 'learned-reference.log').write_bytes(raw if mode != 'dirty-reference' else raw + b'X')
    if mode == 'linked-model':
        (stage / 'model-target').write_bytes(b'FAKE_METADATA_ENDPOINT_NOT_MODEL')
        (stage / 'original').symlink_to(stage / 'model-target')
    elif mode != 'missing-model':
        (stage / 'original').write_bytes(b'FAKE_METADATA_ENDPOINT_NOT_MODEL')
    result = subprocess.run(['sh', str(gate_program), str(stage), mode], text=True, capture_output=True)
    assert (result.returncode == 0) == (mode == 'pass'), (mode, result.stdout, result.stderr)
print(f'PASS actual learned parent:{len(cases)} flow/output+7 idle+{len(gate_cases)} prior/reference/model-metadata cases;each192 missing/duplicate/equal-but-wrong word refused,strict source banners/sample ordering/fidelity/geometry/epoch/polls,first failure NO query/restore/retry;NO device proof')
