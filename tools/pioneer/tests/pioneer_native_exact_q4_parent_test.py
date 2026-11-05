#!/usr/bin/env python3
"""Actual fixed-Q4 parent flow;fake commands ONLY,never SSH/device IO."""
import os,pathlib,subprocess,tempfile,sys
if not __debug__:raise SystemExit('Assertions required')
here=pathlib.Path(__file__).resolve().parent
assert len(sys.argv)==1 or sys.argv[1:]==['--affine']
affine=sys.argv[1:]==['--affine']
source=(here.parent/('pioneer_native_exact_q4_affine_physical.sh' if affine else 'pioneer_native_exact_q4_physical.sh')).read_text()
if affine:source=source.replace('EXACT_Q4_AFFINE_PHYSICAL_FLOW','EXACT_Q4_PHYSICAL_FLOW')
flow=source.split('# EXACT_Q4_PHYSICAL_FLOW_BEGIN\n',1)[1].split('# EXACT_Q4_PHYSICAL_FLOW_END',1)[0]
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-exact-q4-parent.'))
client=folder/'client'
client_text='''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-exact-q4 ] || exit 2
echo run >> "$events"
case "$mode" in
 fail) echo EXACT_Q4_UNKNOWN >&2;exit 1;;
 bad) echo WRONG_COMPLETION;exit 0;;
 stderr) echo EXTRA_UNPROVEN >&2;;
esac
echo 'PASS_NATIVE_EXACT_Q4_FIXED_KERNEL rows16 dual_jobs2 exact_words32 padding_words32 polls1 generation21;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM'
'''
if affine:
    client_text=client_text.replace('--fixed-exact-q4','--fixed-exact-q4-full-k').replace('EXACT_Q4_','EXACT_Q4_AFFINE_')
    client_text=client_text.replace('FIXED_KERNEL rows16','FIXED_KERNEL k17408 rows16').replace('generation21;','generation23;').replace('NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM','NO_MODEL_OR_SPEED_CLAIM')
client.write_text(client_text);client.chmod(0o700)
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
        assert result.returncode==0 and sequence.count('exchange:22:23' if affine else 'exchange:20:21')==sequence.count('exchange:23:24' if affine else 'exchange:21:22')==1
        assert (stage/'state').read_text().strip()==('EXACT_Q4_AFFINE_CLOSED_STABLE24_RESTORED' if affine else 'EXACT_Q4_CLOSED_STABLE22_RESTORED')
    else:
        assert result.returncode!=0 and ('exchange:23:24' if affine else 'exchange:21:22') not in sequence
        assert sequence[-1]=='run' # NO status/admin/compute/restore after failure
    assert (stage/'.trial-once').is_dir()
    again=subprocess.run(['sh',str(program),str(stage),str(client),str(events),mode],capture_output=True,text=True)
    assert again.returncode!=0 and events.read_text().splitlines()==sequence
print('PASS_ACTUAL_EXACT_Q4'+('_AFFINE' if affine else '')+'_PARENT fourFlows+fourSpentRefusals;knownCleanRestoreOnly;unknownOrMalformedNO_RETRY_RESET_METADATA_RESTORE;FAKE_ONLY')
