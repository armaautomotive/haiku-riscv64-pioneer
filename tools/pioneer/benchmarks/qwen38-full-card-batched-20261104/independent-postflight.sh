#!/bin/sh
# Read-only AFTER the exact physical parent actually exited0.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 value=$(sha256sum "$1")
 printf '%s\n' "$value"
 [ "${value%% *}" = "$2" ]
}
stage=/boot/home/pioneer-q38-full-batched-20261104-a
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933
exact "$stage/tenstorrent_pci_probe-full-batched89" de688dc5e682fa7714ac53450657c92052b139831eed7dce91b2300f82449eb6
exact "$stage/HaikuAssistantQwen27FullOffload-batched" bca5625332f3e94d37a233d30b69d355fce0cc9a2cc835c4348eb2835d3a9cd4
exact "$stage/pioneer-q38-full-offload-smoke" 32e1ed595821dc4ee8497ed36a8502f8e443e617d751a149198ab1e676cae80c
exact "$stage/pioneer_q38_full_offload_batched_physical.sh" 90001ae65aab8eac168406186728a8c20ba9e22cfe96993ef87fedd25359e86d
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
value=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$value"
[ "$value" = 'P100A_RELOAD driver55 tag2026102100010012 generation14 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
value=$(ps -o Id -o Team)
printf '%s\n' "$value" | awk '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
value=$(sysinfo)
printf '%s\n' "$value"
printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
[ "$(cat "$stage/full-offload.exit")" = 0 ]
[ "$(cat "$stage/state")" = FULL_CARD_HELLO_FOUR_CLOSED_STABLE_RESTORED ]
test ! -s "$stage/full-offload.stderr" && test ! -e "$stage/driver.incoming"
sha256sum "$stage/full-offload.stdout"
printf '%s\n' PASS_INDEPENDENT_POST_TERMINAL_STABLE14_IDLE_64CPU_FULL_RAM_APPS_UNCHANGED
