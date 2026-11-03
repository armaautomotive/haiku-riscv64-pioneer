#!/usr/bin/env python3
"""Actual protected parent RUN/END/restore flow, fake commands only; never SSH/GPU."""
import os
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_native_bfp_physical.sh').read_text()
flow = source[source.index('exchange "$stable_hash" "$candidate_hash"'):]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-native-bfp-parent.'))
client = folder / 'fake-client'
client.write_text('''#!/bin/sh
bits=${1#--fixed-bfp}
printf 'run%s\\n' "$bits" >> "$events"
if [ "$mode" = "fail$bits" ];then echo FIXTURE_UNKNOWN >&2;exit 70;fi
if [ "$mode" = bad4 ] && [ "$bits" = 4 ];then echo WRONG_COMPLETION;exit 0;fi
echo "PASS_NATIVE_BFP_FIXED_KERNEL bits$bits rows32 dual_jobs2 exact_words64 polls1 generation19;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM"
''')
client.chmod(0o700)
prefix = '''#!/bin/sh
set -eu
stage=$1;client=$2;events=$3;mode=$4;export events mode
stable_hash=STABLE;candidate_hash=CANDIDATE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events"; }
exact(){ :; }
idle(){ :; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
sysinfo(){ echo FIXTURE_SYSINFO; }
'''
program = folder / 'flow.sh'
program.write_text(prefix + flow)
for mode in ('pass', 'fail4', 'fail8', 'bad4'):
    stage = folder / mode
    stage.mkdir()
    events = stage / 'events'
    result = subprocess.run(['sh', str(program), str(stage), str(client), str(events), mode], text=True, capture_output=True)
    sequence = events.read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0 and sequence.count('run4') == sequence.count('run8') == 1
        assert 'exchange:2026110300010078:2026102100010012' in sequence
        assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_BOTH_FIXED_FORMATS_PASSED'
    else:
        assert result.returncode != 0 and sequence.count('run4') == 1
        assert sequence.count('run8') == int(mode == 'fail8')
        assert 'exchange:2026110300010078:2026102100010012' not in sequence
        last_run = max(i for i, value in enumerate(sequence) if value.startswith('run'))
        assert last_run == len(sequence)-1  # NO subsequent metadata/GPU/restore command
print('PASS ACTUAL protected native-BFP parent flow:4 cases;known-clean restore only,failed/malformed first or second run NO retry/reset/metadata/restore;FAKE commands,no hardware')
print('CPU-only parent-flow evidence:', folder)
