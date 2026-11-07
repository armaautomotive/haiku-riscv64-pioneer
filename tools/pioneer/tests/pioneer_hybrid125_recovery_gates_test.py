#!/usr/bin/env python3
"""Run actual recovery gates with simulated OS, hashes and private files only."""
from pathlib import Path
import os
import subprocess
import tempfile

assert __debug__
source=(Path(__file__).resolve().parent.parent/'pioneer_hybrid125_fresh_recovery.sh').read_text()
shim='''
uname(){ printf '%s\\n' Haiku; }
id(){ printf '%s\\n' 0; }
stat(){
 case "$2" in
  '%d') printf '%s\\n' 1;;
  '%u:%a') printf '%s\\n' 0:700;;
  '%u:%h:%a') case "$3" in */trial.*|*/state|*/fresh-epoch-attestation) printf '%s\\n' 0:1:600;; *) printf '%s\\n' 0:1:755;; esac;;
  *) return 1;;
 esac
}
sysinfo(){ if [ "$SIM_MODE" = bad-health ];then printf '%s\\n' '1 CPU';else printf '%s\\n' '64 CPUs running at speed' 'used/max 1 / 137034936320)';fi; }
ps(){ printf '%s\\n' 'Id Team';if [ "$SIM_MODE" = busy ];then printf '%s\\n' '99 99 /boot/system/Assistant';else printf '%s\\n' '99 99 harmless';fi; }
readlink(){ /usr/bin/readlink "$@"; }
awk(){ /usr/bin/awk "$@"; }
sync(){ :; }
sha256sum(){
 case "${1##*/}" in
  state) hash=3bce1c40bc5a6f07b157a73ad0f284edb4434758a84f42c63d1725de3e192134;;
  trial.exit) hash=4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865;;
  trial.stdout) hash=a58a9e280debf17d63dd8b5a9300a48251ca38cd2f3df94cfa0d3bfae3ba2eaf;;
  trial.stderr) hash=1a0a0100639af812b389f109a5cd3164d58ce3f24f67f39caefb8a0ca7508322;;
  driver125) hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099;;
  tenstorrent_pci_probe) if [ "$(cat "$1")" = STABLE ];then hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403;else hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099;fi;;
  tenstorrent_pci_probe-scale-reuse-qualification-041fef18|recovery.incoming) hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403;;
  pioneer-p100a-reload) hash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f;;
  pioneer_p100a_reload_guard) hash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229;;
  HaikuAssistantQwen27) hash=861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74;;
  HaikuAssistantService) hash=c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18;;
  *) return 1;;
 esac
 if [ "$SIM_MODE" = bad-hash ] && [ "${1##*/}" = trial.stdout ];then hash=WRONG;fi
 printf '%s  %s\\n' "$hash" "$1"
}
'''
with tempfile.TemporaryDirectory(prefix='pioneer-hybrid125-gates-') as directory:
 base=Path(directory)
 for mode in ('pass','bad-hash','no-attestation','bad-attestation','bad-health','busy','cold','owner','wrong-tag','wrong-generation','spent'):
  root=base/mode/'home';stage=root/'pioneer-hybrid125-full-trial-20261107-a';stage.mkdir(parents=True)
  (stage/'.trial-once').mkdir()
  for name in ('state','trial.exit','trial.stdout','trial.stderr','driver125'):(stage/name).write_text('TEST DATA')
  if mode!='no-attestation':(stage/'fresh-epoch-attestation').write_text('WRONG\n' if mode=='bad-attestation' else 'OFF_ON_VERIFIED_FRESH_HAIKU_BOOT\n')
  if mode=='spent':(stage/'recovery-once').mkdir()
  installed=root/'config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe';installed.parent.mkdir(parents=True);installed.write_text('NATIVE')
  device=root/'config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe';device.parent.mkdir(parents=True);device.symlink_to('../../bin/tenstorrent_pci_probe')
  guard=root/'config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard';guard.parent.mkdir(parents=True);guard.write_text('GUARD')
  for name in ('HaikuAssistantQwen27','HaikuAssistantService'):
   item=root/'config/non-packaged/servers'/name;item.parent.mkdir(parents=True,exist_ok=True);item.write_text('ASSISTANT')
  stable=root/'pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18';stable.parent.mkdir(parents=True);stable.write_text('STABLE')
  admin=stable.parent/'pioneer-p100a-reload';admin.write_text('''#!/bin/sh
echo "$1" >> "$SIM_EVENTS"
case "$1" in
 --status)
  tag=2026110700010125;generation=1;cold=0;owner=0
  case "$SIM_MODE" in cold)cold=1;; owner)owner=1;; wrong-tag)tag=2026102100010012;; wrong-generation)generation=2;; esac
  printf 'P100A_RELOAD driver55 tag%s generation%s quiesced0 cold%s active_owner%s open_count1 guard1;CPU_METADATA_ONLY\\n' "$tag" "$generation" "$cold" "$owner";;
 --quiesce) :;;
 --wait) :;;
esac
''');admin.chmod(0o700)
  # The status mock switches to stable metadata only after replacement.
  admin.write_text(admin.read_text().replace('tag=2026110700010125;generation=1;cold=0;owner=0',
   'tag=2026110700010125;generation=1;cold=0;owner=0\n  if [ "$(cat "'+str(installed)+'")" = STABLE ];then tag=2026102100010012;generation=2;fi'))
  program=base/mode/'run.sh';program.write_text(source.replace('set -eu\n','set -eu\n'+shim,1).replace('/boot/home',str(root)))
  events=base/mode/'events';events.write_text('')
  result=subprocess.run(['sh',str(program)],env={**os.environ,'SIM_MODE':mode,'SIM_EVENTS':str(events)},capture_output=True,text=True)
  calls=events.read_text().splitlines()
  if mode=='pass':
   assert result.returncode==0,(result.stderr,calls)
   assert calls==['--status','--quiesce','--wait','--status'] and installed.read_text()=='STABLE'
   assert (stage/'recovery-once').is_dir()
  else:
   assert result.returncode!=0,(mode,result.stdout,calls)
   assert '--quiesce' not in calls and '--wait' not in calls and installed.read_text()=='NATIVE'
   if mode=='spent':assert not calls # no status query after any previous attempt
   if mode!='spent':assert not (stage/'recovery-once').exists()
print('PASS actual fresh-recovery gates: 11 fake scenarios; no Pioneer, SSH, power or device access')
