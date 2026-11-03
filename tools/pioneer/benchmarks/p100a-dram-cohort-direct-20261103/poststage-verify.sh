#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-dram-cohort-direct-20261103-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:644) ;; *) exit 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-dram-cohort-direct77" f35559c24c60a36dfdd94cb6f965502bedcbd38be7f6ffe66ca317a5a872e195
exact "$stage/pioneer-dram-cohort-direct77" 1ad52983872710c801825a09c2e8794842bc02fbc92b8f98edb00948e9c0178a
exact "$stage/pioneer_dram_cohort_direct_exchange.sh" 9bc85769c6a79a295bb14600d8669b8fd41f4bd5e637a577cb40e7937367a7b0
chmod 755 "$stage/tenstorrent_pci_probe-dram-cohort-direct77" "$stage/pioneer-dram-cohort-direct77" "$stage/pioneer_dram_cohort_direct_exchange.sh"
for name in tenstorrent_pci_probe-dram-cohort-direct77 pioneer-dram-cohort-direct77 pioneer_dram_cohort_direct_exchange.sh; do
 [ "$(stat -c '%u:%h:%a' "$stage/$name")" = 0:1:755 ]
done
for name in driver.incoming .exchange-lock stable041.backup; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
echo EXACT_THREE_DIRECT_STAGED_OBJECTS_VERIFIED_NO_INSTALL_OR_GPU_RUN
