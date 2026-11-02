#!/bin/sh
set -eu
# Exact subject only;postterminal/restore scripts MUST NOT run while a compute parent lives.
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-token-matrix-phase-20261102-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 [ "$(stat -c '%u:%h:%a' "$1")" = "${3:-0:1:755}" ]
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY" 5ae9d64ae4757c4635e4bbc6f59cf8350e8f549427e99e1e99e5897092616e39
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact "$stage/pioneer-q38-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY" 559f986ddc88bbc2ff8cbe034dcda1546d385b366177f027a245452e605c7a22
exact "$stage/pioneer_q38_token_matrix_phase_exchange.sh" 761dfd1494ed3e7963d643572ece1183b07b702471fc2d528d41712c049315bf
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 0:1:700
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 0:1:700
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ]
for name in driver.incoming .exchange-lock; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026102100010012 generation12 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-batched-output-closure/ || $0 ~ /pioneer-p100a-wide-gdn/ || $0 ~ /pioneer-p100a-token-phase/ || $0 ~ /pioneer-p100a-token-constant-q4/ || $0 ~ /pioneer-p100a-token-matrix-phase/) owner++
  next
 }
 NF {invalid++}
 END {print "PROJECT_COMPUTE_PARENT_COUNT",owner+0;if(header!=1 || !teams || invalid || owner)exit 1}
'
model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$model" && test ! -L "$model" && [ "$(stat -c '%u:%h:%s' "$model")" = 0:1:18973870528 ]
sysinfo -cpu
sysinfo -mem
echo EXACT_STABLE041_RESTORED_GENERATION12_IDLE_NO_COMPUTE
