#!/usr/bin/env python3
"""Exercise actual head130 parent flow with fake client; never accesses GPU."""
import pathlib
import subprocess
import tempfile

assert __debug__
source = (pathlib.Path(__file__).resolve().parent.parent / 'pioneer_qgw8_head130_physical_once.sh').read_text()
flow = source.split('# HEAD130_FLOW_BEGIN\n', 1)[1].split('# HEAD130_FLOW_END', 1)[0]
normal = [
 'HEAD130_AUTHENTICATING_ORIGINAL_REFERENCE_AND_PAYLOAD;CPU_ONLY_NO_DEVICE_YET',
 'HEAD_EXPORT_VERIFY authenticating inputs; CPU_ONLY_NO_DEVICE',
 'HEAD_EXPORT_VERIFY sample0 first0 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE',
 'HEAD_EXPORT_VERIFY sample1 first70240 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE',
 'HEAD_EXPORT_VERIFY sample2 first108128 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE',
 'PASS_HEAD_EXPORT_VERIFIED three head bands, independent tile reader, normalized input and 96 reference words;NO_DEVICE_OR_SPEED',
 'HEAD130_SOURCE_VERIFIED real_head_bands3 fixed_cpu_reference;DEBUG_NOT_WORK',
 'HEAD130_SAMPLE sample0 first0 exact_words64 clean_end1 generation3;DEBUG_NOT_WORK',
 'HEAD130_SAMPLE sample1 first70240 exact_words64 clean_end1 generation3;DEBUG_NOT_WORK',
 'HEAD130_SAMPLE sample2 first108128 exact_words64 clean_end1 generation3;DEBUG_NOT_WORK',
 'PASS_HEAD130 three real-head bands exact192 clean_end3;NOT_FULL_MODEL_QUALITY_OR_SPEED']
with tempfile.TemporaryDirectory(prefix='qgw8-head130-parent-', dir='/private/tmp') as tmp:
    folder = pathlib.Path(tmp)
    client = folder / 'client'
    client.write_text('''#!/bin/sh
[ "$#" = 1 ] || exit 2
if [ "$1" = --verify-source-only ];then
 echo preflight >> "$events"
 [ "$mode" != preflightfail ] || exit 1
 cat "$stage/preflight-output"
 exit 0
fi
[ "$1" = --fixed-qgw8-head-bands ] || exit 2
echo run >> "$events"
[ "$mode" != runfail ] || { echo UNPROVEN >&2; exit 1; }
cat "$stage/output"
[ "$mode" != dirty ] || echo UNEXPECTED >&2
[ "$mode" != falsepass ] || exit 1
''')
    client.chmod(0o700)
    program = folder / 'flow.sh'
    program.write_text('''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events
export stage events mode
stable_hash=STABLE;candidate_hash=CANDIDATE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
''' + flow)
    cases = [('pass', normal), ('runfail', []), ('dirty', normal), ('falsepass', normal),
             ('swapfail', normal), ('busy', normal), ('preflightfail', normal),
             ('badpreflight', normal), ('extra', normal + ['garbage'])]
    for n in range(len(normal)):
        cases.extend([(f'missing{n}', normal[:n]+normal[n+1:]),
                      (f'duplicate{n}', normal[:n]+[normal[n]]+normal[n:]),
                      (f'altered{n}', normal[:n]+[normal[n]+'x']+normal[n+1:])])
    for name, output in cases:
        stage = folder / name
        stage.mkdir()
        (stage / 'output').write_text('\n'.join(output)+'\n')
        preflight = normal[:7] + ['PASS_HEAD130_SOURCE_ONLY;NO_DEVICE_OPEN']
        if name == 'badpreflight': preflight[2] += 'x'
        (stage / 'preflight-output').write_text('\n'.join(preflight)+'\n')
        result = subprocess.run(['sh', str(program), str(stage), str(client), name], capture_output=True, text=True)
        events = (stage / 'events').read_text().splitlines()
        restore = 'exchange:2026110800010130:2026102100010012'
        if name == 'pass':
            assert result.returncode == 0 and events.count('run') == 1 and events.count(restore) == 1, (name, events, result.stderr)
            assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_HEAD130_BANDS_EXACT192_PASSED'
        else:
            assert result.returncode != 0 and restore not in events, (name, events, result.stderr)
            if name in ('preflightfail', 'badpreflight'):
                assert events == ['preflight'], (name, events)
            if 'run' in events:
                assert events[-1] == 'run', (name, events)
print('PASS head130 parent flow: %d fake-client success/failure and transcript cases; NO DEVICE' % len(cases))
