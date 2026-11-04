#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
exact() {
 test -f "$1" && test ! -L "$1"
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
printf '%s\n' "$metadata" | awk '
 $1=="P100A_RELOAD" && $2=="driver55" && $3=="tag2026102100010012" && $4=="generation10" &&
 $5=="quiesced0" && $6=="cold0" && $7=="active_owner0" && $8=="open_count1" && $9 ~ /^guard1;/ && NF==9 {good++;next}
 NF {bad++} END {if(good!=1 || bad)exit 1}'
cpu=$(sysinfo -cpu);printf '%s\n' "$cpu";case "$cpu" in '64 '*) ;; *) exit 1;; esac
memory=$(sysinfo -mem);printf '%s\n' "$memory"
printf '%s\n' "$memory" | awk 'index($0,"/ 137034936320)"){good++} END{if(good!=1)exit 1}'
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$model" && test ! -L "$model"
[ "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:18973870528 ]
cache=/boot/home/models/.p100a-lossless-weights-v1
test -d "$cache" && test ! -L "$cache" && [ "$(stat -c '%u:%a' "$cache")" = 0:700 ]
ps -o Id -o Team | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-dram-cohort|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/) owner++
  next
 }
 NF {invalid++}
 END {print "PROJECT_COMPUTE_PARENT_COUNT",owner+0;if(header!=1 || !teams || invalid || owner)exit 1}'
echo READONLY_CLEAN041_GEN10_64CPU_FULLRAM_CACHE_ENABLED_WORKER_UNCHANGED_NO_GPU_OR_WRITES
