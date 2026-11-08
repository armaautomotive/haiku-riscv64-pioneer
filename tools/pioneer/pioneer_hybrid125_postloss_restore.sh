#!/bin/sh
# Recovery of the pinned stable driver after the separately recorded loss of
# trial-log contents. This does not change the original guard or trial stage,
# launch a model, retry a trial, reset the card, or auto-recover after failure.
# The backup argument is an OPERATOR ATTESTATION, not proof that it exists;
# verify both snapshot services and their pinned full-disk SHA independently
# on Linux before invoking this script in the documented fresh power epoch.
set -eu
PATH=/boot/system/bin:/bin
export PATH
backup=5f6cea34aa32016fab6703f2ef6ef8ca9e7d52beb5a2f200d26a9b1c9ef2239d
[ "$#" = 2 ] && [ "$1" = --attest-verified-backup ] && [ "$2" = "$backup" ] \
 && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-hybrid125-full-trial-20261107-a
archive=/HaikuData/pioneer-hybrid125-evidence-ssqFWSMo
work=/boot/home/pioneer-hybrid125-postloss-restore-20261107-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
candidate_hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1; [ "${digest%% *}" = "$2" ]
}
exact_data(){
 test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h:%a' "$1")" = 0:1:600 ] || return 1
 digest=$(sha256sum "$1") || return 1; [ "${digest%% *}" = "$2" ]
}
private_dir(){ test -d "$1" && test ! -L "$1" && [ "$(stat -c '%u:%a' "$1")" = 0:700 ]; }
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/ || $0 ~ /(upload-test|preflight-test|full-test)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1; printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
check_records(){
 for prefix in "$stage" "$archive/post-power"; do
  exact_data "$prefix/state" 094397c8680a2bb50de5ca274f1468d53861415c400bb0057af66c8d7a3f5591 && \
  exact_data "$prefix/trial.stdout" cc67398cdf4d67e740c063928b2253e4e06afae79f35e9bfd5cd768e873d23aa && \
  exact_data "$prefix/trial.stderr" 97224f9450e7d5ed6fc6e071d7a17fe39a90a368a8855f182cebac9bfa7e1cfb && \
  exact_data "$prefix/trial.exit" bacddcf184771ace7c730d0d0b27d0ec723c9187adf8f3233d39190a5ba27ff3 || return 1
 done
 exact_data "$archive/pre-power/state" 3bce1c40bc5a6f07b157a73ad0f284edb4434758a84f42c63d1725de3e192134 && \
 exact_data "$archive/pre-power/trial.stdout" a58a9e280debf17d63dd8b5a9300a48251ca38cd2f3df94cfa0d3bfae3ba2eaf && \
 exact_data "$archive/pre-power/trial.stderr" 1a0a0100639af812b389f109a5cd3164d58ce3f24f67f39caefb8a0ca7508322 && \
 exact_data "$archive/pre-power/trial.exit" 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865
}
private_dir "$stage" && private_dir "$archive" && private_dir "$archive/pre-power" && private_dir "$archive/post-power" || exit 1
test -d /HaikuData && test ! -L /HaikuData || exit 1
boot_device=$(stat -c '%d' /boot/home) || exit 1
archive_device=$(stat -c '%d' /HaikuData) || exit 1
[ "$boot_device" != "$archive_device" ] && [ "$(stat -c '%d' "$stage")" = "$boot_device" ] && \
 [ "$(stat -c '%d' "$archive")" = "$archive_device" ] && \
 [ "$(stat -c '%d' "$archive/pre-power")" = "$archive_device" ] && \
 [ "$(stat -c '%d' "$archive/post-power")" = "$archive_device" ] || exit 1
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once" || exit 1
test -f "$stage/fresh-epoch-attestation" && test ! -L "$stage/fresh-epoch-attestation" && \
 [ "$(stat -c '%u:%h:%a' "$stage/fresh-epoch-attestation")" = 0:1:600 ] && \
 [ "$(cat "$stage/fresh-epoch-attestation")" = OFF_ON_VERIFIED_FRESH_HAIKU_BOOT ] || exit 1
for name in recovery-once recovery.incoming recovery.state; do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
test ! -e "$work" && test ! -L "$work" || exit 1
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
check_records && exact "$stage/driver125" "$candidate_hash" && exact "$installed" "$candidate_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f && \
 exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 && \
 exact "$flush" 242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 && \
 exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
[ "$(stat -c '%d' /boot/home)" = "$(stat -c '%d' "$installed")" ] || exit 1
health=$(sysinfo) || exit 1
printf '%s\n' "$health" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}' || exit 1
idle && metadata 2026110700010125 1 || exit 1

# A new exclusive stage is spent even if any later operation fails. The old
# trial and its own refusal guard are never modified. No automatic retry or
# rollback follows an uncertain exchange.
mkdir "$work"
trap 'code=$?; if [ "$code" -ne 0 ]; then printf "RESTORE_FAILED_%s_NO_RETRY_RESET_OR_ROLLBACK\n" "$code" >&2; fi' EXIT
cp -p "$installed" "$work/previous125"
cp -p "$stable" "$work/incoming041"
chmod 755 "$work/incoming041"
exact "$work/previous125" "$candidate_hash" && exact "$work/incoming041" "$stable_hash" && \
 exact "$installed" "$candidate_hash" && check_records && idle || exit 1
"$flush" --flush-only /dev/disk/scsi/0/3/0/0 || exit 1
"$admin" --quiesce 2026110700010125 || exit 1
exact "$work/incoming041" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$work/incoming041" "$installed"
exact "$installed" "$stable_hash" && exact "$work/previous125" "$candidate_hash" || exit 1
"$flush" --flush-only /dev/disk/scsi/0/3/0/0 || exit 1
"$admin" --wait 2026102100010012 0000000000000001 || exit 1
metadata 2026102100010012 2 && idle && check_records || exit 1
printf '%s\n' POSTLOSS_STABLE041_RESTORED_NO_ACCELERATOR_EXECUTION
