#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-dram-cohort-20261103-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:700) ;; *) exit 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe e50f1dfdb05827ee020a106aea474b3e926ad7a8f17de3decfbd71cd72b2e0e3
exact "$stage/tenstorrent_pci_probe-dram-cohort-diagnostic77" e50f1dfdb05827ee020a106aea474b3e926ad7a8f17de3decfbd71cd72b2e0e3
exact "$stage/pioneer-dram-cohort-diagnostic77" 7d2e3e8ec343c117173d94a706e4c1cbbd75666af094c7fe170a6e33d843e8a4
exact "$stage/pioneer_dram_cohort_exchange.sh" be8da789613c56f9efa45e8a9fa93d413a123c84d192d69ccb51d179f3aa525e
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ]
for name in driver.incoming .exchange-lock; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026110300010077 generation13 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
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
cpu=$(sysinfo -cpu);printf '%s\n' "$cpu";case "$cpu" in '64 '*) ;; *) exit 1;; esac
memory=$(sysinfo -mem);printf '%s\n' "$memory"
printf '%s\n' "$memory" | awk 'index($0,"/ 137034936320)"){good++} END{if(good!=1)exit 1}'
echo EXACT_COHORT77_GENERATION13_IDLE_64CPU_FULLRAM
