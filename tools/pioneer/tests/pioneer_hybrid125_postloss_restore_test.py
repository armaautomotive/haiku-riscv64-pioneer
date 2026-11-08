#!/usr/bin/env python3
"""Exercise the actual post-loss restore script against synthetic files only."""
from pathlib import Path
import os
import subprocess
import tempfile

source = (Path(__file__).resolve().parent.parent / 'pioneer_hybrid125_postloss_restore.sh').read_text()
backup = '5f6cea34aa32016fab6703f2ef6ef8ca9e7d52beb5a2f200d26a9b1c9ef2239d'
shim = r'''
uname(){ printf '%s\n' Haiku; }
id(){ printf '%s\n' 0; }
stat(){
 case "$2" in
  '%d') case "$3" in */data*) if [ "$SIM_MODE" = same-device ];then printf '%s\n' 1;else printf '%s\n' 2;fi;; *) printf '%s\n' 1;; esac;;
  '%u:%a') printf '%s\n' 0:700;;
  '%u:%h:%a') case "$3" in */trial.*|*/state|*/fresh-epoch-attestation) printf '%s\n' 0:1:600;; *) printf '%s\n' 0:1:755;; esac;;
  *) return 1;;
 esac
}
sysinfo(){ if [ "$SIM_MODE" = bad-health ];then printf '%s\n' '1 CPU';else printf '%s\n' '64 CPUs running at speed' 'used/max 1 / 137034936320)';fi; }
ps(){ printf '%s\n' 'Id Team';if [ "$SIM_MODE" = busy ];then printf '%s\n' '99 99 /boot/system/Assistant';else printf '%s\n' '99 99 harmless';fi; }
readlink(){ /usr/bin/readlink "$@"; }
awk(){ /usr/bin/awk "$@"; }
sha256sum(){
 case "$1" in */pre-power/*) kind=pre;; */post-power/*) kind=post;; *) kind=stage;; esac
 case "${1##*/}" in
  state) if [ "$kind" = pre ];then hash=3bce1c40bc5a6f07b157a73ad0f284edb4434758a84f42c63d1725de3e192134;else hash=094397c8680a2bb50de5ca274f1468d53861415c400bb0057af66c8d7a3f5591;fi;;
  trial.stdout) if [ "$kind" = pre ];then hash=a58a9e280debf17d63dd8b5a9300a48251ca38cd2f3df94cfa0d3bfae3ba2eaf;else hash=cc67398cdf4d67e740c063928b2253e4e06afae79f35e9bfd5cd768e873d23aa;fi;;
  trial.stderr) if [ "$kind" = pre ];then hash=1a0a0100639af812b389f109a5cd3164d58ce3f24f67f39caefb8a0ca7508322;else hash=97224f9450e7d5ed6fc6e071d7a17fe39a90a368a8855f182cebac9bfa7e1cfb;fi;;
  trial.exit) if [ "$kind" = pre ];then hash=4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865;else hash=bacddcf184771ace7c730d0d0b27d0ec723c9187adf8f3233d39190a5ba27ff3;fi;;
  driver125|previous125) hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099;;
  tenstorrent_pci_probe) case "$(cat "$1")" in STABLE) hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403;; NATIVE) hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099;; *) hash=WRONG;; esac;;
  tenstorrent_pci_probe-scale-reuse-qualification-041fef18|incoming041) hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403;;
  pioneer-p100a-reload) hash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f;;
  pioneer_p100a_reload_guard) hash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229;;
  checked-flush) hash=242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d;;
  HaikuAssistantQwen27) hash=861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74;;
  HaikuAssistantService) hash=c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18;;
  *) return 1;;
 esac
 if [ "${1##*/}" = state ];then
  case "$SIM_MODE:$kind" in bad-stage:stage|bad-pre:pre|bad-post:post) hash=WRONG;; esac
 fi
 if [ "$SIM_MODE" = bad-flush-hash ] && [ "${1##*/}" = checked-flush ];then hash=WRONG;fi
 printf '%s  %s\n' "$hash" "$1"
}
'''

with tempfile.TemporaryDirectory(prefix='pioneer-postloss-restore-') as directory:
    base = Path(directory)
    for mode in ('pass','check-only','bad-backup','bad-stage','bad-pre','bad-post','no-epoch','bad-epoch','old-spent','new-spent','same-device','bad-flush-hash',
                 'bad-health','busy','cold','owner','wrong-tag','wrong-generation','flush1','quiesce-fail',
                 'changed-after-quiesce','flush2','wait-fail'):
        root=base/mode/'home';archive=base/mode/'data'/'pioneer-hybrid125-evidence-ssqFWSMo'
        stage=root/'pioneer-hybrid125-full-trial-20261107-a';stage.mkdir(parents=True)
        (stage/'.trial-once').mkdir()
        if mode!='no-epoch':
            (stage/'fresh-epoch-attestation').write_text('WRONG\n' if mode=='bad-epoch' else 'OFF_ON_VERIFIED_FRESH_HAIKU_BOOT\n')
        for group in (stage,archive/'pre-power',archive/'post-power'):
            group.mkdir(parents=True,exist_ok=True)
            for name in ('state','trial.stdout','trial.stderr','trial.exit'):(group/name).write_text('TEST DATA')
        (stage/'driver125').write_text('NATIVE')
        if mode=='old-spent':(stage/'recovery-once').mkdir()
        if mode=='new-spent':(root/'pioneer-hybrid125-postloss-restore-20261107-a').mkdir()
        installed=root/'config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe'
        installed.parent.mkdir(parents=True);installed.write_text('NATIVE')
        device=root/'config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe'
        device.parent.mkdir(parents=True);device.symlink_to('../../bin/tenstorrent_pci_probe')
        guard=root/'config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard'
        guard.parent.mkdir(parents=True);guard.write_text('GUARD')
        for name in ('HaikuAssistantQwen27','HaikuAssistantService'):
            item=root/'config/non-packaged/servers'/name;item.parent.mkdir(parents=True,exist_ok=True);item.write_text('ASSISTANT')
        stable=root/'pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18'
        stable.parent.mkdir(parents=True);stable.write_text('STABLE')
        admin=stable.parent/'pioneer-p100a-reload'
        admin.write_text('''#!/bin/sh
echo "$1" >> "$SIM_EVENTS"
case "$1" in
 --status)
  tag=2026110700010125;generation=1;cold=0;owner=0
  if [ "$(cat "$SIM_INSTALLED")" = STABLE ];then tag=2026102100010012;generation=2;fi
  case "$SIM_MODE" in cold)cold=1;; owner)owner=1;; wrong-tag)tag=2026102100010012;; wrong-generation)generation=2;; esac
  printf 'P100A_RELOAD driver55 tag%s generation%s quiesced0 cold%s active_owner%s open_count1 guard1;CPU_METADATA_ONLY\\n' "$tag" "$generation" "$cold" "$owner";;
 --quiesce)
  if [ "$SIM_MODE" = changed-after-quiesce ];then printf '%s' UNEXPECTED > "$SIM_INSTALLED";fi
  [ "$SIM_MODE" != quiesce-fail ];;
 --wait) [ "$SIM_MODE" != wait-fail ];;
esac
''');admin.chmod(0o700)
        flush=root/'pioneer-qgw8-flush-tool-20261107-a/checked-flush';flush.parent.mkdir(parents=True)
        flush.write_text('''#!/bin/sh
PATH=/usr/bin:/bin
echo --flush >> "$SIM_EVENTS"
count=$(grep -c '^--flush$' "$SIM_EVENTS")
[ "$SIM_MODE" != "flush$count" ]
''');flush.chmod(0o700)
        program=base/mode/'run.sh'
        program.write_text(source.replace('set -eu\n','set -eu\n'+shim,1).replace('/boot/home',str(root)).replace('/HaikuData',str(base/mode/'data')))
        events=base/mode/'events';events.write_text('')
        env={**os.environ,'SIM_MODE':mode,'SIM_EVENTS':str(events),'SIM_INSTALLED':str(installed)}
        result=subprocess.run(['sh',str(program),'--check-only' if mode=='check-only' else '--restore',
                               '--attest-verified-backup','bad' if mode=='bad-backup' else backup],
                              env=env,capture_output=True,text=True)
        calls=events.read_text().splitlines();work=root/'pioneer-hybrid125-postloss-restore-20261107-a'
        assert (stage/'state').read_text()=='TEST DATA' and (stage/'.trial-once').is_dir()
        if mode=='check-only':
            assert result.returncode==0 and calls==['--status'] and not work.exists()
            assert installed.read_text()=='NATIVE' and 'CHECK_ONLY_PASSED' in result.stdout
        elif mode=='pass':
            assert result.returncode==0,(result.stdout,result.stderr,calls)
            assert calls==['--status','--flush','--quiesce','--flush','--wait','--status']
            assert installed.read_text()=='STABLE' and (work/'previous125').read_text()=='NATIVE'
        else:
            assert result.returncode!=0,(mode,result.stdout,calls)
            if mode in ('bad-backup','bad-stage','bad-pre','bad-post','no-epoch','bad-epoch','old-spent','new-spent','same-device','bad-flush-hash',
                        'bad-health','busy','cold','owner','wrong-tag','wrong-generation'):
                assert '--quiesce' not in calls and '--flush' not in calls and installed.read_text()=='NATIVE'
                if mode!='new-spent':assert not work.exists()
            if mode=='flush1':assert calls==['--status','--flush'] and installed.read_text()=='NATIVE'
            if mode=='quiesce-fail':assert calls==['--status','--flush','--quiesce'] and installed.read_text()=='NATIVE'
            if mode=='changed-after-quiesce':assert calls==['--status','--flush','--quiesce'] and installed.read_text()=='UNEXPECTED'
            if mode=='flush2':assert calls==['--status','--flush','--quiesce','--flush'] and installed.read_text()=='STABLE'
            if mode=='wait-fail':assert calls==['--status','--flush','--quiesce','--flush','--wait'] and installed.read_text()=='STABLE'
print('PASS actual postloss restore flow and 21 refusal/fault cases; synthetic files only, NO DEVICE')
