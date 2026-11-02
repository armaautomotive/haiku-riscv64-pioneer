#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-token-phase-20261102-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 [ "$(stat -c '%u:%h:%a' "$1")" = "${3:-0:1:755}" ]
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-TOKEN-PHASE34-DIAGNOSTIC-ONLY" 0f2066ca48b24ae36550e57dd5334aa29645deb7f65dc8fd7bbbb9629f674e86
exact "$stage/pioneer-q38-TOKEN-PHASE34-DIAGNOSTIC-ONLY" e80d575933ffbb8211cd30fc84d97f818886d27c92d78faf3938d86ca0a8ec7c
exact "$stage/pioneer_q38_token_phase_profile_exchange.sh" 3a2a9c8ce534627f9ea0e026ed76931bac9beed93d38cae8e5ebb38ef6b75c7d
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 0:1:700
exact /boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
for name in driver.incoming .exchange-lock stable041.backup; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026102100010012 generation6 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
/bin/sh "$stage/pioneer_q38_token_phase_profile_exchange.sh" --apply
