#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-token-constant-q4-20261102-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:644|0:1:555|0:1:700|0:1:600) ;; *) return 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY" db032591d905a30debf1e9a36f5990301e3db9ba40bfa8cf945677e3f54e8106
exact "$stage/pioneer-q38-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY" 3edc924a9e76fdcd42815fa49b6d2756d80c6a18e3a3eb8bc9d4b765345ce1de
exact "$stage/pioneer_q38_token_constant_q4_exchange.sh" 7fae5efe1add752818c04459369e6c456364941ff8240afc9ae7a09b55f9ddbb
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026102100010012 generation8 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
chmod 755 "$stage/tenstorrent_pci_probe-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY" "$stage/pioneer-q38-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY" "$stage/pioneer_q38_token_constant_q4_exchange.sh"
for name in tenstorrent_pci_probe-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY pioneer-q38-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY pioneer_q38_token_constant_q4_exchange.sh; do
 [ "$(stat -c '%u:%h:%a' "$stage/$name")" = 0:1:755 ]
done
for name in driver.incoming .exchange-lock stable041.backup; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
echo WHOLE_STAGE_IDENTITIES_EXACT_STABLE041_STILL_INSTALLED_NO_DRIVER_OR_CLIENT_EXECUTION
