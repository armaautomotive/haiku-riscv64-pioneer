#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
exact() {
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:644|0:1:555|0:1:700|0:1:600) ;; *) return 1;; esac
 row=$(sha256sum "$1")
 printf '%s\n' "$row"
 [ "${row%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-frontier-failure-link-20261102-a/pioneer_q38_frontier_failure_link_exchange.sh 7db41f36fb50d1eba4270b2be90b0e8242111f64dc124d072e5f3161095328e3
exact /boot/home/pioneer-p100a-frontier-failure-link-20261102-a/pioneer-q38-FRONTIER34-FAILED-REPLY-DIAGNOSTIC-ONLY f8efe67a51d9685a9e7e3ce57267ed2ae4967600c2802dd21f5f6ee0c24a22c9
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ]
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-batched-output-closure/) {print $1,"PROJECT_COMPUTE_PARENT_PRESENT";owner++}
  next
 }
 NF {invalid++}
 END {print "PROJECT_COMPUTE_PARENT_COUNT",owner+0;if(header!=1 || !teams || invalid || owner)exit 1}
'
sysinfo -cpu
sysinfo -mem
test -f /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
stat -c 'ORIGINAL_MODEL_METADATA %u:%h:%a:%s' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test ! -e /boot/home/pioneer-p100a-batched-output-closure-20261102-a
test ! -L /boot/home/pioneer-p100a-batched-output-closure-20261102-a
echo NEW_STAGE_ABSENT
