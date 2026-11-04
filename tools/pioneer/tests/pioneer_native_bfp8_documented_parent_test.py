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
source = (here.parent / 'pioneer_native_bfp8_documented_physical.sh').read_text()
flow = source.split('# NATIVE_BFP8_DOCUMENTED_FLOW_BEGIN\n', 1)[1].split('# NATIVE_BFP8_DOCUMENTED_FLOW_END')[0]
assert '53f89a183edca74fb2acadbfbbd3a2186847d5e5baafe5d533734c051ed95240' in source
assert 'NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED' in source
raw = (here.parent / 'benchmarks/qwen38-native-bfp8-observe-20261104/fixed8-returned.stderr').read_bytes()
assert hashlib.sha256(raw).hexdigest() == '66112229d3b318d6e79864ecf6d45ce3007a32ae44b7b6c939ce7b05c6ccdf36'
words = []
for line in raw.decode().splitlines()[1:]:
    match = re.fullmatch(r'NATIVE_BFP_RETURNED_WORD bits8 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected[0-9a-f]{8}', line)
    assert match
    job, row, word = match.groups()
    words.append(f'NATIVE_BFP8_DOCUMENTED_RETURNED_WORD pass{job} row{row} got{word} expected{word}')
assert len(words) == 64
summary = 'PASS_NATIVE_BFP8_DOCUMENTED_FIXED_KERNEL bits8 rows32 dual_jobs2 exact_words64 polls1 generation3;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_K_MODEL_QUALITY_OR_SPEED_CLAIM'
normal = words + [summary]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-bfp8-documented-parent.'))
client = folder / 'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-bfp8-documented-rounding ] || exit 2
echo run8 >> "$events"
if [ "$mode" = runfail ];then echo RUN_UNPROVEN >&2;exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo UNEXPECTED_STDERR >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
''')
client.chmod(0o700)
prefix = '''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;observed=$stage/observed
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
         ('bad-generation', words + [summary.replace('generation3', 'generation19')]),
         ('zero-polls', words + [summary.replace('polls1', 'polls0')]),
         ('large-polls', words + [summary.replace('polls1', 'polls20001')]),
         ('early-summary', [summary] + words), ('late-word', words[:-1] + [summary] + words[-1:]),
         ('extra-summary', normal + [summary]), ('garbage', normal + ['UNRECOGNIZED']),
         ('bad-row', [words[0].replace('row0 ', 'row32 ')] + normal[1:]),
         ('bad-pass', [words[0].replace('pass0 ', 'pass2 ')] + normal[1:]),
         ('bad-hex', [words[0].replace('got4180b500', 'gotzzzzzzzz')] + normal[1:]),
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
    (stage / 'observed/fixed8.stderr').write_bytes(raw if mode != 'short-prior' else b'')
    (stage / 'output').write_text('\n'.join(output) + '\n')
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0, result.stderr
        assert events.count('run8') == 1 and 'exchange:2026110300010078:2026102100010012' in events
        assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_DOCUMENTED_BFP8_EXACT64_PASSED'
    else:
        assert result.returncode != 0, (mode, result.stdout)
        assert events.count('run8') == int(mode not in ('swapfail', 'busy'))
        assert 'exchange:2026110300010078:2026102100010012' not in events
        if 'run8' in events:
            assert events[-1] == 'run8' # NO next metadata/restore/GPU after bad completion
idle_body = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
idle_program = folder / 'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle_body + '\n}\nidle\n')
listing = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-bfp8-documented-20261104-a/parent\n'
inventories = ((listing, True), (listing + '99 /boot/system/Tracker\n', True),
               (listing + '99 /bin/bash -c stage=/boot/home/pioneer-native-bfp8-documented-20261104-a\n', False),
               (listing + '99 /boot/home/HaikuAssistantService\n', False),
               (listing + '99 /boot/home/HaikuAssistantQwen27\n', False),
               (listing + '99 /boot/home/pioneer-p100a-wide-gdn-test\n', False),
               ('Id Team\n', False), (listing + 'malformed\n', False), (listing + 'Id Team\n', False))
for index, (text, accepted) in enumerate(inventories):
    inventory = folder / ('inventory-' + str(index))
    inventory.write_text(text)
    check = subprocess.run(['sh', str(idle_program), str(inventory)], text=True, capture_output=True)
    assert (check.returncode == 0) == accepted
print(f'PASS actual documented parent:{len(cases)} fake-flow/output cases+9 idle cases;all64 independent historical words,missing/duplicate/equal-but-wrong values and metadata/order/poll failures refused,NO retry/metadata/restore after unproven result;NO device proof')
