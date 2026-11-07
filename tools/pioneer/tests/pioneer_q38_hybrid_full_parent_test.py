#!/usr/bin/env python3
"""Exercise the actual one-shot shell flow against fake OS/device endpoints."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

here=Path(__file__).resolve().parent;original=(here.parent/'pioneer_q38_hybrid_full_physical_once.sh').read_text()
stable='041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
candidate='379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099'
client='9f8e5abb9543f7d2d31fbace9778bda0862867439c1d9f08d10f86335d5d5c92'
preflight='e354dbf45eebcee92479988391c3b9538390a742163e297e77bb33b5c62f49a6'
admin='b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'
guard='d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229'
gate='6799fec637859923d1de6cc30735fef64bda2d84db2b7884b5deccdc34171e15'
def script(path,text):path.write_text('#!/bin/sh\n'+text+'\n');path.chmod(0o755)
def case(root,mode):
 d=root/mode;d.mkdir();stage=d/'stage';stage.mkdir();bin=d/'bin';bin.mkdir()
 cache4=d/'cache4';cache4.mkdir();cache8=d/'cache8';cache8.mkdir()
 (d/'installed').write_text('stable\n');(d/'stable').write_text('stable\n');(d/'guard').write_text('guard\n')
 (d/'model').write_text('model\n');(stage/'driver125').write_text('candidate\n')
 (stage/'output.awk').write_bytes((here.parent/'pioneer_q38_hybrid_full_output.awk').read_bytes())
 (d/'device').symlink_to('../../bin/tenstorrent_pci_probe')
 script(bin/'uname','echo Haiku');script(bin/'id','echo 0')
 script(bin/'stat',r'''case "$2" in
 "%u:%a") echo 0:700;; "%u:%h:%a") case "$3" in */output.awk) echo 0:1:600;; *) echo 0:1:755;; esac;;
 "%u:%h:%a:%s") echo 0:1:600:18973870528;; "%d") echo 42;; *) exit 2;; esac''')
 script(bin/'sha256sum',f'''case "$1" in
 */installed|*/stable|*/driver125|*/stable.backup|*/driver.incoming)
  case "$(head -1 "$1")" in stable) h={stable};; candidate) h={candidate};; *) exit 2;; esac;;
 */full-test) h={client};; */preflight-test) h={preflight};; */admin) h={admin};; */guard) h={guard};;
 */output.awk) h={gate};; *) exit 2;; esac
 printf '%s  %s\\n' "$h" "$1"''')
 script(bin/'ps','printf "Id Team\\n1 kernel_team\\n"\n[ "${MOCK_MODE}" != busy ] || printf "2 /boot/home/full-test\\n"')
 script(bin/'sysinfo','printf "64 cpus running at 1\\nused/max 0 / 137034936320)\\n"')
 adminstate=d/'adminstate';adminstate.write_text('2026102100010012 20\n');events=d/'events'
 script(d/'admin',f'''echo "$1 ${{2:-}}" >> "{events}"
 case "$1" in
 --status) set -- $(cat "{adminstate}"); echo "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY";;
 --quiesce) [ "${{MOCK_MODE}}" != quiesce-fail ] || exit 1;;
 --wait)
  [ "${{MOCK_MODE}}:$2" != candidate-wait-fail:2026110700010125 ] || exit 1
  [ "${{MOCK_MODE}}:$2" != restore-wait-fail:2026102100010012 ] || exit 1
  case "$2" in 2026110700010125) echo "$2 21" > "{adminstate}";; 2026102100010012) echo "$2 22" > "{adminstate}";; *) exit 1;; esac;;
 *) exit 1;; esac''')
 script(stage/'preflight-test',r'''[ "$1" = --verify-prepared ] || exit 2
 [ "$MOCK_MODE" != preflight-fail ] || exit 4
 echo 'PASS_HYBRID_PREFLIGHT 1071 BFP4 plus 273 BFP8 fragments, existing caches verified; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED' ''')
 script(stage/'full-test',r'''[ "$1" = --run-hybrid-candidate ] && [ "$4" = 32 ] && [ "$5" = 248319 ] || exit 2
 [ "$MOCK_MODE" != client-fail ] || exit 4
 printf '%s\n' 'HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet' 'HYBRID_PREFLIGHT 1344/1344' \
 'HYBRID_UPLOAD 21137039360/21137039360' 'HYBRID_STARTED upload_and_start_seconds=1.000' \
 'HYBRID_RESULT prompt_tokens=1 generated_tokens=1 post_start_seconds=1.000 (includes prompt processing)' 'HYBRID_TOKEN 70253'
 [ "$MOCK_MODE" = bad-output ] || echo 'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated' ''')
 replacements={
  'PATH=/boot/system/bin:/bin':f'PATH={bin}:/usr/bin:/bin',
  'stage=/boot/home/pioneer-hybrid125-full-trial-20261107-a':f'stage={stage}',
  'installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe':f'installed={d}/installed',
  'stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18':f'stable={d}/stable',
  'admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload':f'admin={d}/admin',
  'guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard':f'guard={d}/guard',
  'cache4=/boot/home/pioneer-hybrid-bfp4-20261107-cache':f'cache4={cache4}',
  'cache8=/boot/home/pioneer-hybrid-bfp8-20261107-cache':f'cache8={cache8}',
  'original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf':f'original={d}/model',
  '/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe':str(d/'device'),
 }
 text=original
 for old,new in replacements.items():
  assert text.count(old)==(2 if old.endswith('/dev/misc/tenstorrent_pci_probe') else 1),(old,text.count(old))
  text=text.replace(old,new)
 parent=d/'parent';parent.write_text(text);env=dict(os.environ,MOCK_MODE=mode)
 result=subprocess.run(['sh',str(parent)],env=env,capture_output=True,text=True)
 lines=events.read_text().splitlines() if events.exists() else []
 if mode=='ok':
  assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
  assert (stage/'state').read_text().strip()=='CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED'
  assert (d/'installed').read_text()=='stable\n' and sum(x.startswith('--quiesce') for x in lines)==2
 elif mode=='busy':
  assert result.returncode and not (stage/'.trial-once').exists() and not lines
 elif mode=='preflight-fail':
  assert result.returncode and (stage/'.trial-once').exists() and not any(x.startswith('--quiesce') for x in lines)
  assert (d/'installed').read_text()=='stable\n'
 elif mode in ('client-fail','bad-output'):
  assert result.returncode and (d/'installed').read_text()=='candidate\n'
  assert sum(x.startswith('--quiesce') for x in lines)==1 and not any(x.startswith('--wait 2026102100010012') for x in lines)
 else:
  assert result.returncode and (stage/'state').read_text().startswith('FAILED_')
  if mode=='quiesce-fail':assert (d/'installed').read_text()=='stable\n'
  if mode=='candidate-wait-fail':assert (d/'installed').read_text()=='candidate\n'
  if mode=='restore-wait-fail':assert (d/'installed').read_text()=='stable\n'
 if (stage/'.trial-once').exists():
  before=events.read_text();again=subprocess.run(['sh',str(parent)],env=env,capture_output=True,text=True)
  assert again.returncode and events.read_text()==before
with tempfile.TemporaryDirectory(prefix='hybrid125-parent-',dir='/private/tmp') as directory:
 for mode in ('ok','busy','preflight-fail','client-fail','bad-output','quiesce-fail','candidate-wait-fail','restore-wait-fail'):
  case(Path(directory),mode)
print('PASS one-shot hybrid parent: normal and seven failure cases; mocked OS/device only')
