#!/usr/bin/env python3
"""Exercise the QGWF rate one-shot flow with fake endpoints, never a device."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
source=(here.parent/'pioneer_q38_qgwf_rate_144_physical_once.sh').read_text()
flow=source[source.index('mkdir "$stage/.trial-once"'):]
gate=here.parent/'pioneer_q38_qgwf_rate_candidate_output.awk'
assert hashlib.sha256(gate.read_bytes()).hexdigest() in source
assert 'client_hash=b0a4efe4183ab9782746f5f6983e919b5dc0c836cc46162bd3d45840e04970c6' in source
assert 'metadata 2026102100010012 28 && health' in source
assert '2026102100010012 28 2026111100010144 29' in source
assert '2026111100010144 29 2026102100010012 30' in source
assert 'metadata 2026102100010012 30 && health' in source
with tempfile.TemporaryDirectory(prefix='qgwd-parent-',dir='/private/tmp') as directory:
 root=Path(directory)
 preflight=root/'preflight';preflight.write_text('''#!/bin/sh
echo preflight >> "$events"
[ "$mode" != preflight-fail ] || exit 4
echo 'PASS_QGWD_PREFLIGHT 1113 BFP4 FFN, 231 BFP8 FFN, 7 BFP8 head, 336 BFP8 dense fragments; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED'
''');preflight.chmod(0o700)
 client=root/'client';client.write_text('''#!/bin/sh
[ "$1" = --run-qgwf-rate-144 ] && [ "$#" = 5 ] || exit 2
echo run >> "$events"
[ "$mode" != client-fail ] || exit 4
printf '%s\\n' 'HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet' 'HYBRID_PREFLIGHT 1351/1351' \\
 'HYBRID_UPLOAD 21094223872/21094223872' 'HYBRID_START_REQUEST diagnostic only; no completion implied' \\
 'HYBRID_STARTED upload_and_start_seconds=1.000' \\
 'HYBRID_RESULT prompt_tokens=1 generated_tokens=17 inference_seconds=1.000 (includes START and prompt processing)' \\
 'QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 17.000000'
i=0; while [ "$i" -lt 17 ]; do echo 'HYBRID_TOKEN 70253'; i=$((i+1)); done
if [ "$mode" != bad-output ]; then
 echo 'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated'
 echo 'QGWF_OBSERVED_DECODE_UNAVAILABLE insufficient_or_invalid_count_window;NOT_A_SPEED_RESULT'
fi
''');client.chmod(0o700)
 prefix='''#!/bin/sh
set -eu
stage=$1;client=$2;preflight=$3;gate=$4;events=$5;mode=$6;export events mode
stable_hash=STABLE;candidate_hash=CANDIDATE;client_hash=CLIENT;preflight_hash=PREFLIGHT;gate_hash=GATE
installed=installed;candidate=candidate;stable=stable;cache4=cache4;cache8=cache8;cachehead=cachehead;cachedense=cachedense
exchange(){ echo "exchange:$3:$5" >> "$events";
 [ "$mode" != swap-fail ] || return 1
 [ "$mode" != restore-fail ] || [ "$3" != 2026111100010144 ] || return 1
}
exact(){ :; }
exact_data(){ :; }
idle(){ :; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ :; }
checked_flush(){ :; }
cp(){ :; }
'''
 program=root/'flow.sh';program.write_text(prefix+flow)
 for mode in ('pass','preflight-fail','client-fail','bad-output','swap-fail','restore-fail'):
  stage=root/mode;stage.mkdir();events=stage/'events'
  result=subprocess.run(['sh',str(program),str(stage),str(client),str(preflight),str(gate),str(events),mode],
                        text=True,capture_output=True,env=os.environ)
  sequence=events.read_text().splitlines();restore='exchange:2026111100010144:2026102100010012'
  assert sequence.count('preflight')==1
  if mode=='pass':
   assert result.returncode==0 and sequence.count('run')==1 and restore in sequence,(result.stdout,result.stderr,sequence)
   assert (stage/'state').read_text().strip()=='CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED'
  elif mode in ('preflight-fail','client-fail','bad-output'):
   assert result.returncode!=0 and restore not in sequence
   assert sequence.count('run')==int(mode!='preflight-fail')
   assert sequence[-1]==('preflight' if mode=='preflight-fail' else 'run')
  elif mode=='swap-fail':
   assert result.returncode!=0 and restore not in sequence and 'run' not in sequence
   assert sequence[-1]=='exchange:2026102100010012:2026111100010144'
  else:
   assert result.returncode!=0 and sequence.count('run')==1 and sequence.count(restore)==1 and sequence[-1]==restore
  before=events.read_bytes();again=subprocess.run(['sh',str(program),str(stage),str(client),str(preflight),str(gate),str(events),mode],capture_output=True)
  assert again.returncode and events.read_bytes()==before
print('PASS QGWF rate one-shot flow: success restores once; failed preflight/run/output never retries or restores; FAKE endpoints')
