#!/bin/sh
# Restore stable driver only after the bounded tag206 readiness timeout,
# the client and worker have exited, and no card work remains owned.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-q38-gdn-l1-tag206-20261116-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate_hash=1c2ad62853c4617690956e083da8c364d27c5f1399fdc9509f8c6f781684e475
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
script_hash=fb42bb9fb317c6492ae9df16589d85ac992784dd5d5ea0a58e392266578615a8
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555|0:1:600) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
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
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once" || exit 1
[ "$(stat -c '%u:%a:%s' "$stage/state")" = 0:600:41 ] || exit 1
[ "$(cat "$stage/state")" = FULL_CARD_RATE32_ACTIVE_DO_NOT_INTERVENE ] || exit 1
[ -f "$stage/rate.exit" ] && [ ! -L "$stage/rate.exit" ] && [ "$(stat -c '%u:%h:%a:%s' "$stage/rate.exit")" = 0:1:600:2 ] || exit 1
[ "$(cat "$stage/rate.exit")" = 1 ] || exit 1
[ -f "$stage/rate.stdout" ] && [ ! -L "$stage/rate.stdout" ] && [ "$(stat -c '%u:%h:%a:%s' "$stage/rate.stdout")" = 0:1:600:191 ] || exit 1
[ "$(cat "$stage/rate.stdout")" = "FULL_OFFLOAD_PROGRESS id1 value4294967295 Authenticating Qwen27B; CPU will load vocabulary only,not model weights or an inference context.
FAIL_FULL_OFFLOAD_RATE_READY;CLOSE_ONLY_NO_FALLBACK" ] || exit 1
[ "$(stat -c '%u:%h:%a:%s' "$stage/rate.stderr")" = 0:1:600:0 ] || exit 1
exact "$stage/stable041.backup" "$stable_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$stage/pioneer_q38_gdn_l1_tag206_physical.sh" "$script_hash" || exit 1
exact "$installed" "$candidate_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 93f1d09ba0b2e0d0ea73642ccc89bdc5d0bf0cbed4be0ce3bbd47ea94d24008f || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
health=$(sysinfo) || exit 1
printf '%s\n' "$health"
printf '%s\n' "$health" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}' || exit 1
idle && metadata 2026111500010206 3 || exit 1
for name in recovery-once restore.incoming recovery.state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
mkdir "$stage/recovery-once" || exit 1
printf '%s\n' RATE_READY_TIMEOUT_NO_GPU_INFERENCE_RESTORE_STARTED > "$stage/recovery.state" || exit 1
cp -p "$stage/stable041.backup" "$stage/restore.incoming" || exit 1
chmod 755 "$stage/restore.incoming" || exit 1
exact "$stage/restore.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" && idle || exit 1
"$admin" --quiesce 2026111500010206 || exit 1
exact "$stage/restore.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$stage/restore.incoming" "$installed" || exit 1
sync || exit 1
exact "$installed" "$stable_hash" || exit 1
"$admin" --wait 2026102100010012 0000000000000003 || exit 1
metadata 2026102100010012 4 && idle || exit 1
printf '%s\n' RATE_READY_TIMEOUT_NO_GPU_INFERENCE_STABLE041_GEN4_RESTORED > "$stage/recovery.state" || exit 1
echo RATE_READY_TIMEOUT_NO_GPU_INFERENCE_STABLE041_GEN4_RESTORED
