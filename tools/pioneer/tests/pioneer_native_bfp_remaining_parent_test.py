#!/usr/bin/env python3
"""Actual fixed two-width parent;fake endpoints ONLY,never a device test."""
import hashlib
import pathlib
import re
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_native_bfp_remaining_widths_physical.sh').read_text()
flow = source.split('# NATIVE_BFP_REMAINING_FLOW_BEGIN\n', 1)[1].split('# NATIVE_BFP_REMAINING_FLOW_END', 1)[0]
assert 'for width in 6144 17408;do' in flow and '--fixed-k$width' in flow
raw = (here.parent / 'benchmarks/qwen38-native-bfp-band-owner-20261104/full-k-owner-r2.log').read_bytes()
assert hashlib.sha256(raw).hexdigest() == 'e16f4282406ea8f21ea3b7bdf3f9100482e3eae1b9fc2e16cbcf4b1c7a3e00cb'
expected = {width: [] for width in (6144, 17408)}
for line in raw.decode().splitlines():
    match = re.fullmatch(r'BAND_LEASE_SYNTHETIC_REFERENCE k(6144|17408) row([0-9]+) word([0-9a-f]{8})', line)
    if match:
        width, row = map(int, match.group(1, 2))
        assert row == len(expected[width])
        expected[width].append(match.group(3))
assert all(len(words) == 32 for words in expected.values())
def output(width):
    words = [f'NATIVE_BFP_BAND_RETURNED_WORD k{width} pass{job} row{row} got{word} expected{word}' for job in range(2) for row, word in enumerate(expected[width])]
    return words + [f'PASS_NATIVE_BFP_FULL_K bits8 width{width} rows32 dual_jobs2 phases4 exact_words64 polls1 generation7;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM']
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-remaining-parent.'))
client = folder / 'client'
client.write_text('''#!/bin/sh
case "$1" in --fixed-k6144) width=6144;;--fixed-k17408) width=17408;;*)exit 2;;esac
echo "run:$width" >> "$events"
if [ "$mode" = "runfail-$width" ];then echo UNPROVEN >&2;exit 1;fi
cat "$stage/output$width"
if [ "$mode" = "dirty-$width" ];then echo UNEXPECTED >&2;fi
if [ "$mode" = "false-pass-$width" ];then exit 1;fi
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
cases = [('pass', 0, None), ('swapfail', 0, None), ('busy', 0, None)]
for width in (6144, 17408):
    normal = output(width)
    for mode in ('runfail', 'dirty', 'false-pass'):
        cases.append((f'{mode}-{width}', width, normal))
    for index in range(64):
        cases.append((f'missing-{width}-{index}', width, normal[:index] + normal[index + 1:]))
        cases.append((f'duplicate-{width}-{index}', width, normal[:-1] + [normal[index], normal[-1]]))
        wrong = list(normal)
        wrong[index] = re.sub(r'got[0-9a-f]{8} expected[0-9a-f]{8}', 'got00000000 expected00000000', wrong[index])
        cases.append((f'both-wrong-{width}-{index}', width, wrong))
    for label, old, new in (('generation', 'generation7', 'generation5'), ('poll-zero', 'polls1', 'polls0'),
                            ('poll-large', 'polls1', 'polls20001'), ('fidelity', 'phases4', 'phases1')):
        cases.append((f'{label}-{width}', width, normal[:-1] + [normal[-1].replace(old, new)]))
    cases.append((f'early-{width}', width, [normal[-1]] + normal[:-1]))
for mode, failed_width, changed in cases:
    stage = folder / mode
    stage.mkdir()
    (stage / 'reference').write_bytes(raw)
    for width in (6144, 17408):
        (stage / f'output{width}').write_text('\n'.join(changed if width == failed_width else output(width)) + '\n')
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    runs = [line for line in events if line.startswith('run:')]
    if mode == 'pass':
        assert result.returncode == 0 and runs == ['run:6144', 'run:17408'], (result.stdout, result.stderr)
        assert events.count('exchange:2026110400010079:2026102100010012') == 1
        assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_REMAINING_FULL_K_EXACT128_PASSED'
    else:
        assert result.returncode != 0 and 'exchange:2026110400010079:2026102100010012' not in events, (mode, events)
        if failed_width:
            assert runs == (['run:6144'] if failed_width == 6144 else ['run:6144', 'run:17408'])
            assert events[-1] == f'run:{failed_width}' # NO query/restore/second run after unproven
        else:
            assert not runs

# Reuse the same actual prior/raw-reference gate with the NEW physical5120
# proof,not the earlier fixed32x32 record. Includes real byte hashing.
gate = source.split('# NATIVE_BFP_BAND_PRIOR_GATE_BEGIN\n', 1)[1].split('# NATIVE_BFP_BAND_PRIOR_GATE_END', 1)[0]
gate_program = folder / 'prior.sh'
gate_program.write_text('''set -eu
stage=$1;observed=$stage/prior;mode=$2
stat(){ if [ "$mode" = bad-owner ];then echo 1:1;elif [ "$2" = '%u:%a' ];then echo 0:700;else echo 0:1;fi; }
sha256sum(){ shasum -a 256 "$@"; }
''' + gate)
prior = (here.parent / 'benchmarks/qwen38-native-bfp-band-physical-20261104/k5120/band5120.stdout').read_bytes()
assert hashlib.sha256(prior).hexdigest() == '5c3a33f4a124860f81714eacf3a234204877c73cb31fe2c9f07c43faacdf491e'
gate_cases = ('pass', 'bad-state', 'bad-exit', 'dirty-prior', 'dirty-reference', 'bad-owner')
for mode in gate_cases:
    stage = folder / ('gate-' + mode)
    (stage / 'prior').mkdir(parents=True)
    (stage / 'prior/state').write_text('STABLE041_RESTORED_FULL_K5120_BFP8_EXACT64_PASSED\n' if mode != 'bad-state' else 'OTHER\n')
    (stage / 'prior/band5120.exit').write_text('0\n' if mode != 'bad-exit' else '70\n')
    (stage / 'prior/band5120.stdout').write_bytes(prior if mode != 'dirty-prior' else prior + b'X')
    (stage / 'full-k-reference.log').write_bytes(raw if mode != 'dirty-reference' else raw + b'X')
    check = subprocess.run(['sh', str(gate_program), str(stage), mode], text=True, capture_output=True)
    assert (check.returncode == 0) == (mode == 'pass'), (mode, check.stdout, check.stderr)
print(f'PASS actual remaining-width parent:{len(cases)} flow/output+{len(gate_cases)} prior/reference fixtures;each64 missing/duplicate/equal-but-wrong word in BOTH widths refused,strict fidelity/epoch/polls and first-failure stop;NO device proof')
