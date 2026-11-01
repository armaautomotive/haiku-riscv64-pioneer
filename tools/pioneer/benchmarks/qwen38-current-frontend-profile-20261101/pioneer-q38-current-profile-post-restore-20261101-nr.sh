set -eu
test "$(uname -s)" = Haiku
test "$(id -u)" = 0
profile_stage=/boot/home/pioneer-p100a-hot-reload-20261019-a
profile_installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
profile_guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
exact() {
 test -f "$1" && test ! -L "$1"
 profile_digest=$(sha256sum "$1")
 printf '%s\n' "$profile_digest"
 test "${profile_digest%% *}" = "$2"
}
sysinfo
profile_teams=$(ps -o Id -o Team)
printf '%s\n' "$profile_teams"
printf '%s\n' "$profile_teams" | awk '
 $1=="Id" && $2=="Team" && NF==2 {headers++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;next}
 NF {invalid++}
 END {if(headers!=1 || !teams || invalid) exit 1}'
printf '%s\n' "$profile_teams" | awk '
 /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ {owner++}
 END {if(owner) exit 1}'
exact "$profile_installed" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$profile_stage/tenstorrent_pci_probe-token-prefetch-benchmark-919d4a63" 919d4a631ff664549a67bb2372ce2db344ba29473c452c0b273c4b27a2bdf978
exact "$profile_stage/pioneer-p100a-hot-swap-prefetch-3ceff737.sh" 3ceff7375c872bbcb2394c751ad8200c845cc6a6b62d436b7e99812608df0698
exact "$profile_stage/tenstorrent_pci_probe-scale-reuse-qualification-041fef18" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$profile_stage/pioneer-q38-FRONTIER34-BENCHMARK-4b1dbee6" 4b1dbee63e537b39e5212a9180670139a2a9ba46ef0b3cbeb66a7d5226b7f048
exact "$profile_stage/pioneer-q38-FRONTEND34-PROFILE-718d0cea" 718d0ceac7eb5caad771413aefa32903fabf1e81b3dc6ad34793fd8ad0b7b77c
exact "$profile_stage/pioneer-p100a-reload" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact "$profile_guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
profile_status=$("$profile_stage/pioneer-p100a-reload" --status)
printf '%s\n' "$profile_status"
test "$profile_status" = 'P100A_RELOAD driver55 tag2026102100010012 generation36 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
echo CURRENT_PROFILE_RESTORED_EXACT041_GEN36_HEALTHY_IDLE_NO_GPU_EXECUTION
