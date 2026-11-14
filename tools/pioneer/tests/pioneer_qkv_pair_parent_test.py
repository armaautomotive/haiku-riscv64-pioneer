#!/usr/bin/env python3
"""Exercise the real pair wrapper flow with fake commands, never hardware."""
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_qkv_pair_physical_20261113.sh').read_text()
flow = source.split('# PAIR_QKV_FLOW_BEGIN\n', 1)[1].split('# PAIR_QKV_FLOW_END', 1)[0]
with tempfile.TemporaryDirectory(prefix='pioneer-pair-parent-') as tmp:
    root = pathlib.Path(tmp)
    preflight = root / 'preflight'
    preflight.write_text("#!/bin/sh\n[ \"$mode\" != preflight-fail ] || exit 1\necho 'PASS_RECURRENT_QKV_SOURCE tensor5 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED'\n")
    preflight.chmod(0o700)
    client = root / 'client'
    client.write_text('''#!/bin/sh
[ "$1" = --fixed-qkv-pair ] || exit 2
echo run >> "$events"
[ "$mode" != runfail ] || exit 1
cat "$stage/output"
[ "$mode" != dirty ] || echo unexpected >&2
[ "$mode" != false-pass ] || exit 1
''')
    client.chmod(0o700)
    stable = root / 'stable'
    stable.write_text('fake-stable')
    program = root / 'flow.sh'
    program.write_text('''set -eu
stage=$1;mode=$2;events=$stage/events;reference=$stage/reference
client=$3;preflight=$4;stable=$5;candidate=candidate;installed=installed
export stage mode events
stable_hash=STABLE;candidate_hash=PAIR;client_hash=CLIENT;generation=50;candidate_generation=51;restored_generation=52
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
''' + flow)
    first = (0, 5120, 10208)
    reference = ''.join(f'QKV_INTEGER_REFERENCE sample{s} first{first[s]} row{r} word{(s*32+r+1):08x}\n'
                        for s in range(3) for r in range(32))
    normal = ['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET']
    for sample in range(2):
        for job in range(2):
            for row in range(32):
                normal.append(f'PAIR_QKV_WORD sample{sample} first{first[sample]} pass{job} row{row} got{(sample*32+row+1):08x}')
    normal.append('PASS_NATIVE_QKV_PAIR samples2 exact_words128 overlap1 generation51;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_INFERENCE_SPEED_CLAIM')
    cases = [('pass', normal), ('runfail', []), ('dirty', normal), ('false-pass', normal), ('swapfail', normal),
             ('busy', normal), ('preflight-fail', normal), ('missing-auth', normal[1:]),
             ('duplicate-auth', normal[:1]+normal), ('missing-final', normal[:-1]),
             ('duplicate-final', normal+[normal[-1]]), ('extra-line', normal+['BAD']),
             ('wrong-generation', normal[:-1]+[normal[-1].replace('generation51','generation50')]),
             ('no-overlap', normal[:-1]+[normal[-1].replace('overlap1','overlap0')]),
             ('short-reference', normal)]
    for pos in range(1,129):
        wrong = normal.copy();wrong[pos] = wrong[pos][:-8] + 'ffffffff'
        cases.extend(((f'missing-{pos}', normal[:pos]+normal[pos+1:]),
                      (f'duplicate-{pos}', normal[:pos]+[normal[pos]]+normal[pos:]),
                      (f'wrong-{pos}', wrong)))
    for label, output in cases:
        stage = root / label
        stage.mkdir()
        (stage / 'reference').write_text('' if label == 'short-reference' else reference)
        (stage / 'output').write_text('\n'.join(output)+'\n')
        result = subprocess.run(['sh', str(program), str(stage), label, str(client), str(preflight), str(stable)], capture_output=True, text=True)
        events = (stage / 'events').read_text().splitlines() if (stage / 'events').exists() else []
        if label in ('pass','no-overlap'):
            assert result.returncode == 0 and events.count('run') == 1, (result.stdout,result.stderr,events)
            assert events.count('exchange:2026111300010153:2026102100010012') == 1
            suffix='1' if label=='pass' else '0'
            assert (stage / 'trial.state').read_text().strip() == 'STABLE_RESTORED_EXACT128_OVERLAP'+suffix
        else:
            assert result.returncode != 0 and 'exchange:2026111300010153:2026102100010012' not in events, (label,events,result.stderr)
            if 'run' in events:
                assert events[-1] == 'run', (label,events)
print(f'PASS_PAIR_PARENT {len(cases)} fake flow/output cases; exact independent words required; NO_DEVICE')
