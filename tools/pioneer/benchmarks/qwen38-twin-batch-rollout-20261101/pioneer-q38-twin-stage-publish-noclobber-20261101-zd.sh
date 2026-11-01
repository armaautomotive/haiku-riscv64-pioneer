set -eu
test "$(uname -s)" = Haiku
test "$(id -u)" = 0
batch_stage=/boot/home/pioneer-p100a-hot-reload-20261019-a
batch_installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
batch_guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
exact() {
 test -f "$1" && test ! -L "$1"
 batch_digest=$(sha256sum "$1")
 printf '%s\n' "$batch_digest"
 test "${batch_digest%% *}" = "$2"
}
test -d "$batch_stage" && test ! -L "$batch_stage"
batch_sysinfo=$(sysinfo)
printf '%s\n' "$batch_sysinfo"
printf '%s\n' "$batch_sysinfo" | awk '
 /^64 .*running at/ {cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/ {ram++}
 END {if(cpus!=1 || ram!=1) exit 1}'
batch_teams=$(ps -o Id -o Team)
printf '%s\n' "$batch_teams"
printf '%s\n' "$batch_teams" | awk '
 $1=="Id" && $2=="Team" && NF==2 {headers++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;next}
 NF {invalid++}
 END {if(headers!=1 || !teams || invalid) exit 1}'
printf '%s\n' "$batch_teams" | awk '
 /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ {owner++}
 END {if(owner) exit 1}'
exact "$batch_installed" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$batch_stage/tenstorrent_pci_probe-scale-reuse-qualification-041fef18" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$batch_stage/pioneer-q38-FRONTIER34-BENCHMARK-4b1dbee6" 4b1dbee63e537b39e5212a9180670139a2a9ba46ef0b3cbeb66a7d5226b7f048
exact "$batch_stage/pioneer-p100a-reload" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact "$batch_guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
batch_status=$("$batch_stage/pioneer-p100a-reload" --status)
printf '%s\n' "$batch_status"
test "$batch_status" = 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
exact "$batch_stage/tenstorrent_pci_probe-token-batched-prefetch-benchmark-34322035" 34322035d8aadcc8667d8a7ec3f2a144646f0060604e9e00ebfdd59fa7e4ca1c
exact "$batch_stage/pioneer-p100a-hot-swap-batched-prefetch-33d69062.sh" 33d6906295813080dfa1db2950ec3170b79cb2f5e2cf2d9dedab9810fb0fbd08
echo BATCHED_RECOVERY_STABLE041_33D_GEN2_HEALTHY_IDLE_64CPU_FULLRAM_NO_GPU_EXECUTION

twin_dir=$batch_stage/twin-batch-prefetch-010c184e-6c3215d8-20261101
twin_module=tenstorrent_pci_probe-token-twin-batch-prefetch-benchmark-010c184e
twin_helper=pioneer-p100a-hot-swap-twin-batch-prefetch-6c3215d8.sh
twin_probe=$twin_dir/NONEXECUTABLE-NOCLOBBER-COPY-PROBE
twin_link=$twin_dir/NONEXECUTABLE-NOCLOBBER-LINK-PROBE
test -d "$twin_dir" && test ! -L "$twin_dir"
exact "$twin_dir/$twin_module" 010c184ef474f962205918fd9a9a34b23b2ab47c3cf6a62d285fcb36d2e9e899
exact "$twin_dir/$twin_helper" 6c3215d8186ebaf9198c60fb8a2ac2a02489ad08bdc7bf37df855469c638881f
exact "$batch_stage/frontier-failed-reply-f8efe67a-20261101/pioneer-q38-FRONTIER34-FAILED-REPLY-DIAGNOSTIC-ONLY" f8efe67a51d9685a9e7e3ce57267ed2ae4967600c2802dd21f5f6ee0c24a22c9
test ! -e "$batch_stage/$twin_module" && test ! -L "$batch_stage/$twin_module"
test ! -e "$batch_stage/$twin_helper" && test ! -L "$batch_stage/$twin_helper"
test ! -e "$twin_probe" && test ! -L "$twin_probe"
test ! -e "$twin_link" && test ! -L "$twin_link"
# On disposable own stage artifacts,prove that -nT preserves an existing
# different regular target AND refuses overwriting through a symlink.
cp -nT -- "$twin_dir/$twin_helper" "$twin_probe"
exact "$twin_probe" 6c3215d8186ebaf9198c60fb8a2ac2a02489ad08bdc7bf37df855469c638881f
cp -nT -- "$twin_dir/$twin_module" "$twin_probe"
exact "$twin_probe" 6c3215d8186ebaf9198c60fb8a2ac2a02489ad08bdc7bf37df855469c638881f
ln -s "$twin_probe" "$twin_link"
cp -nT -- "$twin_dir/$twin_module" "$twin_link"
test -L "$twin_link" && test "$(readlink "$twin_link")" = "$twin_probe"
exact "$twin_probe" 6c3215d8186ebaf9198c60fb8a2ac2a02489ad08bdc7bf37df855469c638881f
echo NOCLOBBER_COPY_ACTUAL_HAIKU_DIFFERENT_FILE_AND_LINK_TARGET_UNCHANGED
cp -nT -- "$twin_dir/$twin_module" "$batch_stage/$twin_module"
cp -nT -- "$twin_dir/$twin_helper" "$batch_stage/$twin_helper"
exact "$batch_stage/$twin_module" 010c184ef474f962205918fd9a9a34b23b2ab47c3cf6a62d285fcb36d2e9e899
exact "$batch_stage/$twin_helper" 6c3215d8186ebaf9198c60fb8a2ac2a02489ad08bdc7bf37df855469c638881f
chmod 555 "$batch_stage/$twin_module" "$batch_stage/$twin_helper"
echo TWIN010C_6C32_STAGED_BYTE_VERIFIED_NOT_INSTALLED_STABLE041_GEN2_NO_GPU_EXECUTION

