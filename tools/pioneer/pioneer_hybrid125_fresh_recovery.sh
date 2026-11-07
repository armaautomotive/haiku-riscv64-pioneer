#!/bin/sh
# ONLY after independently verified OFF-once/ON-once, fresh Linux->Haiku boot,
# and pinned SSH identity. The attestation is a reminder, not proof of power.
# No old-epoch reset, retry, test rerun, implicit card cleanup or GPU launch.
set -eu
PATH=/boot/system/bin:/bin
export PATH
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-hybrid125-full-trial-20261107-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
candidate_hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
exact_data(){
 test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h:%a' "$1")" = 0:1:600 ] || return 1
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
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
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once" || exit 1
# Refuse a spent attempt before even querying driver status.
for name in recovery-once recovery.incoming recovery.state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
exact_data "$stage/state" 3bce1c40bc5a6f07b157a73ad0f284edb4434758a84f42c63d1725de3e192134 || exit 1
exact_data "$stage/trial.exit" 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 || exit 1
exact_data "$stage/trial.stdout" a58a9e280debf17d63dd8b5a9300a48251ca38cd2f3df94cfa0d3bfae3ba2eaf || exit 1
exact_data "$stage/trial.stderr" 1a0a0100639af812b389f109a5cd3164d58ce3f24f67f39caefb8a0ca7508322 || exit 1
test -f "$stage/fresh-epoch-attestation" && test ! -L "$stage/fresh-epoch-attestation" \
 && [ "$(stat -c '%u:%h:%a' "$stage/fresh-epoch-attestation")" = 0:1:600 ] \
 && [ "$(cat "$stage/fresh-epoch-attestation")" = OFF_ON_VERIFIED_FRESH_HAIKU_BOOT ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$installed" "$candidate_hash" && exact "$stage/driver125" "$candidate_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
health=$(sysinfo) || exit 1
printf '%s\n' "$health" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}' || exit 1
idle && metadata 2026110700010125 1 || exit 1
# HYBRID125_FRESH_RESTORE_FLOW_BEGIN
mkdir "$stage/recovery-once" # spent on every outcome
printf '%s\n' FRESH_EPOCH_ADMITTED_BEFORE_RESTORE > "$stage/recovery.state"
cp -p "$stable" "$stage/recovery.incoming"
chmod 755 "$stage/recovery.incoming"
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" && idle || exit 1
"$admin" --quiesce 2026110700010125 || exit 1
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$stage/recovery.incoming" "$installed"
sync
exact "$installed" "$stable_hash" || exit 1
"$admin" --wait 2026102100010012 0000000000000001 || exit 1
metadata 2026102100010012 2 && idle || exit 1
printf '%s\n' NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION > "$stage/recovery.state"
echo NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION
# HYBRID125_FRESH_RESTORE_FLOW_END
