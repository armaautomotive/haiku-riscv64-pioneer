#!/bin/sh
# Independent read-only check, only AFTER the primary comparison is terminal.
set -eu
stage=/boot/home/pioneer-native-factored-q4-compare111-a
test "$(cat /boot/home/pioneer-native-factored-q4-recovery111-a/recovery.state)" = NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED
test ! -e /boot/home/pioneer-native-factored-q4-recovery111-a/recovery.incoming
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v parent=44029 -v launcher=44024 '
 $1==parent || $1==launcher || $1==44113 {bad++}
 $0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q38|pioneer-native-|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ {bad++}
 END {if(bad)exit 1}'
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1")
 printf '%s\n' "$digest"
 test "${digest%% *}" = "$2"
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$stage/tenstorrent_pci_probe-native-q4-fpu-stream111" 1050163f12ecf72ffe123cea91451dbb8995fcedc2df12e5fe96083e4dd9438d
exact "$stage/pioneer-native-factored-q4-compare" f8a1b2ec227aaae16f3d74b07aa03c36294e6002b08389cdbfc7714100362934
exact "$stage/pioneer_native_factored_q4_compare_physical.sh" f90b5288feb08039e65a8d6afd646c206bf7aa5bc56c1d088aa24cd3787c6bdf
exact "$stage/launch.sh" c9a61e7c14ceedeef4697cec9028689a2453d4ba71288012f1e44bdf53bf5d5b
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af
exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16
exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389
exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original"
test "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528
metadata=$("$admin" --status)
printf '%s\n' "$metadata"
test "$metadata" = 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
info=$(sysinfo)
printf '%s\n' "$info"
printf '%s\n' "$info" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
sha256sum /boot/home/pioneer-native-factored-q4-recovery111-a/recovery.state
sha256sum "$stage/parent.stdout" "$stage/parent.stderr" "$stage/factored-q4-compare.stdout" "$stage/factored-q4-compare.stderr" "$stage/state" "$stage/parent.exit" "$stage/factored-q4-compare.exit" "$stage/parent.pid" "$stage/launcher.pid"
date -u
echo NATIVE111_FRESH_POWER_RECOVERY_INDEPENDENT_NORMAL2_IDLE64_FULLRAM_APPS_PRESERVED_NOT_TRIAL_END
