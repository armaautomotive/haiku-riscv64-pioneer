#!/bin/sh
# No driver/model execution. Check current admission, then create new stage only.
set -eu
umask 077
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-q38-full-metrics-20261104-a
test ! -e "$stage" && test ! -L "$stage"
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 value=$(sha256sum "$1")
 printf '%s\n' "$value"
 [ "${value%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
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
mkdir "$stage"
[ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
printf '%s\n' NEW_PRIVATE_METRICS_STAGE_CREATED_NO_EXCHANGE_OR_MODEL_EXECUTION
