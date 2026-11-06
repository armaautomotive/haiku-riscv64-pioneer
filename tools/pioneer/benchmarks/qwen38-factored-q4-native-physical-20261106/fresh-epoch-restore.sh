#!/bin/sh
# ONLY after separately recorded OFF-once/ON-once and fresh Linux->Haiku boot.
# Fresh epoch only;NOT failed111's cleanup/END/reset.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-factored-q4-compare111-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate_hash=1050163f12ecf72ffe123cea91451dbb8995fcedc2df12e5fe96083e4dd9438d
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 printf '%s\n' "$digest"
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
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
test -f "$stage/state" && test ! -L "$stage/state" && [ "$(stat -c '%u:%h' "$stage/state")" = 0:1 ] || exit 1
[ "$(cat "$stage/state")" = UNPROVEN_FACTORED_Q4_COMPARE_NO_END_RESET_RETRY_RESTORE ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$installed" "$candidate_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$stage/pioneer_native_factored_q4_compare_physical.sh" f90b5288feb08039e65a8d6afd646c206bf7aa5bc56c1d088aa24cd3787c6bdf || exit 1
exact "$stage/pioneer-native-factored-q4-compare" f8a1b2ec227aaae16f3d74b07aa03c36294e6002b08389cdbfc7714100362934 || exit 1
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
idle && metadata 2026110600010111 1 || exit 1
for name in recovery-once recovery.incoming recovery.state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
# NATIVE_BFP8_FRESH_RESTORE_FLOW_BEGIN
mkdir "$stage/recovery-once"
printf '%s\n' FRESH_EPOCH_ADMITTED_BEFORE_RESTORE > "$stage/recovery.state"
cp -p "$stable" "$stage/recovery.incoming"
chmod 755 "$stage/recovery.incoming"
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" && idle || exit 1
"$admin" --quiesce 2026110600010111 || exit 1
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$stage/recovery.incoming" "$installed"
sync
exact "$installed" "$stable_hash" || exit 1
"$admin" --wait 2026102100010012 0000000000000001 || exit 1
metadata 2026102100010012 2 && idle || exit 1
printf '%s\n' NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED > "$stage/recovery.state"
echo NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION
# NATIVE_BFP8_FRESH_RESTORE_FLOW_END
