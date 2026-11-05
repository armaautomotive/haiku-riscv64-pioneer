#!/bin/sh
# READ ONLY,ONLY after protected95658 ACTUALLY exits0 and whole reader passes.
# Three arguments are facts pinned from the CLOSED fetch,not liveness or
# execution authority supplied by a state file. NEVER invoke during95658.
set -eu
[ "$#" = 3 ] || exit 2
closed_parent=$1;closed_bytes=$2;closed_sha=$3
case "$closed_parent:$closed_bytes" in *[!0-9:]*|:*|*:) exit 2;; esac
case "$closed_parent" in 0*) exit 2;; esac
case "$closed_bytes" in 0*) exit 2;; esac
[ "$closed_bytes" -le 16777216 ] && [ "${#closed_sha}" = 64 ] || exit 2
case "$closed_sha" in *[!0-9a-f]*) exit 2;; esac
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 value=$(sha256sum "$1")
 printf '%s\n' "$value"
 [ "${value%% *}" = "$2" ]
}
stage=/boot/home/pioneer-native-exact-q4-20261105-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933
exact "$stage/tenstorrent_pci_probe-native-exact-q4-95" 734d64f3daaebda9b67a10909d5a2302a025dae93b3d51ae7ec31ab53914b6a7
exact "$stage/pioneer-native-exact-q4-diagnostic" d3db09a76b66184b033f301bcfc561d4df2469882a364c63f382fa87f9b9171f
exact "$stage/physical.sh" 820a2c9d14cdb6314ea61d0306240b61502f247c7f955fdf83a6594251764508
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af
exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16
exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389
exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$model" && test ! -L "$model" && [ "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:18973870528 ]
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ]
value=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$value"
[ "$value" = 'P100A_RELOAD driver55 tag2026102100010012 generation22 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
value=$(ps -o Id -o Team)
printf '%s\n' "$value" | awk -v closed_parent="$closed_parent" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==closed_parent)parent_live++
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners || parent_live)exit 1}'
value=$(sysinfo)
printf '%s\n' "$value"
printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
[ "$(cat "$stage/fixed.exit")" = 0 ]
[ "$(cat "$stage/state")" = EXACT_Q4_CLOSED_STABLE22_RESTORED ]
[ "$(cat "$stage/parent.pid")" = "$closed_parent" ]
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once"
test ! -s "$stage/fixed.stderr" && test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming"
test -f "$stage/fixed.stdout" && test ! -L "$stage/fixed.stdout"
[ "$(stat -c '%u:%h:%a:%s' "$stage/fixed.stdout")" = "0:1:600:$closed_bytes" ]
value=$(sha256sum "$stage/fixed.stdout")
printf '%s\n' "$value"
[ "${value%% *}" = "$closed_sha" ]
printf '%s\n' PASS_INDEPENDENT_POST_TERMINAL_STABLE22_IDLE_64CPU_FULL_RAM_APPS_RUNTIME_UNCHANGED
