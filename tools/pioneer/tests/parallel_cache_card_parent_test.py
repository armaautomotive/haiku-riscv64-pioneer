#!/usr/bin/env python3
"""Exercise actual post-preflight flow with fake probes only; no device."""
from pathlib import Path
import subprocess
import tempfile
assert __debug__
source=(Path(__file__).resolve().parent.parent/'pioneer_parallel_cache_card_trial_20261113.sh').read_text()
flow=source.split('# PARALLEL_CARD_FLOW_BEGIN\n',1)[1].split('# PARALLEL_CARD_FLOW_END',1)[0]
with tempfile.TemporaryDirectory(prefix='pioneer-parallel-parent-') as name:
 base=Path(name);program=base/'flow.sh';probe=base/'card-probe'
 probe.write_text('''#!/bin/sh
echo probe >> "$stage/events"
case "$mode" in
 fail) exit 70;;
 warning) echo warning >&2;;
 malformed) echo BAD;exit 0;;
esac
echo 'PASS_ACTUAL_ASSISTANT_CACHED_FOUR_TOKEN_WORK_KNOWN_CLEAN;NOT_FULL_NATIVE_NOT_20TOKENS_SEC_OR_COMPARATIVE_SPEEDUP_PROOF'
''');probe.chmod(0o700)
 program.write_text('''#!/bin/sh
set -eu
stage=$1;mode=$2;runtime=fake;export stage mode
idle(){ echo idle >> "$stage/events"; [ "$mode" != postflight ]; }
metadata(){ echo metadata >> "$stage/events"; }
health(){ echo health >> "$stage/events"; }
''' + flow)
 for mode in ('pass','fail','warning','malformed','postflight'):
  stage=base/mode;stage.mkdir();(stage/'card-probe').symlink_to(probe)
  run=subprocess.run(['sh',str(program),str(stage),mode],capture_output=True,text=True)
  assert run.returncode==(0 if mode=='pass' else 1 if mode=='postflight' else 70),(mode,run.stdout,run.stderr)
  assert (stage/'.card-once').is_dir()
  state=(stage/'card.state').read_text().strip()
  assert state==('CARD_CLOSED_PARALLEL_CACHE_VERIFIED' if mode=='pass' else 'CARD_POSTFLIGHT_UNPROVEN_NO_RETRY' if mode=='postflight' else 'CARD_UNPROVEN_NO_RETRY')
  before=(stage/'events').read_text()
  assert subprocess.run(['sh',str(program),str(stage),mode],capture_output=True).returncode!=0
  assert (stage/'events').read_text()==before
 for suffix in ('card.state','card.stdout','card.stderr','card.exit'):
  stage=base/('existing-'+suffix);stage.mkdir();(stage/'card-probe').symlink_to(probe)
  (stage/suffix).write_text('preserve')
  assert subprocess.run(['sh',str(program),str(stage),'pass'],capture_output=True).returncode!=0
  assert (stage/suffix).read_text()=='preserve' and not (stage/'.card-once').exists()
print('PASS actual one-shot card-trial flow: success/failure/postflight and existing evidence; FAKE ONLY')
