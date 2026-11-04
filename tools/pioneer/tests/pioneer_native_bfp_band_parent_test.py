#!/usr/bin/env python3
"""Actual new parent/output/idle gates;fake commands only,NO hardware access."""
import hashlib
import pathlib
import re
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_native_bfp_band_physical.sh').read_text()
flow = source.split('# NATIVE_BFP_BAND_FLOW_BEGIN\n', 1)[1].split('# NATIVE_BFP_BAND_FLOW_END')[0]
assert '0283070c764a5d992407bb5e06212cd82d75536bef37caa6cbd825f176618fe3' in source
assert 'STABLE041_RESTORED_DOCUMENTED_BFP8_EXACT64_PASSED' in source
raw = (here.parent / 'benchmarks/qwen38-native-bfp-band-owner-20261104/full-k-owner-r2.log').read_bytes()
assert hashlib.sha256(raw).hexdigest() == 'e16f4282406ea8f21ea3b7bdf3f9100482e3eae1b9fc2e16cbcf4b1c7a3e00cb'
unique = []
for line in raw.decode().splitlines():
    match = re.fullmatch(r'BAND_LEASE_SYNTHETIC_REFERENCE k5120 row([0-9]+) word([0-9a-f]{8})', line)
    if match:
        row, word = match.groups()
        assert int(row) == len(unique)
        unique.append(word)
assert len(unique) == 32
words = [f'NATIVE_BFP_BAND_RETURNED_WORD k5120 pass{job} row{row} got{word} expected{word}' for job in range(2) for row, word in enumerate(unique)]
summary = 'PASS_NATIVE_BFP_FULL_K bits8 width5120 rows32 dual_jobs2 phases4 exact_words64 polls1 generation5;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM'
normal = words + [summary]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-bfp8-documented-parent.'))
client = folder / 'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-k5120 ] || exit 2
echo run8 >> "$events"
if [ "$mode" = runfail ];then echo RUN_UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED_STDERR >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
''')
client.chmod(0o700)
prefix = '''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;observed=$stage/observed;reference=$stage/full-k-reference.log
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
         ('bad-generation', words + [summary.replace('generation5', 'generation19')]),
         ('zero-polls', words + [summary.replace('polls1', 'polls0')]),
         ('large-polls', words + [summary.replace('polls1', 'polls20001')]),
         ('early-summary', [summary] + words), ('late-word', words[:-1] + [summary] + words[-1:]),
         ('extra-summary', normal + [summary]), ('garbage', normal + ['UNRECOGNIZED']),
         ('bad-row', [words[0].replace('row0 ', 'row32 ')] + normal[1:]),
         ('bad-pass', [words[0].replace('pass0 ', 'pass2 ')] + normal[1:]),
         ('bad-hex', [words[0].replace('gotc10ebfc0', 'gotzzzzzzzz')] + normal[1:]),
         ('short-prior', normal)]
for index in range(64):
    cases.append((f'missing-{index}', normal[:index] + normal[index + 1:]))
    cases.append((f'duplicate-{index}', words + [words[index], summary]))
    wrong = list(normal)
    wrong[index] = re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}', 'got00000000 expected00000000', wrong[index])
    cases.append((f'both-wrong-{index}', wrong))
for mode, output in cases:
    stage = folder / mode
    stage.mkdir()
    (stage / 'observed').mkdir()
    (stage / 'full-k-reference.log').write_bytes(raw if mode != 'short-prior' else b'')
    (stage / 'output').write_text('\n'.join(output) + '\n')
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0, result.stderr
        assert events.count('run8') == 1 and 'exchange:2026110400010079:2026102100010012' in events
        assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_FULL_K5120_BFP8_EXACT64_PASSED'
    else:
        assert result.returncode != 0, (mode, result.stdout)
        assert events.count('run8') == int(mode not in ('swapfail', 'busy'))
        assert 'exchange:2026110400010079:2026102100010012' not in events
        if 'run8' in events:
            assert events[-1] == 'run8' # NO next metadata/restore/GPU after bad completion
idle_body = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
idle_program = folder / 'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle_body + '\n}\nidle\n')
listing = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-bfp-band-20261104-a/parent\n'
inventories = ((listing, True), (listing + '99 /boot/system/Tracker\n', True),
               (listing + '99 /bin/bash -c stage=/boot/home/pioneer-native-bfp-band-20261104-a\n', False),
               (listing + '99 /boot/home/HaikuAssistantService\n', False),
               (listing + '99 /boot/home/HaikuAssistantQwen27\n', False),
               (listing + '99 /boot/home/pioneer-p100a-wide-gdn-test\n', False),
               ('Id Team\n', False), (listing + 'malformed\n', False), (listing + 'Id Team\n', False))
for index, (text, accepted) in enumerate(inventories):
    inventory = folder / ('inventory-' + str(index))
    inventory.write_text(text)
    check = subprocess.run(['sh', str(idle_program), str(inventory)], text=True, capture_output=True)
    assert (check.returncode == 0) == accepted
gate = source.split('# NATIVE_BFP_BAND_PRIOR_GATE_BEGIN\n', 1)[1].split('# NATIVE_BFP_BAND_PRIOR_GATE_END', 1)[0]
gate_program = folder / 'prior.sh'
gate_program.write_text('''set -eu
stage=$1;observed=$stage/prior;mode=$2
stat(){ if [ "$mode" = bad-owner ];then echo 1:1;elif [ "$2" = '%u:%a' ];then echo 0:700;else echo 0:1;fi; }
sha256sum(){ shasum -a 256 "$@"; }
''' + gate)
prior_stdout = (here.parent / 'benchmarks/qwen38-native-bfp8-documented-pass-20261104/documented8.stdout').read_bytes()
assert hashlib.sha256(prior_stdout).hexdigest() == 'e952ec44ef26b0c912180ebb2eacb90cb3634098e18716982b867642de48ee20'
gate_cases = ('pass', 'bad-state', 'bad-exit', 'missing-stdout', 'dirty-stdout', 'linked-prior',
              'linked-stdout', 'bad-owner', 'dirty-reference', 'linked-reference', 'missing-reference')
for mode in gate_cases:
    stage = folder / ('gate-' + mode)
    stage.mkdir()
    prior = stage / 'prior'
    actual_prior = stage / 'actual-prior'
    if mode == 'linked-prior':
        actual_prior.mkdir()
        prior.symlink_to(actual_prior, target_is_directory=True)
    else:
        prior.mkdir()
    (prior / 'state').write_text('STABLE041_RESTORED_DOCUMENTED_BFP8_EXACT64_PASSED\n' if mode != 'bad-state' else 'OTHER\n')
    (prior / 'documented8.exit').write_text('0\n' if mode != 'bad-exit' else '70\n')
    if mode == 'linked-stdout':
        (stage / 'linked').write_bytes(prior_stdout)
        (prior / 'documented8.stdout').symlink_to(stage / 'linked')
    elif mode != 'missing-stdout':
        (prior / 'documented8.stdout').write_bytes(prior_stdout if mode != 'dirty-stdout' else prior_stdout + b'X')
    if mode == 'linked-reference':
        (stage / 'reference-target').write_bytes(raw)
        (stage / 'full-k-reference.log').symlink_to(stage / 'reference-target')
    elif mode != 'missing-reference':
        (stage / 'full-k-reference.log').write_bytes(raw if mode != 'dirty-reference' else raw + b'X')
    check = subprocess.run(['sh', str(gate_program), str(stage), mode], text=True, capture_output=True)
    assert (check.returncode == 0) == (mode == 'pass'), (mode, check.stdout, check.stderr)
print(f'PASS actual full-K parent:{len(cases)} fake-flow/output cases+9 idle+{len(gate_cases)} prior/raw-reference provenance cases;all64 independent Fraction-qualified synthetic predictions,missing/duplicate/equal-but-wrong values and metadata/order/poll failures refused,NO retry/metadata/restore after unproven result;NO device proof')
