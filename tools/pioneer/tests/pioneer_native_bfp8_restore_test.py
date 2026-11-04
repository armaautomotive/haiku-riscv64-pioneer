#!/usr/bin/env python3
"""Fresh-epoch restore flow and gate shape;fake endpoints,NO power/SSH/GPU."""
import pathlib
import subprocess
import sys
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
assert len(sys.argv)==1 or (len(sys.argv)==2 and sys.argv[1]=='--documented')
documented = len(sys.argv)==2
name = 'pioneer_native_bfp8_documented_recovery_restore.sh' if documented else 'pioneer_native_bfp8_recovery_restore.sh'
source = (pathlib.Path(__file__).resolve().parent.parent / name).read_text()
assert 'metadata 2026110300010078 1' in source and 'cold0 active_owner0' in source
assert ('UNPROVEN_DOCUMENTED_BFP8_NO_END_RESET_RETRY_RESTORE' if documented else 'UNPROVEN_FIXED_BFP8_NO_END_RESET_RETRY_RESTORE') in source
assert ('7b38656c597dc4809522737bae9dad6a67414326dca1d46c4afc4395db8f8de9' if documented else 'a9e13ea83c7bee6724d560a8ade4eda862360885eae8f8560567a6e4e178f822') in source
flow = source.split('# NATIVE_BFP8_FRESH_RESTORE_FLOW_BEGIN\n', 1)[1].split('# NATIVE_BFP8_FRESH_RESTORE_FLOW_END')[0]
temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-bfp8-fresh-restore.'))
prefix = '''set -eu
stage=$1;mode=$2;events=$stage/events;admin=$stage/admin
stable=$stage/stable;installed=$stage/installed;stable_hash=STABLE;candidate_hash=NATIVE
export mode events
exact(){
 echo "exact:${1##*/}:$2" >> "$events"
 [ "$(cat "$1")" = "$2" ] || return 1
 if [ "$mode" = incoming-fail ] && [ "$1" = "$stage/recovery.incoming" ];then return 1;fi
}
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events";[ "$mode" != metadata-fail ]; }
sync(){ echo sync >> "$events"; }
mv(){ echo mv >> "$events";[ "$mode" != move-fail ] || return 1;command mv "$@"; }
'''
program = temporary / 'flow.sh'
program.write_text(prefix + flow)
for mode in ('pass', 'incoming-fail', 'busy', 'quiesce-fail', 'move-fail', 'wait-fail', 'metadata-fail'):
    stage = temporary / mode
    stage.mkdir()
    (stage / 'stable').write_text('STABLE')
    (stage / 'installed').write_text('NATIVE')
    (stage / 'admin').write_text('''#!/bin/sh
echo "$1" >> "$events"
if [ "$mode" = quiesce-fail ] && [ "$1" = --quiesce ];then exit 1;fi
if [ "$mode" = wait-fail ] && [ "$1" = --wait ];then exit 1;fi
''')
    (stage / 'admin').chmod(0o700)
    result = subprocess.run(['sh', str(program), str(stage), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    assert (stage / 'recovery-once').is_dir()
    if mode == 'pass':
        assert result.returncode == 0 and (stage / 'installed').read_text() == 'STABLE'
        assert events.count('--quiesce') == events.count('--wait') == events.count('mv') == 1
        assert (stage / 'recovery.state').read_text().startswith('NEW_POWER_EPOCH_STABLE041_RESTORED')
    else:
        assert result.returncode != 0
        if mode in ('incoming-fail', 'busy'):
            assert '--quiesce' not in events and 'mv' not in events
        if mode == 'quiesce-fail':
            assert events[-1] == '--quiesce' and 'mv' not in events
        if mode == 'move-fail':
            assert events[-1] == 'mv' and '--wait' not in events
        if mode == 'wait-fail':
            assert events[-1] == '--wait'
        if mode == 'metadata-fail':
            assert events[-1] == 'metadata:2026102100010012:2'
    old_events = events
    again = subprocess.run(['sh', str(program), str(stage), mode], text=True, capture_output=True)
    assert again.returncode and (stage / 'events').read_text().splitlines() == old_events
print('PASS '+name+':7 fake flow cases;candidate epoch1/cold0/owner0 gate,first failure stops IO,one replacement/wait only,no GPU/old-owner cleanup/retry,spent marker retained')
