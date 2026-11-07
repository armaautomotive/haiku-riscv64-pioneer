#!/usr/bin/env python3
"""Exercise recovery control flow with fake files/commands only; no device."""
from pathlib import Path
import subprocess
import tempfile

assert __debug__
source=(Path(__file__).resolve().parent.parent/'pioneer_hybrid125_fresh_recovery.sh').read_text()
for required in ('metadata 2026110700010125 1', 'cold0 active_owner0',
 'OFF_ON_VERIFIED_FRESH_HAIKU_BOOT',
 '379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099',
 '3bce1c40bc5a6f07b157a73ad0f284edb4434758a84f42c63d1725de3e192134',
 '4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865'):
 assert required in source,required
flow=source.split('# HYBRID125_FRESH_RESTORE_FLOW_BEGIN\n',1)[1].split('# HYBRID125_FRESH_RESTORE_FLOW_END')[0]
prefix='''set -eu
stage=$1;mode=$2;events=$stage/events;admin=$stage/admin
stable=$stage/stable;installed=$stage/installed;stable_hash=STABLE;candidate_hash=NATIVE
incoming_checks=0;installed_checks=0;idle_checks=0
export mode events
exact(){
 name=${1##*/};echo "exact:$name:$2" >> "$events"
 if [ "$name" = recovery.incoming ];then
  incoming_checks=$((incoming_checks+1))
  if [ "$mode" = incoming-fail ] || { [ "$mode" = incoming-late-fail ] && [ "$incoming_checks" = 2 ]; };then return 1;fi
 fi
 if [ "$name" = installed ];then
  installed_checks=$((installed_checks+1))
  if { [ "$mode" = installed-early-fail ] && [ "$installed_checks" = 1 ]; } ||
   { [ "$mode" = installed-late-fail ] && [ "$installed_checks" = 2 ]; } ||
   { [ "$mode" = installed-final-fail ] && [ "$installed_checks" = 3 ]; };then return 1;fi
 fi
 [ "$(cat "$1")" = "$2" ]
}
idle(){ idle_checks=$((idle_checks+1));echo idle >> "$events";[ "$mode" != busy ] && { [ "$mode" != busy-final ] || [ "$idle_checks" != 2 ]; }; }
metadata(){ echo "metadata:$1:$2" >> "$events";[ "$mode" != metadata-fail ]; }
cp(){ echo cp >> "$events";[ "$mode" != cp-fail ] || return 1;command cp "$@"; }
chmod(){ echo chmod >> "$events";[ "$mode" != chmod-fail ] || return 1;command chmod "$@"; }
mv(){ echo mv >> "$events";[ "$mode" != move-fail ] || return 1;command mv "$@"; }
sync(){ echo sync >> "$events";[ "$mode" != sync-fail ]; }
'''
with tempfile.TemporaryDirectory(prefix='pioneer-hybrid125-recovery-') as directory:
 base=Path(directory);program=base/'flow.sh';program.write_text(prefix+flow)
 modes=('pass','cp-fail','chmod-fail','incoming-fail','installed-early-fail','busy',
        'quiesce-fail','incoming-late-fail','installed-late-fail','move-fail','sync-fail',
        'installed-final-fail','wait-fail','metadata-fail','busy-final')
 for mode in modes:
  stage=base/mode;stage.mkdir();(stage/'stable').write_text('STABLE');(stage/'installed').write_text('NATIVE')
  admin=stage/'admin';admin.write_text('''#!/bin/sh
echo "$1:$2:$3" >> "$events"
if [ "$mode" = quiesce-fail ] && [ "$1" = --quiesce ];then exit 1;fi
if [ "$mode" = wait-fail ] && [ "$1" = --wait ];then exit 1;fi
''');admin.chmod(0o700)
  result=subprocess.run(['sh',str(program),str(stage),mode],text=True,capture_output=True)
  events=(stage/'events').read_text().splitlines();assert (stage/'recovery-once').is_dir()
  quiesces=[e for e in events if e.startswith('--quiesce:')];waits=[e for e in events if e.startswith('--wait:')]
  assert len(quiesces)<=1 and len(waits)<=1 and events.count('mv')<=1
  if quiesces:assert quiesces==['--quiesce:2026110700010125:']
  if waits:assert waits==['--wait:2026102100010012:0000000000000001']
  if mode=='pass':
   assert result.returncode==0 and (stage/'installed').read_text()=='STABLE'
   assert len(quiesces)==len(waits)==events.count('mv')==1
   assert (stage/'recovery.state').read_text().strip()=='NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION'
  else:
   assert result.returncode!=0 and (stage/'recovery.state').read_text().strip()=='FRESH_EPOCH_ADMITTED_BEFORE_RESTORE'
   if mode in ('cp-fail','chmod-fail','incoming-fail','installed-early-fail','busy'):
    assert not quiesces and 'mv' not in events and (stage/'installed').read_text()=='NATIVE'
   if mode in ('quiesce-fail','incoming-late-fail','installed-late-fail','move-fail'):
    assert len(quiesces)==1 and not waits and (stage/'installed').read_text()=='NATIVE'
   if mode in ('sync-fail','installed-final-fail'):
    assert len(quiesces)==1 and not waits and (stage/'installed').read_text()=='STABLE'
   if mode in ('wait-fail','metadata-fail','busy-final'):
    assert len(quiesces)==len(waits)==1 and (stage/'installed').read_text()=='STABLE'
  again=subprocess.run(['sh',str(program),str(stage),mode],text=True,capture_output=True)
  assert again.returncode and (stage/'events').read_text().splitlines()==events
print('PASS fresh hybrid125 restore: 15 fake outcomes, spent-once marker, ordered stop-on-failure; NO DEVICE')
