#!/bin/sh
# AFTER actual parent/waiter exit0 ONLY. Read-only;never exchange or retry.
set -eu
stage=/boot/home/pioneer-q38-bf16-deferred-110-a
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
date -u
test "$(cat "$stage/direct-parent.exit")" = 0
test "$(cat "$stage/rate.exit")" = 0
test "$(cat "$stage/state")" = FULL_CARD_RATE32_CLOSED_STABLE_RESTORED
for name in direct-parent.stderr rate.stderr;do test -f "$stage/$name" && test ! -s "$stage/$name";done
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v self="$$" '
$1=="Id" && $2=="Team" && NF==2 {header++;next}
$1 ~ /^[0-9]+$/ && NF>=2 {
 teams++;if($1==41588 || $1==41593 || $1==41677 || $1==41678)bad++
 if($1==self)next
 if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
  $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)bad++
 next
}
NF{bad++}END{if(header!=1 || !teams || bad)exit 1;print "ALL110_TRIAL_PIDS_ABSENT_NO_MATCHING_OWNER"}'
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1");printf '%s\n' "$digest";[ "${digest%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af
exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16
exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389
exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2
status=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$status"
[ "$status" = 'P100A_RELOAD driver55 tag2026102100010012 generation50 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
value=$(sysinfo);printf '%s\n' "$value"
printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++}/used\/max [0-9]+ \/ 137034936320\)/{ram++}END{if(cpus!=1 || ram!=1)exit 1}'
test "$(stat -c '%u:%h:%a:%s' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf)" = 0:1:600:18973870528
exact "$stage/tenstorrent_pci_probe-bf16-deferred" fab6d0e741cbb06ab540af07d4d1d4864cf84ec29eaf02b5048d11464bc771e9
exact "$stage/HaikuAssistantQwen27Bf16DeferredFullOffload" 4b526caefad509819b6453ebc99e0199e7e5e6ad079534f76e848cfe66706d00
exact "$stage/pioneer-q38-bf16-deferred-rate-smoke" 47eb2c07d49e540db8c6bd02a38c6cad9dac8147ddeb60e83d17b89c75a33dbd
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$stage/pioneer_q38_bf16_deferred_full_offload_physical.sh" b1a7c8ac9d830fe78bc3b1174e48bd694499fc49d16dcd61aa118f2b8459e1dc
for name in direct-parent.stdout direct-parent.stderr direct-parent.exit rate.stdout rate.stderr rate.exit state parent.pid direct-launcher.pid;do
 test -f "$stage/$name" && test ! -L "$stage/$name"
 sha256sum "$stage/$name"
done
printf '%s\n' PASS_INDEPENDENT_DEFERRED_110_CLOSED_NORMAL041_GEN50_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED
