#!/usr/bin/env python3
"""Actual fixed-Q4 parent flow;fake commands ONLY,never SSH/device IO."""
import os,pathlib,subprocess,tempfile
if not __debug__:raise SystemExit('Assertions required')
here=pathlib.Path(__file__).resolve().parent
source=(here.parent/'pioneer_native_exact_q4_physical.sh').read_text()
flow=source.split('# EXACT_Q4_PHYSICAL_FLOW_BEGIN\n',1)[1].split('# EXACT_Q4_PHYSICAL_FLOW_END',1)[0]
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-exact-q4-parent.'))
client=folder/'client'
client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-exact-q4 ] || exit 2
echo run >> "$events"
case "$mode" in
 fail) echo EXACT_Q4_UNKNOWN >&2;exit 1;;
 bad) echo WRONG_COMPLETION;exit 0;;
 stderr) echo EXTRA_UNPROVEN >&2;;
esac
echo 'PASS_NATIVE_EXACT_Q4_FIXED_KERNEL rows16 dual_jobs2 exact_words32 padding_words32 polls1 generation21;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM'
''');client.chmod(0o700)
prefix='''#!/bin/sh
set -eu
stage=$1;client=$2;events=$3;mode=$4;export events mode
stable_hash=STABLE;candidate_hash=CANDIDATE;client_hash=CLIENT;installed=INSTALLED;candidate=CANDIDATE
stable=$stage/source-stable;: > "$stable"
exchange(){ echo "exchange:$5:$6" >> "$events"; }
exact(){ :; };idle(){ :; };metadata(){ echo "metadata:$1:$2" >> "$events"; };health(){ echo health >> "$events"; }
'''
program=folder/'flow.sh';program.write_text(prefix+flow)
for mode in ('pass','fail','bad','stderr'):
    stage=folder/mode;stage.mkdir();events=stage/'events'
    result=subprocess.run(['sh',str(program),str(stage),str(client),str(events),mode],capture_output=True,text=True)
    sequence=events.read_text().splitlines();assert sequence.count('run')==1
    if mode=='pass':
        assert result.returncode==0 and sequence.count('exchange:20:21')==sequence.count('exchange:21:22')==1
        assert (stage/'state').read_text().strip()=='EXACT_Q4_CLOSED_STABLE22_RESTORED'
    else:
        assert result.returncode!=0 and 'exchange:21:22' not in sequence
        assert sequence[-1]=='run' # NO status/admin/compute/restore after failure
    assert (stage/'.trial-once').is_dir()
    again=subprocess.run(['sh',str(program),str(stage),str(client),str(events),mode],capture_output=True,text=True)
    assert again.returncode!=0 and events.read_text().splitlines()==sequence
print('PASS_ACTUAL_EXACT_Q4_PARENT fourFlows+fourSpentRefusals;knownCleanRestoreOnly;unknownOrMalformedNO_RETRY_RESET_METADATA_RESTORE;FAKE_ONLY')
