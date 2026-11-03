#!/bin/sh
# One-shot, CPU-metadata-only restore AFTER the recorded board power cycle.
# This is not a quarantine clear, GPU reset or failed-owner cleanup.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-bfp-physical-20261103-9V2ncOLK
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
candidate_hash=e79e070b9e4fe434b868898748834e0cd89b03bcb250d9c1694b4d0b890f46ac
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact() {
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 printf '%s\n' "$digest"
 [ "${digest%% *}" = "$2" ]
}
idle() {
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/) owners++
  next
 }
 NF{bad++}
 END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata() {
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 [ "$current" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -L /boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe || exit 1
[ "$(readlink /boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe)" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$installed" "$candidate_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
health=$(sysinfo) || exit 1
printf '%s\n' "$health"
printf '%s\n' "$health" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}' || exit 1
idle && metadata 2026110300010078 1 || exit 1
test ! -e "$stage/recovery-once" && test ! -L "$stage/recovery-once" || exit 1
test ! -e "$stage/recovery.incoming" && test ! -L "$stage/recovery.incoming" || exit 1
mkdir "$stage/recovery-once"
cp -p "$stable" "$stage/recovery.incoming"
chmod 755 "$stage/recovery.incoming"
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" && idle || exit 1
"$admin" --quiesce 2026110300010078 || exit 1
exact "$stage/recovery.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$stage/recovery.incoming" "$installed"
sync
exact "$installed" "$stable_hash" || exit 1
"$admin" --wait 2026102100010012 0000000000000001 || exit 1
metadata 2026102100010012 2 && idle || exit 1
echo NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION
