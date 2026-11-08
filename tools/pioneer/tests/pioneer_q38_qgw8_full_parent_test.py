#!/usr/bin/env python3
"""Exercise the actual one-shot QGW8 parent with synthetic files/devices."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
source=(here.parent/'pioneer_q38_qgw8_full_physical_once.sh').read_text()
stable='041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
candidate='676d929e4ae0044d91e5f0b0adfe014374359a0130b12ea34d54c71f6b63e95b'
client='d5ceac52133805dc744611bb089dc77d66ebd028fa095a032b28c7627bcde874'
preflight='026053aeba4a27084806f924ab7d256e593061655d8c77d895e204143b911f4f'
gate='9ea260980ce98e874d690bbde889d0c51aa1eaf3aee186e397c9779da3bb3089'
admin='b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'
guard='d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229'
flush='242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d'
def script(path,text):path.write_text('#!/bin/sh\n'+text+'\n');path.chmod(0o755)
def case(root,mode):
 d=root/mode;d.mkdir();stage=d/'stage';stage.mkdir();bindir=d/'bin';bindir.mkdir()
 for name in ('cache4','cache8','cachehead'):(d/name).mkdir()
 for name,text in (('installed','stable\n'),('stable','stable\n'),('guard','guard\n'),('model','model\n')):(d/name).write_text(text)
 (stage/'tenstorrent_pci_probe-qgw8-128').write_text('candidate\n')
 if mode=='spent':(stage/'.trial-once').mkdir()
 (stage/'pioneer_q38_qgw8_full_output.awk').write_bytes((here.parent/'pioneer_q38_qgw8_full_output.awk').read_bytes())
 (d/'device').symlink_to('../../bin/tenstorrent_pci_probe')
 script(bindir/'uname','echo Haiku');script(bindir/'id','echo 0')
 script(bindir/'stat',r'''case "$2" in
 '%u:%a') echo 0:700;; '%u:%h:%a') case "$3" in */pioneer_q38_qgw8_full_output.awk) echo 0:1:644;; *) echo 0:1:755;; esac;;
 '%u:%h:%a:%s') echo 0:1:600:18973870528;; '%d') echo 42;; *) exit 2;; esac''')
 script(bindir/'sha256sum',f'''case "$1" in
 */installed|*/stable|*/tenstorrent_pci_probe-qgw8-128|*/stable.backup|*/driver.incoming)
  case "$(head -1 "$1")" in stable) h={stable};; candidate) h={candidate};; *) exit 2;; esac;;
 */pioneer-q38-qgw8-full-candidate128) h={client};; */pioneer-q38-qgw8-cache-preflight) h={preflight};;
 */admin) h={admin};; */guard) h={guard};; */flush) h={flush};; */pioneer_q38_qgw8_full_output.awk) h={gate};; *) exit 2;; esac
 case "$MOCK_MODE:${{1##*/}}" in wrong-candidate:tenstorrent_pci_probe-qgw8-128|wrong-client:pioneer-q38-qgw8-full-candidate128|wrong-gate:pioneer_q38_qgw8_full_output.awk) h=WRONG;; esac
 printf '%s  %s\\n' "$h" "$1"''')
 script(bindir/'ps','printf "Id Team\\n1 kernel_team\\n"\n[ "${MOCK_MODE}" != busy ] || printf "2 /boot/home/pioneer-q38-full-test\\n"')
 script(bindir/'sysinfo','printf "64 cpus running at 1\\nused/max 0 / 137034936320)\\n"')
 adminstate=d/'adminstate';adminstate.write_text('2026102100010012 2\n');events=d/'events';events.write_text('')
 script(d/'admin',f'''echo "$1 ${{2:-}}" >> "{events}"
 case "$1" in
 --status) set -- $(cat "{adminstate}"); tag=$1;generation=$2;cold=0;owner=0
  case "$MOCK_MODE" in cold)cold=1;; owner)owner=1;; wrong-generation)generation=99;; wrong-tag)tag=2026110700010125;; esac
  echo "P100A_RELOAD driver55 tag$tag generation$generation quiesced0 cold$cold active_owner$owner open_count1 guard1;CPU_METADATA_ONLY";;
 --quiesce) [ "${{MOCK_MODE}}" != quiesce-fail ] || exit 1;;
 --wait)
  [ "${{MOCK_MODE}}:$2" != candidate-wait-fail:2026110700010128 ] || exit 1
  [ "${{MOCK_MODE}}:$2" != restore-wait-fail:2026102100010012 ] || exit 1
  case "$2" in 2026110700010128) echo "$2 3" > "{adminstate}";; 2026102100010012) echo "$2 4" > "{adminstate}";; *) exit 1;; esac;;
 *) exit 1;; esac''')
 script(d/'flush',f'''echo --flush >> "{events}"
 count=$(grep -c '^--flush$' "{events}")
 [ "$MOCK_MODE" != "flush$count" ]''')
 script(stage/'pioneer-q38-qgw8-cache-preflight',r'''[ "$1" = --verify-qgw8 ] || exit 2
 [ "$MOCK_MODE" != preflight-fail ] || exit 4
 echo 'PASS_QGW8_PREFLIGHT 1113 BFP4 FFN, 231 BFP8 FFN, 7 BFP8 head fragments; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED' ''')
 script(stage/'pioneer-q38-qgw8-full-candidate128',r'''[ "$1" = --run-qgw8-candidate ] && [ "$5" = 32 ] && [ "$6" = 248319 ] || exit 2
 [ "$MOCK_MODE" != client-fail ] || exit 4
 printf '%s\n' 'HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet' 'HYBRID_PREFLIGHT 1351/1351' \
 'HYBRID_UPLOAD 21167624192/21167624192' 'HYBRID_STARTED upload_and_start_seconds=1.000' \
 'HYBRID_RESULT prompt_tokens=1 generated_tokens=1 inference_seconds=1.000 (includes START and prompt processing)' \
 'QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 1.000000' 'HYBRID_TOKEN 70253'
 [ "$MOCK_MODE" = bad-output ] || echo 'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated' ''')
 replacements={
  'PATH=/boot/system/bin:/bin':f'PATH={bindir}:/usr/bin:/bin',
  'stage=/boot/home/pioneer-qgw8-full-trial-20261107-a':f'stage={stage}',
  'installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe':f'installed={d}/installed',
  'device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe':f'device={d}/device',
  'stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18':f'stable={d}/stable',
  'admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload':f'admin={d}/admin',
  'guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard':f'guard={d}/guard',
  'flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush':f'flush={d}/flush',
  'cache4=/HaikuData/pioneer-qgw8-bfp4-native-20261107':f'cache4={d}/cache4',
  'cache8=/boot/home/pioneer-hybrid-bfp8-20261107-cache':f'cache8={d}/cache8',
  'cachehead=/HaikuData/pioneer-qgw8-head-native-20261107':f'cachehead={d}/cachehead',
  'original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf':f'original={d}/model',
 }
 text=source
 for old,new in replacements.items():
  assert text.count(old)==1,(old,text.count(old));text=text.replace(old,new)
 parent=d/'parent';parent.write_text(text);env=dict(os.environ,MOCK_MODE=mode)
 result=subprocess.run(['sh',str(parent),*(['--check-only'] if mode=='check-only' else [])],env=env,capture_output=True,text=True)
 calls=events.read_text().splitlines();current=(d/'installed').read_text()
 if mode=='check-only':
  assert result.returncode==0 and 'QGW8_ONE_SHOT_CHECK_ONLY_PASSED' in result.stdout
  assert current=='stable\n' and calls==['--status '] and not (stage/'.trial-once').exists()
 elif mode=='ok':
  assert result.returncode==0,(result.stdout,result.stderr,calls)
  assert (stage/'state').read_text().strip()=='CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED'
  assert current=='stable\n' and sum(x.startswith('--quiesce') for x in calls)==2
 elif mode in ('busy','wrong-candidate','wrong-client','wrong-gate','cold','owner','wrong-generation','wrong-tag'):
  assert result.returncode and not (stage/'.trial-once').exists() and not any(x.startswith('--quiesce') for x in calls)
 elif mode=='spent':assert result.returncode and not calls
 elif mode in ('preflight-fail','flush1'):
  assert result.returncode and (stage/'.trial-once').exists() and not any(x.startswith('--quiesce') for x in calls) and current=='stable\n'
 elif mode in ('client-fail','bad-output','flush7'):
  assert result.returncode and current=='candidate\n'
  assert sum(x.startswith('--quiesce') for x in calls)==1 and not any(x.startswith('--wait 2026102100010012') for x in calls)
 else:
  assert result.returncode and (stage/'state').read_text().startswith('FAILED_')
  if mode=='quiesce-fail':assert current=='stable\n'
  if mode in ('candidate-wait-fail','flush5'):assert current=='candidate\n'
  if mode in ('restore-wait-fail','flush9'):assert current=='stable\n'
 if (stage/'.trial-once').exists():
  before=events.read_text();again=subprocess.run(['sh',str(parent)],env=env,capture_output=True,text=True)
  assert again.returncode and events.read_text()==before
with tempfile.TemporaryDirectory(prefix='qgw8-parent-',dir='/private/tmp') as directory:
 for mode in ('ok','check-only','busy','spent','wrong-candidate','wrong-client','wrong-gate','cold','owner','wrong-generation','wrong-tag',
              'preflight-fail','client-fail','bad-output','quiesce-fail','candidate-wait-fail',
              'restore-wait-fail','flush1','flush5','flush7','flush9'):case(Path(directory),mode)
print('PASS actual QGW8 one-shot parent and 19 synthetic refusal/failure cases; NO DEVICE')
