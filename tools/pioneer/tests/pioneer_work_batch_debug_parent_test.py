#!/usr/bin/env python3
"""Exercise the actual one-shot flow with fake endpoints; never touch hardware."""
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
source=(here.parent/'pioneer_work_batch_debug_physical_20261113.sh').read_text()
assert source.count('# WORK_BATCH_FLOW_BEGIN\n')==source.count('# WORK_BATCH_FLOW_END')==1
flow=source.split('# WORK_BATCH_FLOW_BEGIN\n',1)[1].split('# WORK_BATCH_FLOW_END',1)[0]
with tempfile.TemporaryDirectory(prefix='work-batch-parent-',dir='/private/tmp') as tmp:
    folder=Path(tmp);client=folder/'client'
    client.write_text('''#!/bin/sh
echo run >> "$events"
case "$mode" in runfail) exit 1;; dirty) echo bad >&2;; missing) exit 0;; esac
echo 'PASS_TWO_WAVE_BATCH_SYNTHETIC_REFERENCE_EXACT_KNOWN_CLEAN;NO_MODEL_OR_SPEED_CLAIM'
[ "$mode" != duplicate ] || echo 'PASS_TWO_WAVE_BATCH_SYNTHETIC_REFERENCE_EXACT_KNOWN_CLEAN;NO_MODEL_OR_SPEED_CLAIM'
[ "$mode" != falsepass ]
''');client.chmod(0o700)
    prefix='''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events
export mode events
generation=48;candidate_generation=49;restored_generation=50
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable;candidate_tag=2026111300010152
cp(){ :; }
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swapfail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events";[ "$mode" != metadatafail ]; }
health(){ echo health >> "$events"; }
'''
    program=folder/'flow.sh';program.write_text(prefix+flow)
    for mode in ('pass','swapfail','busy','metadatafail','runfail','dirty','missing','duplicate','falsepass'):
        stage=folder/mode;stage.mkdir()
        result=subprocess.run(['sh',str(program),str(stage),str(client),mode],capture_output=True,text=True)
        events=(stage/'events').read_text().splitlines();restore='exchange:2026111300010152:2026102100010012'
        if mode=='pass':
            assert result.returncode==0 and events.count('run')==events.count(restore)==1,(result.stderr,events)
            assert (stage/'trial.state').read_text().strip()=='STABLE_RESTORED_EXACT'
        else:
            assert result.returncode!=0 and restore not in events,(mode,events,result.stderr)
            if 'run' in events:assert events[-1]=='run',(mode,events)
print('PASS actual one-shot batch flow: one run, clean-only restore and failure/duplicate/refusal gates; NO DEVICE')
