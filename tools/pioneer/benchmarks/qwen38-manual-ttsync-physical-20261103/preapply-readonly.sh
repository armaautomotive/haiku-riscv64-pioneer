#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-dram-cohort|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/) owner++
  next
 }
 NF {invalid++}
 END {print "PROJECT_COMPUTE_PARENT_COUNT",owner+0;if(header!=1 || !teams || invalid || owner)exit 1}
'
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:700) ;; *) exit 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ]
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
printf '%s\n' "$metadata" | awk '
 $1=="P100A_RELOAD" && $2=="driver55" && $3=="tag2026102100010012" && $4=="generation16" &&
 $5=="quiesced0" && $6=="cold0" && $7=="active_owner0" && $8=="open_count1" && $9 ~ /^guard1;/ && NF==9 {good++;next}
 NF {bad++} END {if(good!=1 || bad)exit 1}'
cpu=$(sysinfo -cpu);printf '%s\n' "$cpu";case "$cpu" in '64 '*) ;; *) exit 1;; esac
memory=$(sysinfo -mem);printf '%s\n' "$memory"
printf '%s\n' "$memory" | awk 'index($0,"/ 137034936320)"){good++} END{if(good!=1)exit 1}'
stage=/boot/home/pioneer-p100a-token-manual-ttsync-20261103-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact "$stage/tenstorrent_pci_probe-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY" c01f1cda7549ef3b191454ab6c21d697de143f3f11fcc5c9da48c81dd37c1736
exact "$stage/pioneer-q38-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY" aaffcaa125a02d1629dc49055d45d26af28f3187049dda631a8b5a272a9c165a
exact "$stage/pioneer_q38_token_manual_ttsync_exchange.sh" 32114b2ad5139df0dab8a238c1f3c885033c7fc8b34428ebaffaab3982db04cd
for name in driver.incoming .exchange-lock stable041.backup; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$model" && test ! -L "$model"
[ "$(stat -c '%u:%h:%s' "$model")" = 0:1:18973870528 ]
echo READONLY_STABLE041_GEN16_IDLE_64CPU_FULLRAM_MODEL_STAT_AND_EXACT_STAGED_TUPLE_NO_GPU
