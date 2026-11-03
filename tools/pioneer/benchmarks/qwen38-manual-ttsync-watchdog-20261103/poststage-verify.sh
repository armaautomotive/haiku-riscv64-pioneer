#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-p100a-token-manual-ttsync-20261103-a
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
exact(){
 test -f "$1" && test ! -L "$1"
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:755|0:1:644) ;; *) exit 1;; esac
 row=$(sha256sum "$1");printf '%s\n' "$row";[ "${row%% *}" = "$2" ]
}
exact "$stage/tenstorrent_pci_probe-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY" c01f1cda7549ef3b191454ab6c21d697de143f3f11fcc5c9da48c81dd37c1736
exact "$stage/pioneer-q38-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY" aaffcaa125a02d1629dc49055d45d26af28f3187049dda631a8b5a272a9c165a
exact "$stage/pioneer_q38_token_manual_ttsync_exchange.sh" 32114b2ad5139df0dab8a238c1f3c885033c7fc8b34428ebaffaab3982db04cd
chmod 755 "$stage/tenstorrent_pci_probe-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY" "$stage/pioneer-q38-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY" "$stage/pioneer_q38_token_manual_ttsync_exchange.sh"
for name in tenstorrent_pci_probe-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY pioneer-q38-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY pioneer_q38_token_manual_ttsync_exchange.sh; do
 [ "$(stat -c '%u:%h:%a' "$stage/$name")" = 0:1:755 ]
done
for name in driver.incoming .exchange-lock stable041.backup; do test ! -e "$stage/$name" && test ! -L "$stage/$name"; done
echo EXACT_THREE_MANUAL_STAGED_OBJECTS_VERIFIED_NO_INSTALL_OR_GPU_RUN
