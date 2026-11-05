#!/bin/sh
# READ ONLY. Refuse before any device metadata call while protected jobs live.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
trial_dir=/boot/home/pioneer-q38-full-two-product-replay-q4-20261105-a
[ "$(cat "$trial_dir/parent.exit")" = 0 ] && [ "$(cat "$trial_dir/rate.exit")" = 0 ]
[ "$(cat "$trial_dir/state")" = FULL_CARD_RATE32_CLOSED_STABLE_RESTORED ]
test ! -s "$trial_dir/rate.stderr" && test ! -s "$trial_dir/physical-parent.stderr"
# Snapshot before starting awk,as in the tested physical parent. Otherwise
# ps includes awk's multiline program argument as apparent extra rows.
trial_listing=$(ps -o Id -o Team)
printf '%s\n' "$trial_listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {headers++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($1==16500 || $1==16501 || $1==16585 || $1==16586)bad++
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)bad++
  next
 }
 NF{bad++} END{if(headers!=1 || !teams || bad)exit 1}'
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 trial_digest=$(sha256sum "$1")
 [ "${trial_digest%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$trial_dir/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
trial_admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
exact "$trial_admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933
trial_runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact "$trial_runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af
exact "$trial_runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16
exact "$trial_runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389
exact "$trial_runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2
trial_status=$("$trial_admin" --status)
printf '%s\n' "$trial_status"
[ "$trial_status" = 'P100A_RELOAD driver55 tag2026102100010012 generation30 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
trial_health=$(sysinfo)
printf '%s\n' "$trial_health"
printf '%s\n' "$trial_health" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
printf '%s\n' PASS_READONLY_POSTFLIGHT_parent_and_child_actual0_protected_pids_absent_idle_stable041_gen30_apps_runtime_guard_64CPU_fullRAM_preserved
