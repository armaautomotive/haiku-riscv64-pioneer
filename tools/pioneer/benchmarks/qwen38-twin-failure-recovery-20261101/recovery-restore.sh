set -eu
test "$(uname -s)" = Haiku
test "$(id -u)" = 0
recovery_stage=/boot/home/pioneer-p100a-hot-reload-20261019-a
recovery_installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
exact() {
 test -f "$1" && test ! -L "$1"
 recovery_digest=$(sha256sum "$1")
 printf '%s\n' "$recovery_digest"
 test "${recovery_digest%% *}" = "$2"
}
recovery_sysinfo=$(sysinfo)
printf '%s\n' "$recovery_sysinfo"
printf '%s\n' "$recovery_sysinfo" | awk '
 /^64 .*running at/ {cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/ {ram++}
 END {if(cpus!=1 || ram!=1) exit 1}'
recovery_teams=$(ps -o Id -o Team)
printf '%s\n' "$recovery_teams"
printf '%s\n' "$recovery_teams" | awk '
 /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ {owner++}
 END {if(owner) exit 1}'
exact "$recovery_installed" 010c184ef474f962205918fd9a9a34b23b2ab47c3cf6a62d285fcb36d2e9e899
exact "$recovery_stage/tenstorrent_pci_probe-scale-reuse-qualification-041fef18" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$recovery_stage/pioneer-p100a-hot-swap-twin-batch-prefetch-6c3215d8.sh" 6c3215d8186ebaf9198c60fb8a2ac2a02489ad08bdc7bf37df855469c638881f
exact "$recovery_stage/pioneer-p100a-reload" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
recovery_status=$("$recovery_stage/pioneer-p100a-reload" --status)
printf '%s\n' "$recovery_status"
test "$recovery_status" = 'P100A_RELOAD driver55 tag2026103100010075 generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
echo CLEAN_NEW_EPOCH_GEN1_BEFORE_EXPLICIT_RESTORE_ONLY
/bin/sh "$recovery_stage/pioneer-p100a-hot-swap-twin-batch-prefetch-6c3215d8.sh" --restore-token-twin-batch-prefetch-benchmark
exact "$recovery_installed" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
recovery_status=$("$recovery_stage/pioneer-p100a-reload" --status)
printf '%s\n' "$recovery_status"
test "$recovery_status" = 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
echo RESTORED041_GEN2_HEALTHY_METADATA_NO_COMPUTE_OR_RESET
