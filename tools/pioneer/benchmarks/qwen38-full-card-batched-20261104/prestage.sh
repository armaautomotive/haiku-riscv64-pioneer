#!/bin/sh
# Create ONLY the new private stage after a fresh readonly admission.
set -eu
umask 077
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-q38-full-batched-20261104-a
test ! -e "$stage" && test ! -L "$stage"
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
test -f "$installed" && test ! -L "$installed"
digest=$(sha256sum "$installed")
[ "${digest%% *}" = 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 ]
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
digest=$(sha256sum "$admin")
[ "${digest%% *}" = b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f ]
value=$("$admin" --status)
printf '%s\n' "$value"
[ "$value" = 'P100A_RELOAD driver55 tag2026102100010012 generation12 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
value=$(sysinfo)
printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
mkdir "$stage"
[ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
printf '%s\n' NEW_BATCHED_STAGE_CREATED_NO_DRIVER_EXCHANGE_OR_INFERENCE
