#!/bin/sh
# ONLY after OFF-once/ON-once and a fresh Linux->Haiku boot.
# New-epoch82 restoration,NEVER cleanup/END/reset of the retained old lease.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-dram-load-only-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate_hash=3df4fdb1d034c39df5c24bc06735012109d3881431c5a9ddaedd16ecc535d3d6
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 printf '%s\n' "$digest" || return 1
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-native-model|pioneer-native-dram|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value" || return 1
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
# NATIVE_LOAD_FRESH_PRIOR_BEGIN
for name in state load.exit load.stdout load.stderr;do
 test -f "$stage/$name" && test ! -L "$stage/$name" && [ "$(stat -c '%u:%h:%a' "$stage/$name")" = 0:1:600 ] || exit 1
done
[ "$(cat "$stage/state")" = UNPROVEN_LOAD_ONLY_NO_FURTHER_END_RESET_RETRY_RESTORE ] || exit 1
[ "$(cat "$stage/load.exit")" = 1 ] || exit 1
digest=$(sha256sum "$stage/load.stdout") || exit 1
[ "${digest%% *}" = 63f6c0619cb2769932daf9d8b83d644076a70c953a867ab061c6135ec597d99d ] || exit 1
digest=$(sha256sum "$stage/load.stderr") || exit 1
[ "${digest%% *}" = f21f376681d8ab635a7df556cc6f231f3ba52073f92668c27a3922981094da7e ] || exit 1
# NATIVE_LOAD_FRESH_PRIOR_END
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$installed" "$candidate_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$stage/pioneer_native_dram_load_physical.sh" f6859f95a58524cca324e0f150b358c63d2656e8af12eb3b71af1582726a1ba9 || exit 1
exact "$stage/pioneer-native-dram-load-diagnostic" f7ae6e1be5fbcf6618ddd4326715c49166169a0073f868a86ec353519bc4f719 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
health=$(sysinfo) || exit 1
printf '%s\n' "$health"
printf '%s\n' "$health" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}' || exit 1
idle && metadata 2026110400010082 1 || exit 1
for name in recovery-once recovery.incoming recovery.state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
# NATIVE_LOAD_FRESH_RESTORE_FLOW_BEGIN
mkdir "$stage/recovery-once" || exit 1
printf '%s\n' FRESH_EPOCH_ADMITTED_BEFORE_RESTORE > "$stage/recovery.state" || exit 1
cp -p "$stable" "$stage/recovery.incoming" || exit 1
chmod 755 "$stage/recovery.incoming" || exit 1
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" && idle || exit 1
"$admin" --quiesce 2026110400010082 || exit 1
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$stage/recovery.incoming" "$installed" || exit 1
sync || exit 1
exact "$installed" "$stable_hash" || exit 1
"$admin" --wait 2026102100010012 0000000000000001 || exit 1
metadata 2026102100010012 2 && idle || exit 1
printf '%s\n' NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED > "$stage/recovery.state" || exit 1
echo NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION || exit 1
# NATIVE_LOAD_FRESH_RESTORE_FLOW_END
