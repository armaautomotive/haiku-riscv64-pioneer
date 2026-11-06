#!/bin/sh
# AFTER actual protected parent process exit only. Read-only independent audit.
set -eu
stage=/boot/home/pioneer-native-compact-q4-pair-issue108-a
test "$(uname -s)" = Haiku && test "$(id -u)" = 0
test "$(cat "$stage/direct-parent.exit")" = 0
test "$(cat "$stage/compact-q4-stream.exit")" = 0
test "$(cat "$stage/state")" = STABLE041_RESTORED_COMPACT_Q4_PAIR_ISSUE_EXACT128
test ! -s "$stage/direct-parent.stderr" && test ! -s "$stage/compact-q4-stream.stderr"
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v self="$$" '
$1=="Id" && $2=="Team" && NF==2{header++;next}
$1 ~ /^[0-9]+$/ && NF>=2{teams++;if($1==38711 || $1==38712 || $1==38791)bad++;if($1==self)next;
 if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/)bad++;next}
NF{bad++}END{if(header!=1 || !teams || bad)exit 1;print "ALL108_PIDS_ABSENT_NO_MATCHING_OWNER"}'
exact(){ test -f "$1" && test ! -L "$1";value=$(sha256sum "$1");printf '%s\n' "$value";test "${value%% *}" = "$2"; }
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
test "$status" = 'P100A_RELOAD driver55 tag2026102100010012 generation46 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
value=$(sysinfo);printf '%s\n' "$value"
printf '%s\n' "$value" | awk '/^64 .*running at/{cpu++}/used\/max [0-9]+ \/ 137034936320\)/{ram++}END{if(cpu!=1 || ram!=1)exit 1}'
test "$(stat -c '%u:%h:%a:%s' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf)" = 0:1:600:18973870528
exact "$stage/tenstorrent_pci_probe-native-q4-fpu-stream108" d6d457fdb4a5535d84a82ae19b138d156ef295a0dcb66142274580a4bc78bf52
exact "$stage/pioneer-native-compact-q4-pair-issue-diagnostic" cc57303dda69059a7d9762eee56ded50e2c68d77035a25357921d8ccd9de3cad
exact "$stage/pioneer_native_compact_q4_pair_issue_physical.sh" b1df413ac0665e7c166786b1dc8eb372a953a422eb326cebae06cc22b3f5e26b
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
for name in direct-parent.stdout direct-parent.stderr direct-parent.exit compact-q4-stream.stdout compact-q4-stream.stderr compact-q4-stream.exit state parent.pid direct-launcher.pid;do
 test -f "$stage/$name" && test ! -L "$stage/$name";sha256sum "$stage/$name"
done
printf '%s\n' PASS_INDEPENDENT108_NORMAL041_GEN46_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED
