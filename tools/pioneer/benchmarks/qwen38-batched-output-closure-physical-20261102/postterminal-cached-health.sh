#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-batched-output-closure-20261102-a
exact() {
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:644|0:1:555|0:1:700|0:1:600) ;; *) return 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 7c85898c5591115521e5f38c9ee546da7909a1da82b9a025046c5f8c81d1b623
exact "$stage/tenstorrent_pci_probe-BATCHED-OUTPUT-CLOSURE34-DIAGNOSTIC-ONLY" 7c85898c5591115521e5f38c9ee546da7909a1da82b9a025046c5f8c81d1b623
exact "$stage/pioneer_q38_batched_output_closure_exchange.sh" 0d8dc2eb62439daefd9acadd9a2addbc21cec36e4ccc713902025001b9a246db
exact "$stage/pioneer-q38-BATCHED-OUTPUT-CLOSURE34-DIAGNOSTIC-ONLY" f8efe67a51d9685a9e7e3ce57267ed2ae4967600c2802dd21f5f6ee0c24a22c9
exact "$stage/stable041.backup" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ]
for name in driver.incoming .exchange-lock; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
# Record cached health without granting restoration if cold/owned/uncertain.
# Caller interprets these exact fields only AFTER the model parent terminates.
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
echo POST_TERMINAL_CACHED_METADATA_ONLY_NO_CARD_IO_OR_RECOVERY
