#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-dram-cohort-20261103-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:644) ;; *) exit 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-dram-cohort-diagnostic77" e50f1dfdb05827ee020a106aea474b3e926ad7a8f17de3decfbd71cd72b2e0e3
exact "$stage/pioneer-dram-cohort-diagnostic77" 7d2e3e8ec343c117173d94a706e4c1cbbd75666af094c7fe170a6e33d843e8a4
exact "$stage/pioneer_dram_cohort_exchange.sh" be8da789613c56f9efa45e8a9fa93d413a123c84d192d69ccb51d179f3aa525e
chmod 755 "$stage/tenstorrent_pci_probe-dram-cohort-diagnostic77" "$stage/pioneer-dram-cohort-diagnostic77" "$stage/pioneer_dram_cohort_exchange.sh"
for name in tenstorrent_pci_probe-dram-cohort-diagnostic77 pioneer-dram-cohort-diagnostic77 pioneer_dram_cohort_exchange.sh; do
 [ "$(stat -c '%u:%h:%a' "$stage/$name")" = 0:1:755 ]
done
for name in driver.incoming .exchange-lock stable041.backup; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
echo EXACT_THREE_STAGED_OBJECTS_VERIFIED_NO_INSTALL_OR_GPU_RUN
